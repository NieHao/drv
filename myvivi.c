
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>


/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int myvivi_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	strcpy(cap->driver, "myvivi");//名字
	strcpy(cap->card, "myvivi");
	cap->version = 0x0001; //版本号
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	//表示是一个video捕捉设备.
	return 0;
}

static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	/*表示它是一个摄像头设备*/
		.vidioc_querycap	  = myvivi_vidioc_querycap,
#if 0
	/* 用于列举,获取,测试和设置摄像头的数据格式 */
		.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
		.vidioc_g_fmt_vid_cap	  = myvivi_vidioc_g_fmt_vid_cap,
		.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
		.vidioc_s_fmt_vid_cap	  = myvivi_vidioc_s_fmt_vid_cap,
	
	/* 缓冲区操作: 申请/查询/放入队列/取出队列 */
		.vidioc_reqbufs 	  = myvivi_vidioc_reqbufs,
		.vidioc_querybuf	  = myvivi_vidioc_querybuf,
		.vidioc_qbuf		  = myvivi_vidioc_qbuf,
		.vidioc_dqbuf		  = myvivi_vidioc_dqbuf,
	
	/*启动,停止摄像头:若没有streamon则源码中显示不启动摄像头.*/
		.vidioc_streamon	  = myvivi_vidioc_streamon,
		.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//驱动是桥梁,加载了驱动后,APP调用read,write或ioctl时,就会调用到内核驱动提供的相应函数:
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.ioctl = video_ioctl2,
	
};


// 零，相关结构等等
	/* 0.1,定义一个 video_device 结构体 */
static struct video_device *myvivi_device;

//解决没有fops时注册驱动直接killed
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
	/*添加release函数,不然安装驱动时直接报错*/
	myvivi_device->release = myvivi_release;
	myvivi_device->fops = &myvivi_fops;
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;

	// 1.3,注册
	/* 	@1:video_device结构体即myvivi_devcie.
		@2:注册的设备的类型
		@3:(0 == /dev/video0, 1 == /dev/video1, ... -1 == first free)
	*/
	errno = video_register_device(myvivi_device,VFL_TYPE_GRABBER,-1);
	return errno;
}

// 二，出口函数
static void myvivi_exit(void)
{
	/* 2.1, 释放注册的结构体*/
	video_unregister_device(myvivi_device);

	/* 2.2,上面有分配，则出口函数就注销 */
	video_device_release(myvivi_device);
}

//之所以是入口与出口是因为有修饰
module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");
