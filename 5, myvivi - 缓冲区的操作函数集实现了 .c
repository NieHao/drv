
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <linux/videodev2.h>
#include <media/videobuf2-vmalloc.h>


// 零，相关结构等等
	/* 0.1,定义一个 video_device 结构体 */
static struct video_device *myvivi_device;


/*2，队列操作*/
	//2.1，定义buffer队列的结构:
static struct vb2_queue myvivi_vb_vidqueue;

/*v4l2像素的格式结构: formats格式数组的索引，vivi.c中定义了6项格式。
还有BUF的类型，格式的描述，即格式名称。像素的格式。
*/
static struct v4l2_format myvivi_format;


//缓冲区操作函数集合
/*queue_setup:APP调用ioctl VIDIOC_REQBUFS 会导致此函数被调用，其重新调整count-buf大小和size-buf个数*/
//static int myvivi_queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
//				unsigned int *nbuffers, unsigned int *nplanes,
//				unsigned int sizes[], void *alloc_ctxs[])

int myvivi_queue_setup(struct vb2_queue *q, const struct v4l2_format *fmt,
			   unsigned int *num_buffers, unsigned int *num_planes,
			   unsigned int sizes[], void *alloc_ctxs[])
{
	*sizes = myvivi_format.fmt.pix.sizeimage;
	if (0 == *num_buffers)
		*num_buffers = 32;

	return 0;
}

/* APP调用ioctl VIDIOC_QBUF 时:
1,先调用‘buffer_prepare()’作准备工作，会去填充video_bufer 结构体--即头部信息。
  当内存为V4L2_MEMORY_USERPTR类型时，调用 videobuf_iolock来分配内存。
2，将buffer放入队列:list_add_tail(&buf->stream, &q->stream);
3，调用‘buf_queue()’起通知作用。
*/
static int myvivi_buffer_prepare(struct vb2_buffer *vb)
{
	/* 1,做准备工作 */

	/* 2, 调用 videobuf_iolock为类型为V4L2_MEMORY_USERPTR的vudeo_buf分配内存。
	但是在vivi中其实并用不着，可以去掉,VIVI是用V4L2_MEMORY_MMAP。。*/
#if 0
	if(VIDEOBUF_NEEDS_INIT == buf->vb.state){
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}
#endif
	//buffer_prepare函数是要填充video_buf结构体 -- struct vb2_buffer
	/* 3,设置状态:这里将结构中的状态填充为VIDEOBUF_PREPARED，表示已准备好。 */
	vb->state = VB2_BUF_STATE_PREPARED;//2.6中是VIDEOBUF_PREPARED

	return 0;
}

static void myvivi_buffer_queue(struct vb2_buffer *vb)
{
	vb->state = VB2_BUF_STATE_QUEUED; //只是将结构中的状态修改了下。
	//spin_lock_irqsave(&dev->slock, flags);
	//list_add_tail(&buf->list, &vidq->active);
	//spin_unlock_irqrestore(&dev->slock, flags);
}

/* APP不再使用缓冲区时，就用此函数释放内存 */
#if 0
void myvivi_buffer_release(struct videobuf_buffer *vb)
{
	/*free_buffer(vq, buf);
	  ->videobuf_vmalloc_free(&buf->vb);
	    buf->vb.state = VIDEOBUF_NEEDS_INIT;
	*/
	//videobuf_vmalloc_free(vb);//释放内存
	//将videobuf_queue结构头部的状态置为初始状态。3.4中不要了。
	//vb.stata = VIDEOBUF_NEEDS_INIT;

	vfree(vb);
}
#endif

static void myvivi_buffer_cleanup(struct vb2_buffer *vb)
{
	vb2_get_drv_priv(vb->vb2_queue);
}


	//2.3，定义操作buffer的函数集体--结构:
/*
buf_setup:计算buf大小，以免浪费空间。
buf_prepare:填充video_buf结构体和调用 videobuf_iolock()函数去分配prepare-预备的空间。
*/
static struct vb2_ops myvivi_video_qops = {
	.queue_setup		= myvivi_queue_setup,
	.buf_prepare		= myvivi_buffer_prepare,
	.buf_queue			= myvivi_buffer_queue,
//	.buf_release        = myvivi_buffer_release, 2.6中的
	.buf_cleanup		= myvivi_buffer_cleanup,

};



	//2.4, 定义队列初始化中要使用的‘自旋锁’:
