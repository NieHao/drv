
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

// �㣬��ؽṹ�ȵ�
	/* 0.1,����һ�� video_device �ṹ�� */
static struct video_device *myvivi_device;

/*v4l2���صĸ�ʽ�ṹ: formats��ʽ�����������vivi.c�ж�����6���ʽ��
����BUF�����ͣ���ʽ������������ʽ���ơ����صĸ�ʽ��	
*/
static struct v4l2_fmtdesc myvivi_format;


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
	struct vivi_fmt *fmt;//����һ����ʽ�ṹ�������С���ʽ���ơ��͡���ʽ����

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


//0.3������Ӧ�ó���IOCTL������ioctl�ṹ��ÿ��ioctl��Ҫʵ��Ϊ����Ĺ��ܡ�
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	  = myvivi_vidioc_querycap,//����ʡ���������ͷ���������Բ����йء�
	
	/* �����о٣���ȡ�����Ժ���������ͷ�����ݸ�ʽ */
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,//��ֻ��һ�ָ�ʽʱ���оٸ�ʽ��ʡ��
	.vidioc_g_fmt_vid_cap	  = myvivi_vidioc_g_fmt_vid_cap,//��ȡ��ʽ��APP��֪��������Щ��ʽ��֧�ֵģ�����ʡ��
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,//���ø�ʽǰ�϶����Ȳ��ԣ��ʲ���ʡ��
	.vidioc_s_fmt_vid_cap	  = myvivi_vidioc_s_fmt_vid_cap,//���ø�ʽ����ʡ
#if 0
	/* ����������: ���룬��ѯ��������У�ȡ������ */
	.vidioc_reqbufs 	  = myvivi_vidioc_reqbufs,
	.vidioc_querybuf	  = myvivi_vidioc_querybuf,
	.vidioc_qbuf		  = myvivi_vidioc_qbuf,
	.vidioc_dqbuf		  = myvivi_vidioc_dqbuf,

	/* ����������ȥ�� */
	.vidioc_streamon	  = myvivi_vidioc_streamon,
	.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//0.2��APP����read,write��ioctlʱ���ͻ�ת���ں��е��������ṩ����Ӧ������
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
