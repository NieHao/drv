
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

/*v4l2���صĸ�ʽ�ṹ: formats��ʽ�����������vivi.c�ж�����6���ʽ��
����BUF�����ͣ���ʽ������������ʽ���ơ����صĸ�ʽ��	
*/
static struct v4l2_format myvivi_format;

/*���в���1: ����*/
static struct videobuf_queue myvivi_vb_vidqueue;

/*���������Ǹ��˶���ʹ��.*/
static spinlock_t myvivi_queue_slock;


/* ------------------------------------------------------------------
	Videobuf operation:
	myvivi_buffer_setup:APP����ioctl:VIDIOC_REQBUFSʱ,�ᵼ�´˺���������
	        �����µ���count��size(����count��buf,ÿ��buf�Ƕ��).�����˷�.
	myvivi_buffer_prepare:
	        ���video_buf�ṹ��,���ú���videobuf_iolock()������buf�Ŀռ�.
   ------------------------------------------------------------------*/
static int
myvivi_buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int *size)
{

	*size = myvivi_format.fmt.pix.sizeimage;

	if (0 == *count)
		*count = 32;

	return 0;
}

/* app����ioctl VIDIOC_QBUFʱ���´˺���������.�������video_buffer�ṹ�岢����
 * videobuf_iolock()�������ڴ�.��ǰ����ʱ�ڴ�����mmap����ʱ�Ż����.
 *
*/
static int
myvivi_buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
						enum v4l2_field field)
{
	/*1.��Щ׼������*/

	/*2.���� videobuf_iolock �����ڴ�:
	 * �ӷ���videobuf-vmalloc.c������Ĳ��ļ�:
	 ��videobuf_buffer��������"case V4L2_MEMORY_USERPTR:"�Ż����.
	 ��vivi.c�����������ͷ������,ʹ�õ�������"case V4L2_MEMORY_MMAP:"
	 
	 ����"videobuf_iolock()"��Ϊ����Ϊ"case V4L2_MEMORY_USERPTR"��videobuf�����ڴ�.��
	 ��vivi.c�����������ͷ��,��Ϊʹ�õ�"videobuf_buffer"����Ϊ"V4L2_MEMORY_MMAP",����
	 videobuf_iolock()�����ò���,��Ϊ"__videobuf_iolock()"������Ϊ"V4L2_MEMORY_MMAP"
	 ʱ,ֻ�����˵��жϹ������˳���.��������д�����myviviʱ,����ע�͵����
	 "videobuf_iolock()".
	 */
	#if 0
	if (VIDEOBUF_NEEDS_INIT == buf->vb.state) {
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}
	#endif
	/*3.����״̬:��״̬�޸�ΪVIDEOBUF_PREPARED.��ʾ�Ѿ�׼������.*/
	vb->state = VIDEOBUF_PREPARED;

	return 0;
}

/*֪ͨ��������,Ӧ�ó���������������.*/
static void
myvivi_buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	//ֻ�ǽ�״̬��Ϊ��VIDEOBUF_QUEUED
	vb->state = VIDEOBUF_QUEUED;
	//list_add_tail(&buf->vb.queue, &vidq->active);
}


static void myvivi_buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	videobuf_vmalloc_free(vb);
	vb->state = VIDEOBUF_NEEDS_INIT;//������ͷ���޸ĳɳ�ʼ״̬.
}


/**/
static struct videobuf_queue_ops myvivi_video_qops = {
	.buf_setup      = myvivi_buffer_setup,//�����С�����˷ѿռ�
	.buf_prepare    = myvivi_buffer_prepare,//
	.buf_queue      = myvivi_buffer_queue,
	.buf_release    = myvivi_buffer_release,
};


/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/

static int myvivi_open(struct file * file)
{
	/*���в���2: ��ʼ��*/
	videobuf_queue_vmalloc_init(&myvivi_vb_vidqueue, &myvivi_video_qops,
			NULL, &dev->slock, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_FIELD_INTERLACED,
			sizeof(struct videobuf_buffer), fh);

	vivi_start_thread(fh);

	return 0;
}

static int myvivi_close(struct file *file)
{
	videobuf_stop(&myvivi_vb_vidqueue);
	videobuf_mmap_free(&myvivi_vb_vidqueue);
}

/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int myvivi_vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	strcpy(cap->driver, "myvivi");//����
	strcpy(cap->card, "myvivi");
	cap->version = 0x0001; //�汾��
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	//��ʾ��һ��video��׽�豸.
	return 0;
}

