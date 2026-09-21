#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/kernel.h>

#include <linux/wait.h>
#include <linux/poll.h>
#define DRIVER_NAME         "char_demo"
#define CLASS_NAME          "char_demo"
#define DEVICE_NAME_BASE    "dev_char"

#define DEVICE_NUM          2
#define BUF_SIZE            1024


/* ============================================================
 * 1. 单个设备结构体
 * ============================================================ */
struct char_demo_dev {
    /* 字符设备框架相关 */
    dev_t devt;
    struct cdev cdev;
    struct device *device;

    /* 设备自己的数据 */
    char buf[BUF_SIZE];
    size_t data_len;
};


/* ============================================================
 * 2. 整个驱动的管理结构体
 * ============================================================ */
struct char_demo_driver {
    /* 申请到的一整段设备号的起始值 */
    dev_t base_devt;

    /* 所有设备共用一个 class */
    struct class *class;

    /* 本驱动管理的所有设备 */
    struct char_demo_dev devices[DEVICE_NUM];
};

wait_queue_head_t read_queue;
static struct char_demo_driver char_drv;


/* ============================================================
 * 3. 文件操作函数
 * ============================================================ */

/*
 * open("/dev/dev_char0")
 *
 * inode->i_cdev
 *      ↓
 * dev->cdev
 *      ↓ container_of
 * struct char_demo_dev
 *
 * 最后保存到 filp->private_data。
 */
static int char_demo_open(struct inode *inode, struct file *filp)
{
    struct char_demo_dev *dev;

    dev = container_of(inode->i_cdev,
                       struct char_demo_dev,
                       cdev);

    filp->private_data = dev;

    pr_info("%s: device opened, minor=%d\n",
            DRIVER_NAME,
            MINOR(dev->devt));

    return 0;
}


static int char_demo_release(struct inode *inode, struct file *filp)
{
    struct char_demo_dev *dev = filp->private_data;

    pr_info("%s: device closed, minor=%d\n",
            DRIVER_NAME,
            MINOR(dev->devt));

    return 0;
}

static ssize_t char_demo_read(struct file *filp,
                              char __user *buf,
                              size_t count,
                              loff_t *f_pos)
{
    struct char_demo_dev *dev = filp->private_data;

    if(filp->f_flags & O_NONBLOCK)
    {
        if(dev->data_len == 0)
        {
            return -EAGAIN;
        }
    }
    wait_event_interruptible(read_queue, dev->data_len > 0);

    if(count > dev->data_len)
    {
        count = dev->data_len;
    }
    
    if(copy_to_user(buf, dev->buf, count))
    {
        return -EFAULT;
    }
    memmove(dev->buf,
            dev->buf + count,
            dev->data_len - count);

    dev->data_len -= count;
    return count;
}


static ssize_t char_demo_write(struct file *filp,
                               const char __user *buf,
                               size_t count,
                               loff_t *f_pos)
{
    struct char_demo_dev *dev = filp->private_data;
    size_t space;
    if(count == 0)
    {
        return 0;
    }
    space = BUF_SIZE - dev->data_len;
    if(space == 0)
    {
        return -ENOSPC;
    }

    if(count > space)
    {
        count = space;
    }

    if(copy_from_user(dev->buf + dev->data_len, buf, count))
    {
        return -EFAULT;
    }
    dev->data_len += count;
    wake_up_interruptible(&read_queue);

    return count;
}

static __poll_t char_demo_poll(struct file *filp, struct poll_table_struct *wait)
{
    __poll_t mask;
    struct char_demo_dev *dev = filp->private_data;
    poll_wait(filp, &read_queue, wait);
    if(dev->data_len > 0)
    {
        mask |= POLLIN;
        return mask;
    }
    return mask;
}

/* ============================================================
 * 4. file_operations
 * ============================================================ */

static const struct file_operations char_demo_fops = {
    .owner      = THIS_MODULE,
    .open       = char_demo_open,
    .release    = char_demo_release,
    .read       = char_demo_read,
    .write      = char_demo_write,
    .poll       = char_demo_poll,
};


/* ============================================================
 * 5. 注册单个字符设备
 * ============================================================ */

