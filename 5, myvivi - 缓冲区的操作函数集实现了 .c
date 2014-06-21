
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


// �㣬��ؽṹ�ȵ�
	/* 0.1,����һ�� video_device �ṹ�� */
static struct video_device *myvivi_device;


/*2�����в���*/
	//2.1������buffer���еĽṹ:
static struct vb2_queue myvivi_vb_vidqueue;

/*v4l2���صĸ�ʽ�ṹ: formats��ʽ�����������vivi.c�ж�����6���ʽ��
����BUF�����ͣ���ʽ������������ʽ���ơ����صĸ�ʽ��
*/
static struct v4l2_format myvivi_format;


//������������������
/*queue_setup:APP����ioctl VIDIOC_REQBUFS �ᵼ�´˺��������ã������µ���count-buf��С��size-buf����*/
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

/* APP����ioctl VIDIOC_QBUF ʱ:
1,�ȵ��á�buffer_prepare()����׼����������ȥ���video_bufer �ṹ��--��ͷ����Ϣ��
  ���ڴ�ΪV4L2_MEMORY_USERPTR����ʱ������ videobuf_iolock�������ڴ档
2����buffer�������:list_add_tail(&buf->stream, &q->stream);
3�����á�buf_queue()����֪ͨ���á�
*/
static int myvivi_buffer_prepare(struct vb2_buffer *vb)
{
	/* 1,��׼������ */

	/* 2, ���� videobuf_iolockΪ����ΪV4L2_MEMORY_USERPTR��vudeo_buf�����ڴ档
	������vivi����ʵ���ò��ţ�����ȥ��,VIVI����V4L2_MEMORY_MMAP����*/
#if 0
	if(VIDEOBUF_NEEDS_INIT == buf->vb.state){
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}
#endif
	//buffer_prepare������Ҫ���video_buf�ṹ�� -- struct vb2_buffer
	/* 3,����״̬:���ｫ�ṹ�е�״̬���ΪVIDEOBUF_PREPARED����ʾ��׼���á� */
	vb->state = VB2_BUF_STATE_PREPARED;//2.6����VIDEOBUF_PREPARED

	return 0;
}

static void myvivi_buffer_queue(struct vb2_buffer *vb)
{
	vb->state = VB2_BUF_STATE_QUEUED; //ֻ�ǽ��ṹ�е�״̬�޸����¡�
	//spin_lock_irqsave(&dev->slock, flags);
	//list_add_tail(&buf->list, &vidq->active);
	//spin_unlock_irqrestore(&dev->slock, flags);
}

/* APP����ʹ�û�����ʱ�����ô˺����ͷ��ڴ� */
#if 0
void myvivi_buffer_release(struct videobuf_buffer *vb)
{
	/*free_buffer(vq, buf);
	  ->videobuf_vmalloc_free(&buf->vb);
	    buf->vb.state = VIDEOBUF_NEEDS_INIT;
	*/
	//videobuf_vmalloc_free(vb);//�ͷ��ڴ�
	//��videobuf_queue�ṹͷ����״̬��Ϊ��ʼ״̬��3.4�в�Ҫ�ˡ�
	//vb.stata = VIDEOBUF_NEEDS_INIT;

	vfree(vb);
}
#endif

static void myvivi_buffer_cleanup(struct vb2_buffer *vb)
{
	vb2_get_drv_priv(vb->vb2_queue);
}


	//2.3���������buffer�ĺ�������--�ṹ:
/*
buf_setup:����buf��С�������˷ѿռ䡣
buf_prepare:���video_buf�ṹ��͵��� videobuf_iolock()����ȥ����prepare-Ԥ���Ŀռ䡣
*/
static struct vb2_ops myvivi_video_qops = {
	.queue_setup		= myvivi_queue_setup,
	.buf_prepare		= myvivi_buffer_prepare,
	.buf_queue			= myvivi_buffer_queue,
//	.buf_release        = myvivi_buffer_release, 2.6�е�
	.buf_cleanup		= myvivi_buffer_cleanup,

};



	//2.4, ������г�ʼ����Ҫʹ�õġ���������:
