#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/uaccess.h> 
#include <linux/cdev.h> 
#include <linux/mutex.h> 

#define DEVICE_NAME "derchar"
#define BUF_SIZE 256

static dev_t devno;
static struct cdev toy_cdev;
static DEFINE_MUTEX(toy_lock);

static char kernel_buf[BUF_SIZE];
static size_t buf_len;

static int toy_open(struct inode *inode, struct file *file) 
{
    pr_info("derchar: device opened\n");
    return 0;
}

static ssize_t toy_read(struct file *file, 
                        char __user *user_buf,
                        size_t count, 
                        loff_t *ppos)
{
    ssize_t ret;

    if (mutex_lock_interruptible(&toy_lock)) 
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
    pr_info("derchar: %zu bytes\n", ret);
    mutex_unlock(&toy_lock);
    return ret; 
}

static ssize_t toy_write(struct file *file, 
                         const char __user *user_buf,
                         size_t count, 
                         loff_t *ppos)
{
    ssize_t ret;
    
    if (mutex_lock_interruptible(&toy_lock))
        return -ERESTARTSYS;

    if (count > BUF_SIZE)
        count = BUF_SIZE;

    if (copy_from_user(kernel_buf, user_buf, count)) {
        ret = -EFAULT;
        goto out;
    }

    buf_len = count;
    *ppos = count;

    ret = count;

out:
    pr_info("derchar: wrote %zu bytes\n", count);
    mutex_unlock(&toy_lock);
    return ret; 
}

static const struct file_operations toy_fops = {
    .owner = THIS_MODULE,
    .open = toy_open,
    .read = toy_read,
    .write = toy_write,
};

static int __init toy_init(void) 
{
    int ret; 
    ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&toy_cdev, &toy_fops);

    ret = cdev_add(&toy_cdev, devno, 1);
    if (ret < 0) {
        unregister_chrdev_region(devno, 1);
        return ret;
    }

    pr_info("derchar: registered with major %d minor %d\n",
        MAJOR(devno), MINOR(devno));
    return 0;
}

static void __exit toy_exit(void)
{
    cdev_del(&toy_cdev);
    unregister_chrdev_region(devno, 1);
    pr_info("derchar: unloaded\n");
}

module_init(toy_init);
module_exit(toy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Niyaz");
MODULE_DESCRIPTION("Toy character device driver");
