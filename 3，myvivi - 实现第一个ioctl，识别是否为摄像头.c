
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

/* 11����Ҫ��ioctl�еĵ�һ��ʵ�֣�ʶ���豸�Ƿ�Ϊ����ͷ�豸 */
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

//����Ӧ�ó���IOCTL������ioctl�ṹ��ÿ��ioctl��Ҫʵ��Ϊ����Ĺ��ܡ�
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	  = myvivi_vidioc_querycap,//����ʡ���������ͷ���������Բ����йء�
#if 0
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

	/* ����������ȥ�� */
	.vidioc_streamon	  = myvivi_vidioc_streamon,
	.vidioc_streamoff	  = myvivi_vidioc_streamoff,
#endif
};

//APP����read,write��ioctlʱ���ͻ�ת���ں��е��������ṩ����Ӧ������
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