static spinlock_t myvivi_queue_slock;

	//2.5, ���绯Ҫʹ�õġ���������:����ں����г�ʼ���ȽϺá�


//��open�����г�ʼ��buffer����
static int myvivi_open(struct file *filp)
{	//2.2�����в���--���г�ʼ��
 	videobuf_queue_vmalloc_init(&myvivi_vb_vidqueue, &myvivi_video_qops,
				    NULL, &myvivi_queue_slock,V4L2_BUF_TYPE_VIDEO_CAPTURE,
				    V4L2_FIELD_INTERLACED,sizeof(myvivi_vb_vidqueue), NULL,NULL);
	return 0;
}

//��open�����г�ʼ��buffer���У�����close����������buffer���С�
static int myvivi_close(struct file *file)
{
	videobuf_stop(&myvivi_vb_vidqueue);
	videobuf_mmap_free(&myvivi_vb_vidqueue);
	return 0;
}


/*0.3 - 1�� 11����Ҫ��ioctl�еĵ�һ��ʵ�֣�ʶ���豸�Ƿ�Ϊ����ͷ�豸 */
static int myvivi_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	strcpy(cap->driver, "myvivi");	//��������
	strcpy(cap->card, "myvivi");
	cap->version = 0x0001; 			//�汾��������д
 	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	/*
	V4L2_CAP_VIDEO_CAPTURE��ʾ��Ƶ�Ĳ�׽�豸.
	APP��������ͷ�����ֻ�����ݵķ�ʽ��V4L2_CAP_STREAMING����ioctl��ʽ��
	����ͨ��readwrite��ʽ - V4L2_CAP_READWRITE.
	*/
	cap->capabilities = cap->device_caps;
	return 0;
}

//0.3 - 2 �о�֧�ֵĸ�ʽ������ֻ�жϸ�ʽf�ṹֻ��һ�ָ�ʽʱ�ųɹ���������Ϊ0ʱ���Ҹ�ʽ�����ֺ��������¡�
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
//	struct vivi_fmt *fmt;//����һ����ʽ�ṹ�������С���ʽ���ơ��͡���ʽ����

	if (f->index >= 1)//ֱ��ֻ֧��һ�ָ�ʽ��������Ϊ0ʱ������
		return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");//����2��ʽ�����������ִ�����ǰ�ĸ�ʽ��
	f->pixelformat = V4L2_PIX_FMT_YUYV;//Ĭ�ϸ�ʽ֮һ��V4L2_PIX_FMT_YUYV��ʽ����ǰ�ĵ�ǰ����ĸ�ʽ�ṹf��
	return 0;
}

//0.3 - 3 ����֧�����ָ�ʽ:��ȡ��ʽ��APP��֪��������Щ��ʽ��֧�ֵġ���Ҫ�ǹ��졮��ʽ�ṹ--v4l2_format����
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	memcpy(f, &myvivi_format, sizeof(myvivi_format));//�Զ���һ����ʽ�ṹֱ�Ӹ��ƹ�����
	return 0;
}


//0.3 - 4 ���ø�ʽǰ�϶����Ȳ���
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	unsigned int maxw, maxh;
	enum v4l2_field field;
	//��ö��֧�ֵĸ�ʽʱ������ֻд��ֻ֧��V4L2_PIX_FMT_YUYV����������ֻ�ж������ʽ�Ƿ�֧�֡�
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

	//����format��Ⱥ͸߶ȣ�����ÿ��ռ���ֽ���������ͼ��Ĵ�С��
	v4l_bound_align_image(&f->fmt.pix.width, 48, 2048, 2,
			      &f->fmt.pix.height, 32, 1536, 0, 0);
	f->fmt.pix.bytesperline =//ÿһ��ռ���ֽ���
		(f->fmt.pix.width * 16) >> 3;//depth��ʾ��ɫ��ȡ�����ֱ��д��16.
	f->fmt.pix.sizeimage =//����ͼ��Ĵ�С
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	return 0;
}

