#include <linux/module.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/etherdevice.h>
#include <linux/semaphore.h>
#include <linux/poll.h>


MODULE_LICENSE("GPL");

struct virnet {
    struct net_device *ndev;
    struct sk_buff* sg_frame;
    struct proc_dir_entry *proc;
    struct proc_dir_entry *proc_uio;
    struct semaphore sem;
    wait_queue_head_t wq;
};

struct virnet *vnet;
static int virnet_send_packet( struct sk_buff *skb, struct net_device *dev)
{
    netif_stop_queue(vnet->ndev); //why not dev

    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;
    vnet->sg_frame = skb;
    up(&vnet->sem);
    wake_up(&vnet->wq); 
	return 0;
}
static struct net_device_ops sg_ops = 
{
    .ndo_start_xmit = virnet_send_packet,
#ifdef T_X86_517
	.ndo_set_mac_address    = eth_mac_addr,
#endif
};
static ssize_t eth_uart_write(struct file* file, const char* buf, \
    size_t count, loff_t* offset)
{
    int ret;
    struct sk_buff* skb = dev_alloc_skb(count +2);
    if (skb == NULL)
    {
        printk("fail t alloc skb\n");
        return -ENOMEM;
    }

    skb_reserve(skb, 2);
    ret = copy_from_user(skb_put(skb, count), buf, count);
     if (ret < 0)
    {
        printk("failed to copy \n");
    }

    skb->dev = vnet->ndev;
    skb->protocol = eth_type_trans(skb, vnet->ndev);
    skb->ip_summed = CHECKSUM_NONE;
    vnet->ndev->stats.rx_packets++;
    vnet->ndev->stats.rx_bytes += skb->len;

    netif_rx(skb);
    return count;
}

static ssize_t eth_uart_read(struct file* file, char* buf, size_t count, \
    loff_t* offset)
{
    int len, ret;
    if (file->f_flags & O_NONBLOCK)
    {
        if (down_trylock(&vnet->sem) != 0)
            return -EAGAIN;
    }else
    {
        if( down_interruptible(&vnet->sem) != 0)
        {
            printk("down interrupted...\n");
            return -EINTR;
        }
    }
  
    len = vnet->sg_frame->len;
    if (count < len)
    {
        up(&vnet->sem);
        printk("no enough to read \n");
        return -EFBIG;
    }
    ret = copy_to_user(buf, vnet->sg_frame->data, len);
    if (ret < 0)
    {
        printk("failed to copy \n");
    }

    dev_kfree_skb(vnet->sg_frame);
    vnet->sg_frame = 0;
    
    netif_wake_queue(vnet->ndev);
    return len;
}
static unsigned int  eth_uart_poll(struct file* file, struct poll_table_struct* queue)
{
    unsigned int mask;
    poll_wait(file, &vnet->wq, queue);

    mask = POLLOUT | POLLWRNORM;
    if (vnet->sg_frame != 0)
        mask |= POLLIN | POLLRDNORM;
    return mask;
}

#ifdef T_ARM
static const struct file_operations vnet_ops = 
{
    .read = eth_uart_read,
    .poll = eth_uart_poll,
    .write = eth_uart_write,
};
#else 
static const struct proc_ops vnet_ops = 
{
    .proc_read = eth_uart_read,
    .proc_poll = eth_uart_poll,
    .proc_write = eth_uart_write,
};
#endif
static int virnet_init(void)
{
    int ret = 0;
    struct net_device *ndev;
#ifdef T_X86_517
	u8 addr[ETH_ALEN];
#endif
    ndev = alloc_netdev(sizeof(struct virnet), "virnet%d", NET_NAME_UNKNOWN, ether_setup);
    if (ndev == NULL)
    {
        printk("fail to alloc net_device\n");
        ret = -EEXIST;
    }
    ndev->netdev_ops = &sg_ops;
#ifdef T_X86_517
	addr[0] = 0x00;
    addr[1] = 0x01;
    addr[2] = 0x00;
    addr[3] = 0x01;
    addr[4] = 0x0a;
    addr[5] = 0x01;
    eth_hw_addr_set(ndev, addr);
	printk("x86_517\n");
#else 
    memcpy(ndev->dev_addr, "\0ABCD0", ETH_ALEN);
    get_random_bytes((char*)ndev->dev_addr + 3, 3);
#endif
	//printk("ndev addr %hhn", ndev->dev_addr);
    ret = register_netdev(ndev); 
    if (ret != 0)
    {
        goto fail_reg;
    }
    vnet = netdev_priv(ndev);
    vnet->ndev = ndev;
    vnet->proc = proc_mkdir("eth_uart", 0);
    if (vnet->proc == NULL)
    {
        printk("fail to mkdir\n");
        goto fail_mkdir;
    }
    vnet->proc_uio = proc_create("uio", 0666, vnet->proc, &vnet_ops);
    if (vnet->proc_uio == NULL)
    {
        printk("fail to create uio\n");
        goto fail_create;
    }
    
    sema_init(&vnet->sem, 0);
    init_waitqueue_head(&vnet->wq);
    return 0;
fail_create: remove_proc_entry("eth_uart", 0);
fail_mkdir:
fail_reg:
    free_netdev(ndev);
    unregister_netdev(ndev); 
    return ret;
}
static void virnet_exit(void)
{
    remove_proc_entry("uio", vnet->proc);
    remove_proc_entry("eth_uart", 0);
    unregister_netdev(vnet->ndev);
    if(vnet->sg_frame != NULL)
        kfree(vnet->sg_frame);
    free_netdev(vnet->ndev);
}
module_init(virnet_init);
module_exit(virnet_exit);
