#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE1(print_student_name, char __user *, name) {
	char buf[256];
	long ret;

	ret = strncpy_from_user(buf, name, sizeof(buf));
	if (ret < 0) {
	return -EFAULT;
	}
	if (ret == sizeof(buf)) {
	buf[sizeof(buf) - 1] = '\0';
	}

	printk(KERN_INFO "My Name is %s\n", buf);
	return 0;
}

SYSCALL_DEFINE1(print_student_id, char __user *, id) {
	char buf[256];
	long ret = strncpy_from_user(buf, id, sizeof(buf));

	if (ret < 0) {
	return -EFAULT;
	}
	buf[sizeof(buf) - 1] = '\0';

	printk(KERN_INFO "My Student ID is %s\n", buf);
	return 0;
}

SYSCALL_DEFINE2(print_student_info, char __user *, school, char __user *, major) {
	char school_buf[256];
	char major_buf[256];
	long ret;

	ret = strncpy_from_user(school_buf, school, sizeof(school_buf));
	if (ret < 0) {
	return -EFAULT;
	}
	school_buf[sizeof(school_buf) - 1] = '\0';

	ret = strncpy_from_user(major_buf, major, sizeof(major_buf));
	if (ret < 0) {
	return -EFAULT;
	}
	major_buf[sizeof(major_buf) - 1] = '\0';

	printk(KERN_INFO "I go to %s\n", school_buf);
	printk(KERN_INFO "I major in %s\n", major_buf);
	return 0;
}