
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>

// 零，相关结构等等
	/* 0.1,定义一个 video_device 结构体 */
static struct video_device *myvivi_device;

// 一，入口函数: 分配，设置，注册
static int myvivi_init(void)
{
	int errno;
	// 1.1,分配一个video_device结构体
	myvivi_device = video_device_alloc();

	// 1.2,设置

	// 1.3,注册
	/* 	参1:video_device结构体即myvivi_devcie.
		参2，注册的设备的类型
		参3，(0 == /dev/video0, 1 == /dev/video1, ... -1 == first free)
	*/
	errno = video_register_device(myvivi_device,VFL_TYPE_GRABBER,-1);
	return errno;
}

// 二，出口函数
static void myvivi_exit(void)
{
	/* 2.1, */
	video_unregister_device(myvivi_device);

	/* 2.2,上面有分配，则出口函数就注销 */
	video_device_release(myvivi_device);
}

//之所以是入口与出口是因为有修饰
module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");