//0.3 - 5.2 //���ø�ʽ
static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);//��2˽������Ϊ��
	if (ret < 0)
		return ret;

	//���������ĸ�ʽ����������myvivi_format�ṹ��
	memcpy(&myvivi_format, f, sizeof(myvivi_format));
	return 0;
}


static int myvivi_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{//�������Ƕ���Ķ���Ϊ:myvivi_vb_vidqueue
	return vb2_reqbufs(&myvivi_vb_vidqueue, p);
}

/* ��ѯbuffer��Ϣ */
static int myvivi_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_querybuf(&myvivi_vb_vidqueue, p);
}
/* ��buffer������� */
static int myvivi_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_qbuf(&myvivi_vb_vidqueue, p);
}

/* ��buffer�Ӷ���ȡ�� */
static int myvivi_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return vb2_dqbuf(&myvivi_vb_vidqueue, p, file->f_flags & O_NONBLOCK);
}

//0.3������Ӧ�ó���IOCTL������ioctl�ṹ��ÿ��ioctl��Ҫʵ��Ϊ����Ĺ��ܡ�
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	  = myvivi_vidioc_querycap,//����ʡ���������ͷ���������Բ����йء�

	/* �����о٣���ȡ�����Ժ���������ͷ�����ݸ�ʽ */
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,//��ֻ��һ�ָ�ʽʱ���оٸ�ʽ��ʡ��
	.vidioc_g_fmt_vid_cap	  = myvivi_vidioc_g_fmt_vid_cap,//��ȡ��ʽ��APP��֪��������Щ��ʽ��֧�ֵģ�����ʡ��
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,//���ø�ʽǰ�϶����Ȳ��ԣ��ʲ���ʡ��
	.vidioc_s_fmt_vid_cap	  = myvivi_vidioc_s_fmt_vid_cap,//���ø�ʽ����ʡ

	/* ����������: ���룬��ѯ��������У�ȡ������ */
	.vidioc_reqbufs 	  = myvivi_vidioc_reqbufs,
	.vidioc_querybuf	  = myvivi_vidioc_querybuf,
	.vidioc_qbuf		  = myvivi_vidioc_qbuf,
	.vidioc_dqbuf		  = myvivi_vidioc_dqbuf,
#if 0
	/* ����������ȥ�� */
	.vidioc_streamon	  = myvivi_vidioc_streamon,
	.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//0.2��APP����read,write��ioctlʱ���ͻ�ת���ں��е��������ṩ����Ӧ������
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
};


	/* video_device�ṹ�е� release ����: */
static void myvivi_release(struct video_device *vdev)
{

}

// һ����ں���: ���䣬���ã�ע��
static int myvivi_init(void)
{
	int errno;
// 1.1,����һ��video_device�ṹ��
	myvivi_device = video_device_alloc();

// 1.2,����
		/* release������2.6�ں������ */
	myvivi_device->release = myvivi_release;

		/* ����fops��APP���� */
	myvivi_device->fops = &myvivi_fops;

		/* ���� ioctl_ops ���ṩAPP���� */
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;

/* ���в��� */
	//2.5, ���绯Ҫʹ�õġ���������:����ں����г�ʼ���ȽϺá�
	spin_lock_init(&myvivi_queue_slock);


// 1.3,ע��
	/* 	��1:video_device�ṹ�弴myvivi_devcie.
		��2��ע����豸������
		��3��(0 == /dev/video0, 1 == /dev/video1, ... -1 == first free)
	*/
	errno = video_register_device(myvivi_device,VFL_TYPE_GRABBER,-1);
	return errno;
}

// �������ں���
static void myvivi_exit(void)
{
	/* 2.1, */
	video_unregister_device(myvivi_device);

	/* 2.2,�����з��䣬����ں�����ע�� */
	video_device_release(myvivi_device);
}

//֮������������������Ϊ������
module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");
