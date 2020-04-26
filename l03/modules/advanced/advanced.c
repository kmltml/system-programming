#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/pid.h>

MODULE_LICENSE("GPL");

const struct file_operations prname_fops;
struct miscdevice prname_dev;

char prname_name[TASK_COMM_LEN] = {0};

static int __init circular_init(void)
{
  int result = 0;
  if ((result = misc_register(&prname_dev)) != 0) {
    printk(KERN_WARNING
           "Cannot register the /dev/prname device\n");
    goto err;
  }
  printk(KERN_INFO "Module advanced initialized");
  return 0;

 err:
  misc_deregister(&prname_dev);
  return result;
}

static void __exit circular_exit(void)
{
  misc_deregister(&prname_dev);
  printk(KERN_INFO "Module advanced removed");
}

module_init(circular_init);
module_exit(circular_exit);


ssize_t prname_read(struct file *filp, char __user *user_buf, size_t count,
                    loff_t *f_pos) {
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

  size_t bytes_to_copy = min(count, (size_t) (length - *f_pos));
  result = copy_to_user(user_buf, prname_name + *f_pos, bytes_to_copy);
  if (result != 0) {
    goto out;
  }

  *f_pos += bytes_to_copy;

  result = bytes_to_copy;

 out:
  return result;
}


ssize_t prname_write(struct file *filp, const char __user *user_buf, size_t count,
                     loff_t *f_pos) {
  int result = 0;
  pid_t new_pid;
  struct task_struct* task = NULL;

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
  if(task != NULL) {
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
