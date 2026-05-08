#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/uaccess.h> 
#include <linux/cdev.h> 
#include <linux/mutex.h> 

#define DEVICE_NAME "derchar"
#define BUF_SIZE 256

static dev_t devno;
static struct cdev cdev;
static DEFINE_MUTEX(cdev_lock);

static char kernel_buf[BUF_SIZE];
static size_t buf_len;

static int cdev_open(struct inode *inode, struct file *file) 
{
    pr_info("derchar: device opened\n");

    pr_info("derchar: file->f_pos: %lld\n", file->f_pos); 
    pr_info("derchar: file->f_mode: 0x%x\n", file->f_mode); 
    pr_info("derchar: file->f_flags: 0x%x\n", file->f_flags); 

    return 0;
}

static int cdev_release(struct inode *inode, struct file *file)
{
    pr_info("derchar: device closed\n");
    return 0;
}

static ssize_t cdev_read(struct file *file, char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    ssize_t ret;

    if (mutex_lock_interruptible(&cdev_lock)) 
        return -ERESTARTSYS;

    if (*ppos >= buf_len) {
        ret = 0;
        goto out;
    }

    if (count > buf_len - *ppos)
        count = buf_len - *ppos;

    if (copy_to_user(user_buf, kernel_buf + *ppos, count)) {
        ret = -EFAULT;
        goto out;
    }

    *ppos += count;
    ret = count;

out: 
    pr_info("derchar: %zd bytes\n", ret);
    mutex_unlock(&cdev_lock);
    return ret; 
}

static ssize_t cdev_write(struct file *file, const char __user *user_buf,
                          size_t count, loff_t *ppos)
{
    ssize_t ret;
    
    if (mutex_lock_interruptible(&cdev_lock))
        return -ERESTARTSYS;

    if (*ppos >= BUF_SIZE) {
        ret = -ENOSPC; 
        goto out;
    }

    if (count > BUF_SIZE - *ppos)
        count = BUF_SIZE - *ppos;

    if (copy_from_user(kernel_buf, user_buf, count)) {
        ret = -EFAULT;
        goto out;
    }

    *ppos += count; 

    if (*ppos > buf_len) 
        buf_len = *ppos;

    ret = count;

out:
    pr_info("derchar: wrote %zu bytes\n", count);
    mutex_unlock(&cdev_lock);
    return ret; 
}

static const struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .open = cdev_open,
    .release = cdev_release,
    .read = cdev_read,
    .write = cdev_write,
};

static int __init cdev_initialise(void) 
{
    int ret; 
    ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&cdev, &cdev_fops);

    ret = cdev_add(&cdev, devno, 1);
    if (ret < 0) {
        unregister_chrdev_region(devno, 1);
        return ret;
    }

    pr_info("derchar: registered with major %d minor %d\n",
        MAJOR(devno), MINOR(devno));

    return 0;
}

static void __exit cdev_exit(void)
{
    cdev_del(&cdev);
    unregister_chrdev_region(devno, 1);
    pr_info("derchar: unloaded\n");
}

module_init(cdev_initialise);
module_exit(cdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Niyaz");
MODULE_DESCRIPTION("character device driver");
