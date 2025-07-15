#include "linux/fs.h"
#include "linux/module.h"
#include "linux/workqueue.h"

#include "allowlist.h"
#include "arch.h"
#include "core_hook.h"
#include "klog.h" // IWYU pragma: keep
#include "ksu.h"
#include "uid_observer.h"

static struct workqueue_struct *ksu_workqueue;

// 将任务加入 KernelSU 的工作队列
bool ksu_queue_work(struct work_struct *work)
{
	return queue_work(ksu_workqueue, work);
}

// 外部声明的两个处理 execveat 的函数
extern int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr,
					void *argv, void *envp, int *flags);

extern int ksu_handle_execveat_ksud(int *fd, struct filename **filename_ptr,
				    void *argv, void *envp, int *flags);

// 调用两个处理函数执行 execveat
int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv,
			void *envp, int *flags)
{
	ksu_handle_execveat_ksud(fd, filename_ptr, argv, envp, flags);
	return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp, flags);
}

// 外部声明的初始化函数
extern void ksu_enable_sucompat();
extern void ksu_enable_ksud();

// KernelSU 初始化函数
int __init kernelsu_init(void)
{
#ifdef CONFIG_KSU_DEBUG
	pr_alert("*************************************************************");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("**                                                         **");
	pr_alert("**         You are running DEBUG version of KernelSU       **");
	pr_alert("**                                                         **");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("*************************************************************");
#endif

	ksu_core_init();

	ksu_workqueue = alloc_workqueue("kernelsu_work_queue", 0, 0);

	ksu_allowlist_init();
	ksu_uid_observer_init();

#ifdef CONFIG_KPROBES
	ksu_enable_sucompat();
	ksu_enable_ksud();
#else
#warning("KPROBES is disabled, KernelSU may not work, please check https://kernelsu.org/guide/how-to-integrate-for-non-gki.html")
#endif

	return 0;
}

// 卸载模块时调用的退出函数
void kernelsu_exit(void)
{
	ksu_allowlist_exit();
	ksu_uid_observer_exit();
	destroy_workqueue(ksu_workqueue);
	ksu_core_exit();
}

module_init(kernelsu_init);
module_exit(kernelsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("weishu");
MODULE_DESCRIPTION("Android KernelSU");

#include <linux/version.h>

// 如果宏未定义（如旧内核），手动定义为空，避免编译出错
#ifndef MODULE_IMPORT_NS
#define MODULE_IMPORT_NS(x)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
