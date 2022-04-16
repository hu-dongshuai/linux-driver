#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/wait.h>
#include <linux/init.h>
#define TEST_MAJOR 230
#define TEST_SIZE 100

static int test_major = TEST_MAJOR;

struct test_dev {
	struct cdev cdev;
	char mem[TEST_SIZE];
	int current_len;
	struct semaphore sem;
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
	struct fasync_struct *async_queue;
};

struct test_dev *test_devp;

static int test_open(struct inode *node, struct file *fop)
{
	struct test_dev *dev = container_of(node->i_cdev, struct test_dev, cdev);
	
	fop->private_data = dev;
	return 0;
}

static ssize_t test_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;
	struct test_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(wait, current);
	
	if (p >= TEST_SIZE)
		return 0;
	if (count > TEST_SIZE - p)
		count = TEST_SIZE - p;
    if (down_interruptible(&dev->sem))
	{
		return ERESTARTSYS;
	}
	add_wait_queue(&dev->w_wait, &wait);
	while(dev->current_len == TEST_SIZE)
	{
		if(filp->f_flags & O_NONBLOCK)
		{
			ret = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		up(&dev->sem);
		schedule();
		if(signal_pending(current))
		{
			ret = -ERESTART;
			goto out2;
		}
		down(&dev->sem);
	}
	if(copy_from_user(dev->mem+p, buf, count))
	{ 
		ret = -EFAULT;
	}
	else 
	{
		dev->current_len += count;
		ret = count;
		wake_up_interruptible(&dev->r_wait);
		if(dev->async_queue)
		{
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
		}
	}

	 up(&dev->sem);
out: up(&dev->sem);
out2: remove_wait_queue(&dev->w_wait, &wait);
	return ret;
}

static ssize_t test_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;
    struct test_dev *dev = filp->private_data;

	DECLARE_WAITQUEUE(wait, current);

    if (down_interruptible(&dev->sem))
	{
		return ERESTARTSYS;
	}
	add_wait_queue(&dev->r_wait, &wait);

	while(dev->current_len == 0)
	{
		if(filp->f_flags & O_NONBLOCK)
		{
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		up(&dev->sem);
		schedule();

		//if wakened by signal ???
		if(signal_pending(current))
		{
			ret = -ERESTARTSYS;
			goto out2;
		}
		down(&dev->sem);
	}
	if(count > dev->current_len)
		count = dev->current_len;

	if (copy_to_user(buf, dev->mem + p, count))
	{
		ret = -EFAULT;
	} else 
	{
		dev->current_len -= count;
		wake_up_interruptible(&dev->w_wait);
		ret = count;
	}
	up(&dev->sem);

out:up(&dev->sem);
out2:remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;
}

static int test_fasync(int fd, struct file *filp, int mode)
{
	struct test_dev *dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}
static int test_close (struct inode *node, struct file *filp)
{
	test_fasync(-1, filp, 0);
	return 0;
}
static const struct file_operations test_fops = {
	.owner = THIS_MODULE,
	.open = test_open,
	.write = test_write,
	.read = test_read,
	.fasync = test_fasync,
	.release = test_close,
};

static void test_dev_setup(struct test_dev *dev, int index)
{
	int err, devno = MKDEV(test_major, index);
	cdev_init(&dev->cdev, &test_fops);
	dev->cdev.owner = THIS_MODULE;	
	err = cdev_add(&dev->cdev, devno, 1);
	if(err < 0)
	{
		printk("fail to add test dev\n");
	}
}

static int __init test_init(void)
{
	int ret, i;
	dev_t devo = MKDEV(test_major, 0);
    if(test_major)
	{
		ret = register_chrdev_region(devo, 2, "test");
	} else
	{
		ret = alloc_chrdev_region(&devo, -1, 2, "test");
		test_major = MAJOR(devo);
		printk("alloc major number %d", test_major);
	}

	if(ret < 0)
		return ret;
	
	test_devp = kzalloc(2*sizeof(struct test_dev), GFP_KERNEL);

	if(!test_devp)
	{
		goto fail_malloc;
	}
	memset(test_devp, 0, 2*sizeof(struct test_dev));

	for(i=0; i<2; i++)
	{
		test_dev_setup(&test_devp[i], i);
		sema_init(&(test_devp[i].sem), 1);
		init_waitqueue_head(&(test_devp[i].r_wait));
		init_waitqueue_head(&(test_devp[i].w_wait));
	}
	
fail_malloc:
	unregister_chrdev_region(MKDEV(test_major, 0), 1);
	return 0;
}

void __exit test_exit(void)
{
	cdev_del(&(test_devp[0].cdev));
	cdev_del(&(test_devp[1].cdev));
	kfree(test_devp);
	unregister_chrdev_region(MKDEV(test_major, 0), 2);
}

MODULE_LICENSE("GPL");
module_init(test_init);
module_exit(test_exit);