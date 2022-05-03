#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/timer.h>
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */ 
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>  /* invalidate_bdev */
#include <linux/bio.h>
MODULE_LICENSE("GPL");

static int vmem_disk_major = 0;
module_param(vmem_disk_major, int, 0);

static int hardsect_size = 512;
module_param(hardsect_size, int, 0);

static int nsectors = 1024;
module_param(nsectors, int, 0);

static int ndevices = 4;
module_param(ndevices, int, 0);

enum{
	RM_SIMPLE = 0,
	RM_FULL = 1,
	RM_NOQUEUE = 2,
};

static int request_mode = RM_NOQUEUE;
module_param(request_mode, int, 0);
#define vmem_disk_MINORS 16
#define MINOR_SHIFT 4
#define DEVNUM(kdevnum) (MINOR(kdev_t_to_nr(kdevnum)) >> MINOR_SHIFT
#define KERNEL_SECTOR_SIZE 512
#define INVALIDATE_DELAY 30*HZ

struct vmem_disk_dev {
	int size;
	u8 *data;
	short users;
	short media_change;
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
};

struct vmem_disk_dev *Devices = NULL;

static void vmem_disk_transfer(struct vmem_disk_dev *dev, unsigned long sector,
	 unsigned long nsect, char *buffer, int write)
{
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;
	printk("offset %ld, nbytes %ld, buffer %s\n", offset, nbytes, buffer);
	if((offset + nbytes) > dev->size)
	{
		printk("Beyond-endwrite.\n");
		return;
	}
	if(write)
	{
		//printk("write");
		//copy_from_user(dev->data+offset, buffer, nbytes);
		memcpy(dev->data + offset, buffer, nbytes);
	}
	else
	{
		//printk("read\n");
		//copy_to_user(buffer, dev->data+offset, nbytes);
		memcpy(buffer, dev->data+offset, nbytes);
	}

}

static int vmem_disk_xfer_bio(struct vmem_disk_dev *dev, struct bio *bio)
{
	struct bio_vec bvec;
	struct bvec_iter iter;
	sector_t sector = bio->bi_iter.bi_sector;
	bio_for_each_segment(bvec, bio, iter)
	{
		char *buffer = __bio_kmap_atomic(bio,iter);
		vmem_disk_transfer(dev, sector, bio_cur_bytes(bio)>>9,
			buffer, bio_data_dir(bio) == WRITE);
			sector += bio_cur_bytes(bio) >> 9;
			__bio_kunmap_atomic(buffer);
	}
	return 0;
}

static unsigned int vmem_disk_make_request(struct request_queue *q, struct bio *bio)
{
	struct vmem_disk_dev *dev = q->queuedata;
	int status;
	status = vmem_disk_xfer_bio(dev, bio);
	bio_endio(bio);
	return status;  
}

static void vmem_disk_request(struct request_queue *q)
{
	struct request *req;
	struct bio* bio;
	while ((req = blk_peek_request(q))!= NULL)
	{
		struct vmem_disk_dev *dev = req->rq_disk->private_data;
		if (req->cmd_flags != 1)
		{
			printk("Skip non-fs request\n");
			blk_start_request(req);
			__blk_end_request_all(req, -EIO);
			continue;
		}
		blk_start_request(req);
		__rq_for_each_bio(bio, req)
			vmem_disk_xfer_bio(dev, bio);
		__blk_end_request_all(req, 0);
	}
}

static int vmem_disk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	long size;
	struct vmem_disk_dev *dev = bdev->bd_disk->private_data;

	size = dev->size*(hardsect_size/KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 4;
	pirntk("vmemdisk getgeo \n");
	return 0;
}
static struct block_device_operations vmem_disk_ops = {
	.owner = THIS_MODULE,
	.getgeo = vmem_disk_getgeo, 
};

static void setup_device( struct vmem_disk_dev *dev, int which)
{
	memset(dev, 0, sizeof(struct vmem_disk_dev));
	dev->size = nsectors*hardsect_size;
	dev->data = vmalloc(dev->size);
	printk("vmalloc for data\n");
	if(dev->data == NULL)
	{
		printk("vmalloc fail.\n");
		return;
	}

	spin_lock_init(&dev->lock);
	switch (request_mode) {
		case RM_NOQUEUE:
			//dev->queue = blk_init_queue(vmem_disk_request, &dev->lock);
			dev->queue = blk_alloc_queue(GFP_KERNEL);
			if(dev->queue == NULL)
				goto out_vfree;
			blk_queue_make_request(dev->queue, vmem_disk_make_request);
			printk("NOQUEUE mode\n");
			break;
		default :
			printk("default\n");
	}
	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;
	dev->gd = alloc_disk(which*vmem_disk_MINORS); // para is minor
	if(! dev->gd)
	{
		printk("alloc_disk fial.\n");
		goto out_vfree;
	}
	dev->gd->major = vmem_disk_major;
	dev->gd->first_minor = which*vmem_disk_MINORS;
	dev->gd->fops = &vmem_disk_ops;
	dev->gd->private_data = dev;
	dev->gd->queue = dev->queue;
	snprintf(dev->gd->disk_name, 32, "vmem_disk%c", which+'a');
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	add_disk(dev->gd);

	return;

out_vfree:
	if(dev->data)
		vfree(dev->data);
}

static int __init vmem_disk_init(void)
{
	int i;
	vmem_disk_major = register_blkdev(vmem_disk_major, "vmem_disk");
	if (vmem_disk_major <= 0 )
	{
		printk("vmem_disk: unable to get major\n");
		return -EBUSY;
	}

	Devices = kmalloc(ndevices*sizeof (struct vmem_disk_dev), GFP_KERNEL);
	if(Devices == NULL)
		goto out_unregister;
	for(i=0; i< ndevices; i++)
		setup_device(Devices+i, i);
	return 0;
	
out_unregister:
	unregister_blkdev(vmem_disk_major, "vmem_disk");
	return -ENOMEM;
}
void __exit vmem_disk_exit(void)
{
	int i;

	for (i = 0; i < ndevices; i++) {
		struct vmem_disk_dev *dev = Devices + i;
		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if (dev->queue) {
			if (request_mode == RM_NOQUEUE)
				kobject_put (&dev->queue->kobj);
			/* blk_put_queue() is no longer an exported symbol */
			else
				blk_cleanup_queue(dev->queue);
		}
		if (dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(vmem_disk_major, "vmem_disk");
	kfree(Devices);
}
module_init(vmem_disk_init);
module_exit(vmem_disk_exit);
