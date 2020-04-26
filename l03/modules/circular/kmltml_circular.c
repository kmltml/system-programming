#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

MODULE_LICENSE("GPL");

#define MAX_BUFFER_SIZE 1024

const struct file_operations circ_fops;
const struct proc_ops proc_fops;
struct miscdevice device;

char *circ_buffer = NULL;
size_t buffer_size = 40;
size_t write_index = 0;

struct proc_dir_entry *proc_entry;

static int __init circular_init(void)
{
	int result = 0;

	proc_entry = proc_create("circular", 0222, NULL, &proc_fops);
	if (proc_entry == NULL) {
		printk(KERN_WARNING "Cannot create /proc/circular\n");
                result = -EFAULT;
		goto err;
	}

	if ((result = misc_register(&device)) != 0) {
		printk(KERN_WARNING
		       "Cannot register the /dev/circular device\n");
		goto err;
	}

	circ_buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (circ_buffer == NULL) {
		result = -ENOMEM;
		printk(KERN_WARNING
		       "Cannot allocate memory for /dev/circular\n");
		goto err;
	}

	printk(KERN_INFO "Circular module initialized with minor number %d\n",
	       device.minor);
	return 0;

err:
	if (proc_entry) {
		proc_remove(proc_entry);
	}
	misc_deregister(&device);
	kfree(circ_buffer);
	return result;
}

static void __exit circular_exit(void)
{
	if (proc_entry) {
		proc_remove(proc_entry);
	}
	misc_deregister(&device);
	kfree(circ_buffer);
	printk(KERN_INFO "Circular module removed\n");
}

ssize_t circ_read(struct file *filp, char __user *user_buf, size_t count,
		  loff_t *f_pos)
{
	if (*f_pos >= buffer_size) {
		return 0;
	}
	size_t bytes_to_copy = min(count, (size_t)(buffer_size - *f_pos));
	if (copy_to_user(user_buf, circ_buffer + *f_pos, bytes_to_copy) != 0) {
		printk(KERN_WARNING
		       "Couldn't copy data to user space in /dev/circular:read\n");
		return -EFAULT;
	}
	*f_pos += bytes_to_copy;
	return bytes_to_copy;
}

ssize_t circ_write(struct file *filp, const char __user *user_buf, size_t count,
		   loff_t *f_pos)
{
	size_t remaining = count;
	size_t read_index = 0;
	while (remaining > 0) {
		size_t copy_count = min(remaining, buffer_size - write_index);
		if (copy_from_user(circ_buffer + write_index,
				   user_buf + read_index, copy_count) != 0) {
			printk(KERN_WARNING
			       "Couldn't copy data from user space in /dev/circular:write\n");
			return -EFAULT;
		}
		read_index += copy_count;
		write_index += copy_count;
		remaining -= copy_count;
		while (write_index >= buffer_size) {
			write_index -= buffer_size;
		}
	}
	return count;
}

ssize_t proc_write(struct file *filp, const char __user *user_buf, size_t count,
		   loff_t *f_pos)
{
	uint64_t new_size;
	int result = 0;
	if ((result = kstrtou64_from_user(user_buf, count, 10, &new_size)) !=
	    0) {
		return result;
	}
	if (new_size > MAX_BUFFER_SIZE) {
		return -ERANGE;
	}
	void *new_buff = krealloc(circ_buffer, new_size, GFP_KERNEL);
	if (new_buff == NULL) {
		printk(KERN_WARNING
		       "Couldn't reallocate the circular buffer\n");
		return -EFAULT;
	}
	circ_buffer = new_buff;
	if (new_size > buffer_size) {
		memset(circ_buffer + buffer_size, 0, new_size - buffer_size);
	}
	buffer_size = new_size;
	return count;
}

const struct file_operations circ_fops = {
	.read = circ_read,
	.write = circ_write,
};

const struct proc_ops proc_fops = {
	.proc_write = proc_write,
};

struct miscdevice device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "circular",
	.fops = &circ_fops,
	.nodename = "circular",
	.mode = 0666,
};

module_init(circular_init);
module_exit(circular_exit);
