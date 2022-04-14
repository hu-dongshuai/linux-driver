#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/wait.h>

#define GLOBALMEM_SIZE 0x1000                                                                                                                                                                               
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230

static int globalmem_major = GLOBALMEM_MAJOR;
//module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev {
    struct cdev cdev;
    unsigned int current_len;
    unsigned char mem[GLOBALMEM_SIZE];
    struct semaphore sem;
    wait_queue_head_t r_wait;
    wait_queue_head_t w_wait;
};

struct globalmem_dev *globalmem_devp;

int globalmem_open(struct inode *inode, struct file *filp)
{
    filp->private_data = globalmem_devp;
    return 0;
}

int globalmem_release(struct inode *inode, struct file *filp)
{
    return 0;
}
static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    struct globalmem_dev *dev = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);
    down(&dev->sem);
    add_wait_queue(&dev->r_wait,&wait);

    while (dev->current_len == 0) {
        if (filp->f_flags & O_NONBLOCK)
        {
            ret = - EAGAIN;
            goto out;
        }
        __set_current_state(TASK_INTERRUPTIBLE);

        up(&dev->sem);
        printk("schedule\n");
        schedule();

        printk("juedge pending\n");
        if(signal_pending(current))
        {
            printk("wake up by signal\n");
            ret = -ERESTARTSYS;
            goto out2;
        }

        down(&dev->sem);
    }

    if (count > dev->current_len)
        count = dev->current_len;
    if (copy_to_user(buf, dev->mem, count))
    {
        ret = - EFAULT;
        goto out;
    }else{
        dev->current_len -= count;
        wake_up_interruptible(&dev->w_wait);
        ret = count;
    }

out:up(&dev->sem);
out2:remove_wait_queue(&dev->w_wait, &wait);
     set_current_state(TASK_RUNNING);
    return ret;
}
static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    struct globalmem_dev *dev = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);
    down(&dev->sem);
    add_wait_queue(&dev->w_wait, &wait);

    while (dev->current_len == GLOBALMEM_SIZE){
        if (filp->f_flags &O_NONBLOCK){
            ret = -EAGAIN;
            goto out;
        }
        __set_current_state(TASK_INTERRUPTIBLE);
        up(&dev->sem);
        
        schedule();
        if (signal_pending(current)) {
            ret = - ERESTARTSYS;
            goto out2;
        }
        down(&dev->sem);
    }

    if (count > GLOBALMEM_SIZE - dev->current_len)
        count = GLOBALMEM_SIZE - dev->current_len;

    if (copy_from_user(dev->mem + dev->current_len, buf, count))
    {
        ret = - EFAULT;
        goto out;
    } else {
        dev->current_len += count;
        wake_up_interruptible(&dev->r_wait);
        ret = count;
    }
out:up(&dev->sem);
out2:remove_wait_queue(&dev->w_wait, &wait);
     set_current_state(TASK_RUNNING);
return ret;
}




static const struct file_operations globalmem_fops = {
    .owner = THIS_MODULE,
//  .llseek = globalmem_llseek,
    .read = globalmem_read,
    .write = globalmem_write,
//  .unlocker_ioctl = globalmem_ioctl,
    .open = globalmem_open,
    .release = globalmem_release,

};


static void globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
    int err,devno = MKDEV(globalmem_major, index);

    cdev_init(&dev->cdev, &globalmem_fops) ;
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding globalmem %d", err, index);
}
static int __init globalmem_init(void)
{
    int ret;
    dev_t devno = MKDEV(globalmem_major, 0);/*get dev_t by major and minor device number*/

    if (globalmem_major)
        ret = register_chrdev_region(devno, 1, "globalmem");
    else {
        ret = alloc_chrdev_region(&devno, -1, 1, "globalmem");
        globalmem_major = MAJOR(devno);
    }

    if (ret < 0)
        return ret;

    globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
    if(!globalmem_devp) {
        ret = ENOMEM;
        goto fail_malloc;
    }
    memset(globalmem_devp, 0, sizeof(struct globalmem_dev));
    globalmem_setup_cdev(globalmem_devp, 0);
    sema_init(&globalmem_devp->sem,1);
    init_waitqueue_head(&globalmem_devp->r_wait);
    init_waitqueue_head(&globalmem_devp->w_wait);
    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}


void globalmem_exit(void)
{
    cdev_del(&globalmem_devp->cdev);
    kfree(globalmem_devp);
    unregister_chrdev_region(MKDEV(globalmem_major, 0),1);
}
MODULE_LICENSE("GPL");
module_param(globalmem_major, int, S_IRUGO);
module_init(globalmem_init);
module_exit(globalmem_exit);