/*�о�֧�ֵĸ�ʽ*/
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	if (f->index >= 1)
		return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}


/*�������������Ƿ�֧��ĳ�ָ�ʽ*/
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	enum v4l2_field field;
	unsigned int maxw, maxh;

	if(f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		return -EINVAL;
	
	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		return -EINVAL;
	}

	maxw  = 1024;
	maxh  = 768;

	/*����format��width,height,
	  ����ÿ��ռ���ַ���bytesperline,��ͼ���Сsizeimage.
	*/
	f->fmt.pix.field = field;
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2,
			      &f->fmt.pix.height, 32, maxh, 0, 0);
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * 16) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;

	return 0;
}


/*���ø�ʽ*/
static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	/*���б��ʽʱ,ֻ֧����һ�ָ�ʽV4L2_PIX_FMT_YUYV,���ﲢû�о���Ӳ���豸,ֻ������
	������ͷ�豸,��������ֱ�ӽ�ӵ��ֵΪV4L2_PIX_FMT_YUYV����ṹ����Ϣ�����û��ռ�.*/
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);
	if (ret < 0)
		return ret;
	//���������ĸ�ʽ����������myvivi_format�ṹ.
	memcpy(&myvivi_format, f, sizeof(myvivi_format));
	return ret;
}


/*���ص�ǰʹ�õĸ�ʽ:
	���� v4l2_format �ṹ��.
*/
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	memcpy(f, &myvivi_format, sizeof(myvivi_format));
	return (0);
}



static int myvivi_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	return (videobuf_reqbufs(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_querybuf(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_qbuf(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_dqbuf(&myvivi_vb_vidqueue, p,file->f_flags & O_NONBLOCK));
}


static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	/*��ʾ����һ������ͷ�豸*/
		.vidioc_querycap	  = myvivi_vidioc_querycap,

	/* �����о�,��ȡ,���Ժ���������ͷ�����ݸ�ʽ */
		.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
		.vidioc_g_fmt_vid_cap	  = myvivi_vidioc_g_fmt_vid_cap,
		.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
		.vidioc_s_fmt_vid_cap	  = myvivi_vidioc_s_fmt_vid_cap,

	/* ����������: ����/��ѯ/�������/ȡ������ */
		.vidioc_reqbufs 	  = myvivi_vidioc_reqbufs,
		.vidioc_querybuf	  = myvivi_vidioc_querybuf,
		.vidioc_qbuf		  = myvivi_vidioc_qbuf,
		.vidioc_dqbuf		  = myvivi_vidioc_dqbuf,
#if 0	
	/*����,ֹͣ����ͷ:��û��streamon��Դ������ʾ����������ͷ.*/
		.vidioc_streamon	  = myvivi_vidioc_streamon,
		.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//����������,������������,APP����read,write��ioctlʱ,�ͻ���õ��ں������ṩ����Ӧ����:
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.ioctl = video_ioctl2,//��ɵ�1��ioctl��,����ͷ�豸����ʶ����.
	.open  = myvivi_open,
	.close = myvivi_close,
	
};


// �㣬��ؽṹ�ȵ�
	/* 0.1,����һ�� video_device �ṹ�� */
static struct video_device *myvivi_device;

//���û��fopsʱע������ֱ��killed
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
	/*���release����,��Ȼ��װ����ʱֱ�ӱ���*/
	myvivi_device->release = myvivi_release;
	myvivi_device->fops = &myvivi_fops;
	myvivi_device->ioctl_ops = &myvivi_ioctl_ops;

	/* ���в���:
	 * ����/��ʼ��һ������(���õ�һ��spinlock������)
	 */
	 //����ں�����ʼ��������:
	 spin_lock_init(&myvivi_queue_slock);

	// 1.3,ע��
	/* 	@1:video_device�ṹ�弴myvivi_devcie.
		@2:ע����豸������
		@3:(0 == /dev/video0, 1 == /dev/video1, ... -1 == first free)
	*/
	errno = video_register_device(myvivi_device,VFL_TYPE_GRABBER,-1);
	return errno;
}

// �������ں���
static void myvivi_exit(void)
{
	/* 2.1, �ͷ�ע��Ľṹ��*/
	video_unregister_device(myvivi_device);

	/* 2.2,�����з��䣬����ں�����ע�� */
	video_device_release(myvivi_device);
}

//֮������������������Ϊ������
module_init(myvivi_init);
module_exit(myvivi_exit);
MODULE_LICENSE("GPL");
