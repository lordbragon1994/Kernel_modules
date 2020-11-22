#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Brach");

#define DEV_CNT 6
#define DEV_NAME "keyboard_time"

/*
*   Declaration function of device driver
*/
static int     dev_open  (struct inode *, struct file *);
static ssize_t dev_read  (struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write (struct file *, const char __user *, size_t, loff_t *);

static dev_t dev; /* dev represents device numbers within the kernel*/
static struct cdev my_cdev; /* Struct cdev represents char devices internally*/

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = dev_read,
  .write = dev_write,
  .open = dev_open
};

/* Define the method open of fops*/
static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Successfully time opened! \n");
    
    return 0;
}

static ssize_t dev_read (struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    int ret = 0;
    if (*offp >= sizeof(int)) {
        return 0;
    }

    if(!buff){
        return -EINVAL;
    }

    ktime_t time = ktime_get_real();
        
   /* Transfer data from Kernel to user */
    if (copy_to_user(buff, &time, sizeof(ktime_t))) {
        printk(KERN_INFO "Send to user failed\n");
        
        return -EFAULT;
    }
    ret = sizeof(ktime_t);
    *offp += count;

    /* Confirm that user got exact data */
    printk (KERN_INFO "User receve: %ld \n", sizeof(time));

    return ret;
}

static ssize_t dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    printk(KERN_INFO "NOP\n");
    
    return -1;
}

int __init my_time_init(void)
{
    if(alloc_chrdev_region(&dev, 0, DEV_CNT, DEV_NAME)) {
        printk(KERN_ERR "alloc_chrdev_region() failed ! \n");
        
        return -1;
    }

    printk(KERN_INFO "Allocated %d devices at Major: %d\n", DEV_CNT, MAJOR(dev));
    
    /* Initialize the character device and add it to the kernel */
    cdev_init(&my_cdev, &fops);
    
    my_cdev.owner = THIS_MODULE;

    if(cdev_add(&my_cdev,dev,DEV_CNT))
    {
        printk(KERN_ERR "cdev_add() failed ! \n");
        /* clean up chrdev allocation if failed */
        unregister_chrdev_region(dev, DEV_CNT);
        
        return -1;
    }
    
    printk(KERN_INFO "TIME INIT\n");
    return 0;
}

void __exit my_time_exit(void)
{
    printk(KERN_INFO "TIME DEINIT!\n");
    /* delete the cdev */
    cdev_del(&my_cdev);

    /* clean up the devices */
    unregister_chrdev_region(dev, DEV_CNT);
}


module_init(my_time_init);
module_exit(my_time_exit);
