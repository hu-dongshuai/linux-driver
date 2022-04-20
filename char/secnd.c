#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/slab.h>


#define SECND_MAJOR 238


static int secnd_major = SECND_MAJOR;

struct secnd_dev {
    struct cdev cdev;
    atomic_t counter;
    struct timer_list timer;
};

struct secnd_dev *secnd_devp;

static void secnod_timer_handle(struct timer_list *timer)
{
    mod_timer(&secnd_devp->timer, jiffies + HZ);
    atomic_inc(&secnd_devp->counter);

    printk("current jiffies is %ld\n", jiffies);
}
static int secnd_open(struct inode *node, struct file *fop)
{
    timer_setup(&secnd_devp->timer, secnod_timer_handle, 0);
    //secnd_devp->timer.function = &secnod_timer_handle;
    secnd_devp->timer.expires = jiffies + HZ;

    add_timer(&secnd_devp->timer);
    atomic_set(&secnd_devp->counter, 0);
    return 0;
}
static ssize_t secnd_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int counter;
    counter = atomic_read(&secnd_devp->counter);
    if(put_user(counter, (int*)buf))
    {
        return -EFAULT;
    }
    else
    {
        return sizeof(unsigned int);
    }

}
static int secnd_close(struct inode *node, struct file *fop)
{
    del_timer(&secnd_devp->timer);
    return 0;
}
static const struct file_operations secnd_fops = 
{
    .owner = THIS_MODULE,
    .open = secnd_open,
    .read = secnd_read,
    .release = secnd_close,
};

void secnd_dev_setup(struct secnd_dev *dev, int index)
{
    int err, devo = MKDEV(secnd_major, index);
    cdev_init(&dev->cdev, &secnd_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devo, 1);
    if(err < 0 )
    {
        printk("Fail to add\n");
    }
}

static int secnd_init(void)
{
    dev_t devo = MKDEV(secnd_major, 0);
    int ret;


    if(secnd_major)
    {
        ret = register_chrdev_region(devo, 1, "secnd");
    } else {
        ret = alloc_chrdev_region(&devo, -1, 1, "secnd");
        secnd_major = MAJOR(devo);
    }

    if(ret < 0)
        return ret;
    
    secnd_devp = kzalloc(sizeof(struct secnd_dev), GFP_KERNEL);

    if(secnd_devp == NULL)
    {
        goto fail_malloc;
    }
    
    memset(secnd_devp, 0, sizeof(struct secnd_dev));
    secnd_dev_setup(secnd_devp, 1);

fail_malloc:
    unregister_chrdev_region(MKDEV(secnd_major,0), 1);
    return ret;
}


void __exit secnd_exit(void)
{
    cdev_del(&secnd_devp->cdev);
    kfree(secnd_devp);
    unregister_chrdev_region(MKDEV(secnd_major,0), 1);
}

module_init(secnd_init);
module_exit(secnd_exit);
MODULE_LICENSE("GPL");