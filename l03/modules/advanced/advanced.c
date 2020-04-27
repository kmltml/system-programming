#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/pid.h>

#include <linux/jiffies.h>

#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/mount.h>

MODULE_LICENSE("GPL");

const struct file_operations prname_fops;
struct miscdevice prname_dev;

const struct file_operations jiffies_fops;
struct miscdevice jiffies_dev;

const struct file_operations mountderef_fops;
struct miscdevice mountderef_dev;

char prname_name[TASK_COMM_LEN] = { 0 };
char mountderef_name[TASK_COMM_LEN] = { 0 };

static int __init advanced_init(void)
{
  int result = 0;
  if ((result = misc_register(&prname_dev)) != 0) {
		printk(KERN_WARNING "Cannot register the /dev/prname device\n");
    goto err;
  }
  if ((result = misc_register(&jiffies_dev)) != 0) {
    printk(KERN_WARNING
           "Cannot register the /dev/jiffies device\n");
    goto err;
  }
        if ((result = misc_register(&mountderef_dev)) != 0) {
                printk(KERN_WARNING
                       "Cannot register the /dev/mountderef device\n");
                goto err;
        }
  printk(KERN_INFO "Module advanced initialized");
  return 0;

err:
  misc_deregister(&prname_dev);
  misc_deregister(&jiffies_dev);
        misc_deregister(&mountderef_dev);
  return result;
}

static void __exit advanced_exit(void)
{
  misc_deregister(&prname_dev);
        misc_deregister(&jiffies_dev);
        misc_deregister(&mountderef_dev);
  printk(KERN_INFO "Module advanced removed");
}

module_init(advanced_init);
module_exit(advanced_exit);

ssize_t prname_read(struct file *filp, char __user *user_buf, size_t count,
		    loff_t *f_pos)
{
  ssize_t result = 0;

  size_t length = strnlen(prname_name, ARRAY_SIZE(prname_name));

  if (length == 0) {
    result = EINVAL;
    goto out;
  }

  if (*f_pos >= length) {
    result = 0;
    goto out;
  }

	size_t bytes_to_copy = min(count, (size_t)(length - *f_pos));
  result = copy_to_user(user_buf, prname_name + *f_pos, bytes_to_copy);
  if (result != 0) {
    goto out;
  }

  *f_pos += bytes_to_copy;

  result = bytes_to_copy;

out:
  return result;
}

ssize_t prname_write(struct file *filp, const char __user *user_buf,
		     size_t count, loff_t *f_pos)
{
  int result = 0;
  pid_t new_pid;
	struct task_struct *task = NULL;

  result = kstrtou32_from_user(user_buf, count, 10, &new_pid);
  if (result != 0) {
    goto out;
  }

  task = get_pid_task(find_vpid(new_pid), PIDTYPE_PID);
  if (task == NULL) {
    result = -ESRCH;
    goto out;
  }

  get_task_comm(prname_name, task);

  result = count;

out:
	if (task != NULL) {
    put_task_struct(task);
    task = NULL;
  }
  return result;
}

const struct file_operations prname_fops = {
        .read = prname_read,
        .write = prname_write,
};

struct miscdevice prname_dev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "prname",
        .fops = &prname_fops,
        .nodename = "prname",
        .mode = 0666,
};

ssize_t jiffies_read(struct file *filp, char __user *user_buf, size_t count,
		     loff_t *f_pos)
{
  char buffer[16];

  uint64_t jiffies = get_jiffies_64();

  size_t length = snprintf(buffer, ARRAY_SIZE(buffer), "%llu", jiffies);

  if (*f_pos >= length) {
    return 0;
  }

	size_t bytes_to_copy = min(count, (size_t)(length - *f_pos));

  int result = copy_to_user(user_buf, buffer + *f_pos, bytes_to_copy);
  if (result != 0) {
    return result;
  }

  *f_pos += bytes_to_copy;
  return bytes_to_copy;
}

const struct file_operations jiffies_fops = {
        .read = jiffies_read,
};

struct miscdevice jiffies_dev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "jiffies",
        .fops = &jiffies_fops,
        .nodename = "jiffies",
        .mode = 0444,
};

ssize_t mountderef_read(struct file *filp, char __user *user_buf, size_t count,
                    loff_t *f_pos)
{
        ssize_t result = 0;

        size_t length = strnlen(mountderef_name, ARRAY_SIZE(mountderef_name));

        if (length == 0) {
                result = EINVAL;
                goto out;
        }

        if (*f_pos >= length) {
                result = 0;
                goto out;
        }

        size_t bytes_to_copy = min(count, (size_t)(length - *f_pos));
        result = copy_to_user(user_buf, mountderef_name + *f_pos, bytes_to_copy);
        if (result != 0) {
                goto out;
        }

        *f_pos += bytes_to_copy;

        result = bytes_to_copy;

out:
        return result;
}

ssize_t mountderef_write(struct file *filp, const char __user *user_buf,
                     size_t count, loff_t *f_pos)
{
  int result = 0;

  static char path_buff[PATH_MAX + 1];
  if (count >= PATH_MAX) {
    result = ERANGE;
    goto out;
  }

  result = copy_from_user(path_buff, user_buf, count);
  if (result != 0) {
    goto out;
  }
  path_buff[count + 1] = 0;
  printk(KERN_INFO "mountderef path: %s\n", path_buff);

  struct path path;

  result = kern_path(path_buff, LOOKUP_OPEN | LOOKUP_FOLLOW | LOOKUP_DOWN, &path);
  if (result != 0) {
    printk(KERN_WARNING "mountderef kern_path result: %d\n", result);
    goto out;
  }

  char* path_res = dentry_path_raw(path.mnt->mnt_root, path_buff, ARRAY_SIZE(path_buff));
  strcpy(mountderef_name, path_res);

  result = count;

 out:
  return result;
}

const struct file_operations mountderef_fops = {
        .read = mountderef_read,
        .write = mountderef_write,
};

struct miscdevice mountderef_dev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "mountderef",
        .fops = &mountderef_fops,
        .nodename = "mountderef",
        .mode = 0666,
};
