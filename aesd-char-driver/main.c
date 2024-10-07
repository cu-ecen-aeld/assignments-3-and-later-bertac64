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

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#include "aesd-circular-buffer.h"
#include "aesdchar.h"
#include "aesd_ioctl.h"

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
    
    if (!dev) return -EFAULT;
    
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("release");
    /**
     * TODO: handle release
     */
     
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    
    if (!dev) return -EFAULT;
    
    return 0;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
	struct aesd_dev *dev;
	loff_t newpos;
	size_t buf_size = 0;
	size_t i;
	
	PDEBUG("aesd_llseek");
	
	dev = filp->private_data;
	
	// mutex activation
	if (mutex_lock_interruptible(&dev->lock)) return -ERESTARTSYS;
	
	// check buffer dimension
	for(i=0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
		buf_size += dev->buffer->entry[i].size;
	}
	
	switch(whence) {
		case 0: /* SEEK_SET */
			newpos = off;
			break;
		
		case 1: /* SEEK_CUR */
			newpos = filp->f_pos + off;
			break;
		
		case 2: /* SEEK_END */
			newpos = buf_size + off;
			break;
		
		default: /* can't happen */
			mutex_unlock(&dev->lock);
			return -EINVAL;
	}
	if ((newpos < 0) || (newpos > buf_size)){
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}
	filp->f_pos = newpos;
	
	mutex_unlock(&dev->lock);
	return newpos;
}

long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct aesd_dev *dev = filp->private_data;
	int retval = 0;
	size_t buf_size = 0;
	size_t i;
	struct aesd_seekto seekto;
	struct aesd_buffer_entry *buff;
	
	PDEBUG("aesd_unlocked_ioctl");
	// check if wrong ioctl command
	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;
	
	if (! capable (CAP_SYS_ADMIN))
		return -EPERM;
		
	if (copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto)))
		return -EFAULT;

	// mutex activation
	if (mutex_lock_interruptible(&dev->lock)) return -ERESTARTSYS;
	
	//check if the buffer position is valid
	if(seekto.write_cmd > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}
	//check if the value to be written is present
	if(dev->buffer->entry[seekto.write_cmd].buffptr == NULL){
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}
	
	//take the value to be written
	buff = &dev->buffer->entry[seekto.write_cmd];
	
	//check if the offset exceed the dimension of the buffer
	if(seekto.write_cmd_offset >= buff->size){
		mutex_unlock(&dev->lock);
		return -EINVAL;
	}
	
	// calculate the dimension of the buffer
	for(i=0; i < seekto.write_cmd; i++){
		buf_size += dev->buffer->entry[i].size;
	}
	
	//search position updated
	filp->f_pos = buf_size + seekto.write_cmd_offset;
	
	PDEBUG("ioctl executed correctly");
	mutex_unlock(&dev->lock);
	return retval;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct aesd_dev *dev;
    struct aesd_buffer_entry *entry;
    size_t entry_offset_byte_rtn;
    size_t available_bytes;
    size_t bytes_to_read;
    size_t total_size;
    ssize_t retval;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
 
    /**
     * TODO: handle read
     */
    
    dev = (struct aesd_dev *)filp->private_data;
    retval = 0;
    entry_offset_byte_rtn = 0;
    total_size = 0;
    available_bytes = 0;
     
    if (mutex_lock_interruptible(&dev->lock)) return -ERESTARTSYS;
    
	entry= aesd_circular_buffer_find_entry_offset_for_fpos(dev->buffer, *f_pos, &entry_offset_byte_rtn);
	if (!entry || !entry->buffptr){
		retval = 0;	//no available data
	}else{		
		available_bytes = entry->size - entry_offset_byte_rtn;
		
		if (count < available_bytes){
			bytes_to_read = count;
		}else{
			bytes_to_read = available_bytes;
		}
		
		if (copy_to_user(buf, entry->buffptr + entry_offset_byte_rtn, bytes_to_read)) {
			retval = -EFAULT;
			goto out;
		}else{
			*f_pos += bytes_to_read;
			retval = bytes_to_read;
		}
	}
	
  out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev;
    struct aesd_buffer_entry entry;
    char *write_buffer;
    char *new_entry;
    size_t new_entry_size;
    ssize_t retval;
    size_t i;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    /**
     * TODO: handle write
     */
    
    dev = (struct aesd_dev *)filp->private_data;
    retval = -ENOMEM;
    new_entry_size = 0;
    
    if (!dev) return -EFAULT;
    
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
    	if (write_buffer[i] == '\n'){
    		new_entry_size = i + 1;
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
	
	entry.buffptr = new_entry;
	entry.size = retval;
	
	aesd_circular_buffer_add_entry(dev->buffer, &entry);

	
	kfree(write_buffer);
	kfree(dev->partial_write);
	dev->partial_write = NULL;
	dev->partial_size = 0;
    
  out:
    mutex_unlock(&dev->lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =			THIS_MODULE,
    .read =				aesd_read,
    .write =			aesd_write,
    .llseek =			aesd_llseek,
    .unlocked_ioctl = 	aesd_unlocked_ioctl,
    .open =				aesd_open,
    .release =			aesd_release,
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
