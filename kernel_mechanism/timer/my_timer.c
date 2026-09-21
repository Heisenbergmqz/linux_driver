#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>

static void my_timer_callback(void);
DEFINE_TIMER(my_timer, my_timer_callback);

static void my_timer_callback(void)
{   
    printk("my_timer_callback\n");
    mod_timer(&my_timer, jiffies_64 + msecs_to_jiffies(5000));
}
static int __init my_timer_init(void)
{   
    my_timer.expires = jiffies_64 + msecs_to_jiffies(5000);
    add_timer(&my_timer);
    return 0;
}
static void __exit my_timer_exit(void)
{
    del_timer(&my_timer);
}
module_init(my_timer_init);
module_exit(my_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("heisenberg");
MODULE_DESCRIPTION("Character device template");
MODULE_VERSION("1.0");