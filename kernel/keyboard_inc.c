#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Brach");

#define IRQ_KEYBORD_NUMBER 1

/*
*   Global variable and parameter
*/
static int counter = 0;

irqreturn_t irq_handler(int irq, void *dev_id)
{
    counter++;
    
    static unsigned char scancode;
    unsigned char status;

    status = inb(0x64);
    scancode = inb(0x60);

    printk (KERN_INFO "Pressed %d (%d)\n", scancode, counter);

    return IRQ_HANDLED;
}

static int __init irq_ex_init(void)
{
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
        printk (KERN_INFO "Can't create handler interrupt for keyboard\n");    
    }
    else {
        printk (KERN_INFO "Handler for keybordinterrupt created successfully\n");    
    }
    
    return ret;
}

static void __exit irq_ex_exit(void)
{
    printk( KERN_INFO "KEYBORD MODULE UNLOAD \n");
    free_irq(1,NULL);
}

module_init(irq_ex_init);
module_exit(irq_ex_exit);