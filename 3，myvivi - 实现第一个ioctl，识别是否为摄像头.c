
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

/* 11个重要的ioctl中的第一个实现，识别设备是否为摄像头设备 */
static int myvivi_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	strcpy(cap->driver, "myvivi");	//驱动名字
	strcpy(cap->card, "myvivi");
	cap->version = 0x0001; 			//版本可以随意写
 	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	/*
	V4L2_CAP_VIDEO_CAPTURE表示视频的捕捉设备.
	APP访问摄像头有两种获得数据的方式。V4L2_CAP_STREAMING这是ioctl方式。
	还能通过readwrite方式 - V4L2_CAP_READWRITE.
	*/
	cap->capabilities = cap->device_caps;
	return 0;
}

//用于应用程序IOCTL操作的ioctl结构。每个ioctl都要实现为具体的功能。
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	  = myvivi_vidioc_querycap,//不能省，与打开摄像头进而与属性操作有关。
#if 0
	/* 用于列举，获取，测试和设置摄像头的数据格式 */
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,//若只有一种格式时，列举格式可省。
	.vidioc_g_fmt_vid_cap	  = myvivi_vidioc_g_fmt_vid_cap,//获取格式后APP才知道你有哪些格式是支持的，不能省。
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,//设置格式前肯定会先测试，故不能省。
	.vidioc_s_fmt_vid_cap	  = myvivi_vidioc_s_fmt_vid_cap,//设置格式不能省

	/* 缓冲区操作: 申请，查询，放入队列，取出队列 */
	.vidioc_reqbufs 	  = myvivi_vidioc_reqbufs,
	.vidioc_querybuf	  = myvivi_vidioc_querybuf,
	.vidioc_qbuf		  = myvivi_vidioc_qbuf,
	.vidioc_dqbuf		  = myvivi_vidioc_dqbuf,

	/* 这两个不能去掉 */
	.vidioc_streamon	  = myvivi_vidioc_streamon,
	.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//APP调用read,write或ioctl时，就会转入内核中调用驱动提供的相应函数。
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
#if 0
	.open			= v4l2_fh_open,
	.release		= vivi_close,
	.read			= vivi_read,
	.poll		= vivi_poll,
	.mmap			= vivi_mmap,
#endif
};


	/* video_device结构中的 release 函数: */
static void myvivi_release(struct video_device *vdev)
{
	
}

// 一，入口函数: 分配，设置，注册
static int myvivi_init(void)
{
	int errno;
// 1.1,分配一个video_device结构体
	myvivi_device = video_device_alloc();

// 1.2,设置
		/* release函数，2.6内核里必须 */
	myvivi_device->release = myvivi_release;

		/* 定义fops，APP调用 */
	myvivi_device->fops = &myvivi_fops;

		/* 设置 ioctl_ops 以提供APP操作 */
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;
		
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