static int char_demo_device_register(struct char_demo_dev *dev,
                                     int index)
{
    int ret;

    dev->devt = char_drv.base_devt + index;

    /* 初始化设备自己的状态 */
    dev->data_len = 0;

    cdev_init(&dev->cdev, &char_demo_fops);

    dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev->cdev,
                   dev->devt,
                   1);
    if (ret) {
        pr_err("%s: cdev_add failed, index=%d, ret=%d\n",
               DRIVER_NAME,
               index,
               ret);

        return ret;
    }

    /*
     * 创建 /sys/class/char_demo/dev_charX
     *
     * 用户空间的设备管理程序通常会根据这个信息
     * 创建：
     *
     * /dev/dev_char0
     * /dev/dev_char1
     */
    dev->device = device_create(char_drv.class,
                                NULL,
                                dev->devt,
                                NULL,
                                DEVICE_NAME_BASE "%d",
                                index);

    if (IS_ERR(dev->device)) {
        ret = PTR_ERR(dev->device);

        pr_err("%s: device_create failed, index=%d, ret=%d\n",
               DRIVER_NAME,
               index,
               ret);

        dev->device = NULL;

        /*
         * device_create 失败，
         * 前面的 cdev_add 已经成功，
         * 所以需要回滚。
         */
        cdev_del(&dev->cdev);

        return ret;
    }

    pr_info("%s: registered /dev/%s%d, major=%d, minor=%d\n",
            DRIVER_NAME,
            DEVICE_NAME_BASE,
            index,
            MAJOR(dev->devt),
            MINOR(dev->devt));

    return 0;
}


/* ============================================================
 * 6. 注销单个字符设备
 * ============================================================ */

static void char_demo_device_unregister(struct char_demo_dev *dev)
{
    /*
     * 与 device_create 对应。
     */
    if (dev->device) {
        device_destroy(char_drv.class,
                       dev->devt);

        dev->device = NULL;
    }

    /*
     * 与 cdev_add 对应。
     */
    cdev_del(&dev->cdev);
}


/* ============================================================
 * 7. 模块初始化
 * ============================================================ */

static int __init char_demo_init(void)
{
    int ret;
    int i;
    init_waitqueue_head(&read_queue);
    ret = alloc_chrdev_region(&char_drv.base_devt,
                              0,
                              DEVICE_NUM,
                              DRIVER_NAME);
    if (ret) {
        pr_err("%s: alloc_chrdev_region failed: %d\n",
               DRIVER_NAME,
               ret);

        return ret;
    }

    pr_info("%s: allocated major=%d, minor=%d ~ %d\n",
            DRIVER_NAME,
            MAJOR(char_drv.base_devt),
            MINOR(char_drv.base_devt),
            MINOR(char_drv.base_devt) + DEVICE_NUM - 1);

    /*
        创建设备号之后可以直接创建设备类
     */
    char_drv.class = class_create(THIS_MODULE,
                                  CLASS_NAME);

    if (IS_ERR(char_drv.class)) {
        ret = PTR_ERR(char_drv.class);

        pr_err("%s: class_create failed: %d\n",
               DRIVER_NAME,
               ret);

        char_drv.class = NULL;

        goto err_unregister_region;
    }

    /*
     * 注册每一个具体设备。
     */
    for (i = 0; i < DEVICE_NUM; i++) {

        ret = char_demo_device_register(
                    &char_drv.devices[i],
                    i);

        if (ret)
            goto err_unregister_devices;
    }

    pr_info("%s: module loaded successfully\n",
            DRIVER_NAME);

    return 0;
/*
 * 如果 devices[i] 注册失败，
 * devices[0] ~ devices[i - 1]
 * 已经成功，需要释放。
 */
err_unregister_devices:

    while (--i >= 0)
        char_demo_device_unregister(
                &char_drv.devices[i]);

    class_destroy(char_drv.class);
    char_drv.class = NULL;


err_unregister_region:

    unregister_chrdev_region(char_drv.base_devt,
                             DEVICE_NUM);

    return ret;
}


/* ============================================================
 * 8. 模块退出
 * ============================================================ */

static void __exit char_demo_exit(void)
{
    int i;

    /*
     * 注销所有设备。
     */
    for (i = 0; i < DEVICE_NUM; i++)
        char_demo_device_unregister(
                &char_drv.devices[i]);

    /*
     * 销毁 class。
     */
    class_destroy(char_drv.class);

    /*
     * 释放设备号。
     */
    unregister_chrdev_region(char_drv.base_devt,
                             DEVICE_NUM);

    pr_info("%s: module unloaded\n",
            DRIVER_NAME);
}


/* ============================================================
 * 9. 模块信息
 * ============================================================ */

module_init(char_demo_init);
module_exit(char_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("heisenberg");
MODULE_DESCRIPTION("Character device template");
MODULE_VERSION("1.0");