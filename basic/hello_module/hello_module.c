#include <linux/module.h>
#include <linux/init.h>

static int __init hello_init(void)
{
    printk("hello linux\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk("bye linux\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mqz");