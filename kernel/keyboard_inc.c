#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Brach");

#define IRQ_KEYBORD_NUMBER 1

#define MY_WORK_QUEUE_NAME "WQ_schedule"
static DEFINE_MUTEX(cache_lock);

#define DEV_CNT 7
#define DEV_NAME "keyboard_inc"

#define MSG_MAX 32

#define LOG_RED            "\x1b[31m"
#define LOG_GREEN          "\x1b[32m"
#define LOG_YELLOW         "\x1b[33m"
#define LOG_BLUE           "\x1b[34m"
#define LOG_DEFAULT        "\x1b[39m"

/*
*   Declaration function of device driver
*/
static int     dev_open  (struct inode *, struct file *);
static ssize_t dev_read  (struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write (struct file *, const char __user *, size_t, loff_t *);

/*
*   Global variable and parameter
*/
static int counter = 0;
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
    printk(KERN_INFO "Successfully opened! \n");
    printk(KERN_INFO "Currently, counter: %d\n",counter);
    
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

	mutex_lock(&cache_lock);

   /* Transfer data from Kernel to user */
    if (copy_to_user(buff, &counter, sizeof(int))) {
        printk(KERN_INFO "Send to user failed\n");
        
        return -EFAULT;
    }
	
	mutex_unlock(&cache_lock);
	
    ret = sizeof(int);
    *offp += count;

    /* Confirm that user got exact data */
    printk (KERN_INFO "User receve: %d \n", counter);

    return ret;
}

static ssize_t dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    /* local kernel memory*/
    char *kern_buf;
    int ret;

    if (!buff) {
        return -EINVAL;
    } 

    /* Allocate memory in kernel */
    kern_buf = kmalloc (count, GFP_KERNEL);
    if (!kern_buf) {
        return -ENOMEM;
    }

    /* Transfer data from user to kernel through kernel buffer*/
    if(copy_from_user(kern_buf, buff, count)) {
        kfree(kern_buf);
        return -EFAULT;
    }

	mutex_lock(&cache_lock);

    /* Update global data in kernel */
    if(kstrtoint(kern_buf, MSG_MAX, &counter))
    {
        kfree(kern_buf);
        return -ERANGE;
    }
	
	mutex_unlock(&cache_lock);
	
    ret = count;
    
    /* Print result what userspace gave back the new value by system call */
    printk(KERN_INFO "Counter is reset %d\n",counter);

    return ret;
    
    return 0;
}

static struct workqueue_struct *my_workqueue;
typedef struct {
  struct work_struct my_work;
  unsigned char keycode;        /* Save scancode for workqueue */
} my_work_t;

my_work_t *work;

static void got_char(struct work_struct *work)
{

    my_work_t *my_work = (my_work_t *)work;
    
    if (my_work->keycode != 224) {
        if (my_work->keycode & 0x80) {
			
			mutex_lock(&cache_lock);
            counter++;
			mutex_unlock(&cache_lock);
			
            printk (KERN_INFO "PRESSEDD %d (%d)\n", my_work->keycode, counter);
        }
        else {
            printk (KERN_INFO "RELEASED %d (%d)\n", my_work->keycode, counter);
        }
    }

    kfree( (void *)work );
    return;
}


irqreturn_t irq_handler(int irq, void *dev_id)
{    
    static unsigned char scancode;
    unsigned char status;

    status = inb(0x64);
    scancode = inb(0x60);
    
    work = (my_work_t *) kmalloc(sizeof(my_work_t),GFP_KERNEL);
    work->keycode = scancode;
    INIT_WORK((struct work_struct *)work,got_char);
    queue_work(my_workqueue,(struct work_struct *) work);

    return IRQ_HANDLED;
}

static int __init irq_ex_init(void)
{
    my_workqueue = create_workqueue(MY_WORK_QUEUE_NAME);
    
    printk (KERN_INFO "INIT KEYBORD MODULE \n");
    
    /* Free interrupt*/
    free_irq(1,NULL);

    int ret = request_irq (
        IRQ_KEYBORD_NUMBER, 
        (irq_handler_t) irq_handler,
        IRQF_SHARED, 
        "test_keyboard_irq_handler",
        (void *)(irq_handler)
    );
    if (ret) {
        printk (KERN_ERR "Can't create handler interrupt for keyboard\n");    
    }
    else {
        printk (KERN_INFO "Handler for keybordinterrupt created successfully\n");    
    }
    
    if(alloc_chrdev_region(&dev, 0, DEV_CNT, DEV_NAME)) {
        printk(KERN_ERR "alloc_chrdev_region() failed ! \n");
        
        return -1;
    }

    printk(KERN_INFO "Allocated %d devices at %s Major: %d %s\n", DEV_CNT, LOG_BLUE, MAJOR(dev), LOG_DEFAULT);
    
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
        
    return ret;
}

static void __exit irq_ex_exit(void)
{
    printk( KERN_INFO "KEYBORD MODULE UNLOAD \n");
    free_irq(1,NULL);
    
    /* delete the cdev */
    cdev_del(&my_cdev);

    /* clean up the devices */
    unregister_chrdev_region(dev, DEV_CNT);
}

module_init(irq_ex_init);
module_exit(irq_ex_exit);