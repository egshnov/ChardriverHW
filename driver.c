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

#include <asm/errno.h>
#include "finite_fields.h"

/* прототипы методов интерфейса */

/* открывает файл устройства (если ещё не), инкрементирует open count */
static int device_open(struct inode *, struct file *);

/* вызывается когда процесс закрывает файл */
static int device_release(struct inode *, struct file *);

/* вызывается когдапроцесс читает из файла т.е при  read(dev/chardev...) */
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);

/* вызывается при записи в файл устройства т.е. при write(dev/chardev...) */
static ssize_t device_write(struct file*, const char __user *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 80/* максимальная длина сообщения от драйвера */

static int major; /* major number драйвера */

/* константы отображающие статус доступности девайса */
enum {
	CDEV_NOT_USED = 0,
	CDEV_EXCLUSIVE_OPEN = 1,
};

/* нужен для монопольного досупа
 * нет необходимости обращаться к already_open из другого .c файла поэтому static */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN + 1];

/* нужен чтобы модуль был доступен из user space и добавлен в /dev /proс и т.д. при insmod
 * и не пришлось делать ручками mknod */
static struct class *cls;

static struct file_operations chardev_fops = {
    .owner = THIS_MODULE, /* https://stackoverflow.com/questions/1741415/linux-kernel-modules-when-to-use-try-module-get-module-put*/
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};


/* вызывается при insmod __init - подсказка компилятору */
static int __init chardev_init(void)
{
	major = register_chrdev(0,DEVICE_NAME,&chardev_fops); // 0 -> возвращает доступнуй major для модуля

	if(major < 0){
		pr_alert("Registering char device failed with %d\n",major);
		return major;
	}

	pr_info("I was assigned major number %d. \n",major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
	cls = class_create(DEVICE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
	device_create(cls, NULL, MKDEV(major,0), NULL, DEVICE_NAME);

	pr_info("Device created on /dev/%s\n",DEVICE_NAME);

	return SUCCESS;
}

/* вызываается при rmmod */
static void __exit chardev_exit(void)
{
	device_destroy(cls, MKDEV(major,0));
	class_destroy(cls);

	/* Unregister device*/
	unregister_chrdev(major, DEVICE_NAME);
}
static int device_open(struct inode *inode, struct file * file)
{
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;
    //TODO: инициализировать F_q(256) какой irreducible? пихнуть в глобальную переменную (?)
    //с данными девайса

    //TODO: вроде как можно убрать т.к. .owner = THIS_MODULE проверить
    try_module_get(THIS_MODULE); //increments the use count

    return SUCCESS;
}

static int device_release(struct inode * inode, struct file * file)
{
    //TODO: видимо почистить всё что навыделяли
	/* We're now ready for our next caller */
	atomic_set(&already_open, CDEV_NOT_USED);
	/* Decrement the usage count, or else once you opened the file, you will
	 * never get rid of the module.
	 */
    //TODO: видимо тоже можно убрать т.к. .owner = THIS_MODULE проверить
    module_put(THIS_MODULE);

	return SUCCESS;
}

/* Called when a process, which already opened the dev file, attempts to read from it.*/
//TODO: если все инициализируется и чистится как в open и close то просто разобраться как
//работает предложенный алоритм и всё ок тогда

static ssize_t device_read(struct file *filp, /* include linux/fs.h */
			   char __user *buffer, /* buffer to fill with data*/
			   size_t length, /* length of the buffer */
			   loff_t *offset)
{
	/* Number of bytes actually written to the buffer */
	int bytes_read = 0;
	const char *msg_ptr = msg;

	if(!*msg_ptr + *offset){ /* we are at the end of the message*/
		*offset = 0;
		return 0;
	}

	msg_ptr += *offset;
	/* Actually put the data into the buffer */
	while (length && *msg_ptr){
		/* The buffer is in the user data segment, not in the kernel
		 * segment so "*" assignment won't work. We have to use put_user which copies data
		 * from the kernel data segment to the user data segment.*/
		put_user(*(msg_ptr++), buffer++);
		length--;
		bytes_read++;
	}
	*offset += bytes_read;

	/* Most read function return the number of bytes put into the buffer. */
	return bytes_read;
}
static ssize_t device_write(struct file *filp, const char __user *buff,
			    size_t len, loff_t *off)
{
	pr_alert("Sorry this operation is not supported.\n");
	return -EINVAL;
}
module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