static spinlock_t myvivi_queue_slock;

	//2.5, 初如化要使用的‘自旋锁’:在入口函数中初始化比较好。


//在open函数中初始化buffer队列
static int myvivi_open(struct file *filp)
{	//2.2，队列操作--队列初始化
 	videobuf_queue_vmalloc_init(&myvivi_vb_vidqueue, &myvivi_video_qops,
				    NULL, &myvivi_queue_slock,V4L2_BUF_TYPE_VIDEO_CAPTURE,
				    V4L2_FIELD_INTERLACED,sizeof(myvivi_vb_vidqueue), NULL,NULL);
	return 0;
}

//在open函数中初始化buffer队列，则在close函数中销毁buffer队列。
static int myvivi_close(struct file *file)
{
	videobuf_stop(&myvivi_vb_vidqueue);
	videobuf_mmap_free(&myvivi_vb_vidqueue);
	return 0;
}


/*0.3 - 1， 11个重要的ioctl中的第一个实现，识别设备是否为摄像头设备 */
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

//0.3 - 2 列举支持的格式。这里只判断格式f结构只有一种格式时才成功，即索引为0时，且格式的名字和类型如下。
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
//	struct vivi_fmt *fmt;//定义一个格式结构，其中有‘格式名称’和‘格式’。

	if (f->index >= 1)//直接只支持一种格式，好索引为0时成立。
		return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");//将参2格式的名称描述字串给当前的格式。
	f->pixelformat = V4L2_PIX_FMT_YUYV;//默认格式之一的V4L2_PIX_FMT_YUYV格式给当前的当前定义的格式结构f。
	return 0;
}

//0.3 - 3 返回支持哪种格式:获取格式后APP才知道你有哪些格式是支持的。主要是构造‘格式结构--v4l2_format’。
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	memcpy(f, &myvivi_format, sizeof(myvivi_format));//自定义一个格式结构直接复制过来。
	return 0;
}


//0.3 - 4 设置格式前肯定会先测试
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	unsigned int maxw, maxh;
	enum v4l2_field field;
	//在枚举支持的格式时，我们只写明只支持V4L2_PIX_FMT_YUYV，所以下面只判断这个格式是否支持。
	if(f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		return -EINVAL;

	field = f->fmt.pix.field;
	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		return -EINVAL;
	}

	maxw = 1024;
	maxh = 768;

	//高速format宽度和高度，计算每行占的字节数和整个图像的大小。
	v4l_bound_align_image(&f->fmt.pix.width, 48, 2048, 2,
			      &f->fmt.pix.height, 32, 1536, 0, 0);
	f->fmt.pix.bytesperline =//每一行占的字节数
		(f->fmt.pix.width * 16) >> 3;//depth表示颜色深度。这里直接写成16.
	f->fmt.pix.sizeimage =//整个图像的大小
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	return 0;
}

//0.3 - 5.2 //设置格式
static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);//参2私有数据为空
	if (ret < 0)
		return ret;

	//将传进来的格式参数拷贝到myvivi_format结构。
	memcpy(&myvivi_format, f, sizeof(myvivi_format));
	return 0;
}


static int myvivi_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{//上面我们定义的队列为:myvivi_vb_vidqueue
	return vb2_reqbufs(&myvivi_vb_vidqueue, p);
}

/* 查询buffer信息 */
static int myvivi_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_querybuf(&myvivi_vb_vidqueue, p);
}
/* 将buffer放入队列 */
static int myvivi_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_qbuf(&myvivi_vb_vidqueue, p);
}

/* 将buffer从队列取出 */
static int myvivi_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_dqbuf(&myvivi_vb_vidqueue, p, file->f_flags & O_NONBLOCK);
}

//0.3，用于应用程序IOCTL操作的ioctl结构。每个ioctl都要实现为具体的功能。
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	  = myvivi_vidioc_querycap,//不能省，与打开摄像头进而与属性操作有关。

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
#if 0
	/* 这两个不能去掉 */
	.vidioc_streamon	  = myvivi_vidioc_streamon,
	.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//0.2，APP调用read,write或ioctl时，就会转入内核中调用驱动提供的相应函数。
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
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

/* 队列操作 */
	//2.5, 初如化要使用的‘自旋锁’:在入口函数中初始化比较好。
	spin_lock_init(&myvivi_queue_slock);


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
