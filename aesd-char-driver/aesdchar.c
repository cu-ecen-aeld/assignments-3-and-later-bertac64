/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/kernel.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;


MODULE_AUTHOR("bertac64"); /** fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");
#define DEVICE_NAME "aesdchar"

struct aesd_dev aesd_device;


int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * handle open
     */
    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    size_t entry_offset_byte_rtn = 0;
    size_t available_bytes;
    int read_count = 0;
    ssize_t retval = 0;
    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;
    while (count > 0){
        entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset_byte_rtn);
        if (entry == NULL) {
            retval = 0; // End of file or no data to read
            goto out;
        }

        available_bytes = entry->size - entry_offset_byte_rtn;
        if (count > available_bytes) {
            count = available_bytes;
        }
        
        if (copy_to_user(buf, entry->buffptr + entry_offset_byte_rtn, count)) {
            retval = -EFAULT;
            goto out;
        }
        count -= available_bytes;
        *f_pos += available_bytes;
        
        bytes_read += available_bytes;
        if (count == 0) {
            break;
        }
    }
    
    retval = bytes_read;
  out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;    
    ssize_t retval = -ENOMEM;
    char *write_buffer;
    char *new_entry;
    size_t new_entry_size = 0;
    int i;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    if(*f_pos >= dev->size)
        goto out;
    // write buffer memory allocation
    write_buffer = kmalloc(count, GFP_KERNEL);
    if (!write_buffer) {
        retval = -ENOMEM;
        goto out;
    }
    
    // store the data into the write_buffer
    if (copy_from_user(write_buffer, buf, count)) {
        retval = -EFAULT;
        kfree(write_buffer);
        goto out;
    }
    
    // Search for '\n' to find the end of the command 
    for (i = 0; i < count; i++) {
        if (write_buffer[i] == '\n') {
            new_entry_size = i + 1; // Includes '\n' into the data
            break;
        }
    }
    // if no '\n', append the data
    if (new_entry_size == 0) {
        // buffer allocation for partial write
        dev->partial_write = krealloc(dev->partial_write, dev->partial_size + count, GFP_KERNEL);
        if (!dev->partial_write) {  //check if allocation fails
            retval = -ENOMEM;
            kfree(write_buffer);
            goto out;
        }
        // store write_buffer into the partial write buffer
        memcpy(dev->partial_write + dev->partial_size, write_buffer, count);
        dev->partial_size += count;
        retval = count;
        kfree(write_buffer);
        goto out;
    }

    // Allocating the new entry into the circular buffer
    new_entry = kmalloc(dev->partial_size + new_entry_size, GFP_KERNEL);    //new entry buffer allocation
    if (!new_entry) {       // check if allocation fails
        retval = -ENOMEM;
        kfree(write_buffer);
        goto out;
    }
    
    // Copy the partial write, if present
    if (dev->partial_size > 0) {
        memcpy(new_entry, dev->partial_write, dev->partial_size);
        kfree(dev->partial_write);
        dev->partial_write = NULL;
        dev->partial_size = 0;
    }
    
    // Copy the new command into the new entry buffer
    memcpy(new_entry + dev->partial_size, write_buffer, new_entry_size);
    retval = new_entry_size + dev->partial_size;
    kfree(write_buffer);

    // Write the new entry buffer into the circular buffer
    aesd_circular_buffer_add_entry(&dev->buffer, new_entry, retval);

  out:
    mutex_unlock(&dev->lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    int result;
    
    //device number allocation
    result = alloc_chrdev_region(&aesd_device.dev_num, aesd_minor, 1, DEVICE_NAME);
    aesd_major = MAJOR(aesd_device.dev_num);
    if (result < 0) {
        printk(KERN_WARNING "aesdchar: alloc_chrdev_region failed. Can't get major %d\n", aesd_major);
        return result;
    }

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    // initialize aesd_device
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    // initalize  circular buffer
    aesd_circular_buffer_init(&aesd_device.buffer);
    // initialize mutex
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);
    if( result ) {
        unregister_chrdev_region(aesd_device.dev_num, 1);
        printk(KERN_WARNING "aesdchar: cdev_add failed\n");
    }
    printk(KERN_INFO "aesdchar: device initialized with major=%d minor=%d\n",
           aesd_major, aesd_minor);
    return result;
}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    unregister_chrdev_region(devno, 1);
    aesd_circular_buffer_cleanup(&aesd_device.buffer);
    mutex_destroy(&aesd_device.lock);
    printk(KERN_INFO "aesdchar: device removed\n");
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
