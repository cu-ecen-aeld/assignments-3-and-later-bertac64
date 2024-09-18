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
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include <linux/kernel.h>
#include <linux/uaccess.h>

#include "aesd-circular-buffer.h"
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

#define DEVICE_NAME "aesdchar"

MODULE_AUTHOR("bertac64"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct aesd_dev *dev;
    ssize_t retval;
    struct aesd_buffer_entry *entry;
    size_t entry_offset_byte_rtn;
    size_t available_bytes;
    size_t bytes_read;
    size_t total_size;
    int i;
    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    dev = (struct aesd_dev *)filp->private_data;
    retval = 0;
    entry_offset_byte_rtn = 0;
    bytes_read = 0;
    total_size = 0;
     
    if (mutex_lock_interruptible(&dev->lock)) return -ERESTARTSYS;
    
    //check if the buffer is empty
    for (i=0; i<AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
    	entry = &dev->buffer->entry[i];
    	if (entry->buffptr != NULL){
    		total_size += entry->size;
    	}
    }
    if (*f_pos >= total_size){
    	retval = 0;
    	goto out;
    }
	

	entry= aesd_circular_buffer_find_entry_offset_for_fpos(dev->buffer, *f_pos, &entry_offset_byte_rtn);
	if (!entry){
		retval = 0;	//no available data
		goto out;
	}
	
	available_bytes = entry->size - entry_offset_byte_rtn;
	if(available_bytes > count) available_bytes = count;
	
	if (copy_to_user(buf + bytes_read, entry->buffptr + entry_offset_byte_rtn, available_bytes)) {
	    retval = -EFAULT;
	    goto out;
	}
	printk(KERN_INFO "aesdchar: %s",buf);

	*f_pos += available_bytes;
	bytes_read +=available_bytes;
		
	
	retval = bytes_read;
	printk(KERN_INFO "aesdchar final: %s",buf);
  out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev;    
    ssize_t retval;
    char *write_buffer;
    char *new_entry;
    size_t new_entry_size;
    int i;
    struct aesd_buffer_entry entry;
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     
    dev = (struct aesd_dev *)filp->private_data;
    retval = -ENOMEM;
    new_entry_size = 0;
    
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    
    write_buffer = kmalloc(count, GFP_KERNEL);
    if(!write_buffer){
    	retval = -ENOMEM;
    	goto out;
    }
    
    if (copy_from_user(write_buffer, buf, count)) {
        retval = -EFAULT;
        kfree(write_buffer);
        goto out;
    }
    
    for (i = 0; i < count; i++){
//    	printk(KERN_INFO "aesdchar: %c",write_buffer[i]);
    	if (write_buffer[i] == '\n'){
    		new_entry_size = i + 1;
//    		dev->partial_size = 0;
//    		printk(KERN_INFO "\n");
    		break;
    	}
    }
    
    if (new_entry_size == 0){
    	dev->partial_write = krealloc(dev->partial_write, dev->partial_size + count, count);
    	if(!dev->partial_write){
    		retval = -ENOMEM;
    		kfree(write_buffer);
        	goto out;
    	}
    	memcpy(dev->partial_write + dev->partial_size, write_buffer, count);
    	dev->partial_size += count;
    	retval = count;
    	kfree(write_buffer);
    	goto out;
    }
    
    new_entry = kmalloc(dev->partial_size + new_entry_size, GFP_KERNEL);
    if(!new_entry){
		retval = -ENOMEM;
		kfree(write_buffer);
    	goto out;
	}
	
	if(dev->partial_size > 0){
		memcpy(new_entry, dev->partial_write, dev->partial_size);
		
	}
    
    memcpy(new_entry + dev->partial_size, write_buffer, new_entry_size);
    retval = new_entry_size + dev->partial_size;
    kfree(write_buffer);
    kfree(dev->partial_write);
	dev->partial_write = NULL;
	dev->partial_size = 0;
    
    entry.buffptr = new_entry;
    entry.size = retval;
    
    aesd_circular_buffer_add_entry(dev->buffer, &entry);
    
    
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
    if (err < 0) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    int result;
    /**
     * TODO: initialize the AESD specific portion of the device
     */
    // major and minor number dynamic allocation
    result = alloc_chrdev_region(&aesd_device.dev_num, aesd_minor, 1, DEVICE_NAME);
    aesd_major = MAJOR(aesd_device.dev_num);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    
    // cdev initialization and adding
    result = aesd_setup_cdev(&aesd_device);
    if( result < 0 ) {
        unregister_chrdev_region(aesd_device.dev_num, 1);
        printk(KERN_WARNING "aesdchar: cdev_add failed\n");
        return result;
    }
    
    // circular buffer memeory allocation
    aesd_device.buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
    if( !aesd_device.buffer ) {
        unregister_chrdev_region(aesd_device.dev_num, 1);
        printk(KERN_WARNING "aesdchar: cdev_add failed\n");
        return -ENOMEM;
    }
    // circular buffer initialization
    aesd_circular_buffer_init(aesd_device.buffer);
    // mutex initialization
    mutex_init(&aesd_device.lock);
	
    printk(KERN_WARNING "aesdchar: device initialized with major= %d minor=%d\n", aesd_major, aesd_minor);
    return 0;
}

void aesd_cleanup_module(void)
{
    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
	cdev_del(&aesd_device.cdev);
    unregister_chrdev_region(aesd_device.dev_num, 1);
    // freeing the circular buffer memeory 
    kfree(aesd_device.buffer);
    // mutex destruction
    mutex_destroy(&aesd_device.lock);
    
    printk(KERN_INFO "aesdchar: device removed\n");
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
