#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kdev_t.h>
#include <asm/errno.h>

#include "generator.h"
#include "finite_fields.h"

/*
 * insmod chardriver.ko
 * ...
 * fopen(chardev...);
 * write(/dev/chardev, k, a_0, ... , a_k-1, x_0, ... x_k-1, c) - инициализировали начальные значения, теперь можно использовать
 * read(/dev/chardev)
 * ...
 * fclose(/dev/chardev)
 * ...
 * rmmod chardriver
 */


static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"
enum {
	CDEV_NOT_USED = 0,
	CDEV_EXCLUSIVE_OPEN = 1,
};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

struct class *cls;

static struct file_operations chardev_fops = {
    .owner = THIS_MODULE, /* https://stackoverflow.com/questions/1741415/linux-kernel-modules-when-to-use-try-module-get-module-put*/
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

static struct cdev my_cdev;
static dev_t dev_num;

static int __init register_module(void)
{
    if(alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0)
        goto fail;

    cdev_init(&my_cdev, &chardev_fops);
    my_cdev.owner = THIS_MODULE;
    my_cdev.ops = &chardev_fops;

    if(cdev_add(&my_cdev, dev_num, 1) < 0)
        goto unregister;

    pr_info("I was assigned major number %d and minor number %d. \n", MAJOR(dev_num), MINOR(dev_num));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
    device_create(cls, NULL, dev_num, NULL, DEVICE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    return SUCCESS;

unregister:
    unregister_chrdev_region(dev_num, 1);
fail:
    pr_alert("Registering char device failed");
    return -1;
}

/* вызывается при rmmod */
static void __exit unregister_module(void)
{
    device_destroy(cls, dev_num);
    class_destroy(cls);
    cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);
    pr_info("removed module\n");
}


static int device_open(struct inode *inode, struct file *file)
{
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    struct generator *gen = (struct generator*) kzalloc(sizeof(struct generator), GFP_KERNEL);

    if(gen == NULL || setup_generator(gen) < 0) {
        return -ENOMEM;
    }

    file->private_data = gen;

    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file)
{
    struct generator *gen = (struct generator *) file->private_data;
    free_generator(gen);
	atomic_set(&already_open, CDEV_NOT_USED);
	return SUCCESS;
}


static ssize_t device_read(struct file *file, /* include linux/fs.h */
			   char __user *buffer, /* buffer to fill with data*/
			   size_t length, /* length of the buffer */
			   loff_t *offset)
{
    struct generator *gen = (struct generator *) file->private_data;;
	int bytes_read = 0;
    for(size_t n = 0; n < length; n++){
        /* пишем в пользовательский буфер */
        uint8_t target;
        if(get_random(gen, &target) < 0){
            return -1;
        }
        put_user(target, buffer++);
        bytes_read++;
    }

	return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *buff,
			    size_t len, loff_t *off)
{
	struct generator *gen = (struct generator *) file->private_data;
    if(gen == NULL || init_random(gen, buff, len) < 0){
        return -1;
    }
    return len;
}

module_init(register_module);
module_exit(unregister_module);

MODULE_LICENSE("GPL");
