
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


static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
#if 0
	.open			= v4l2_fh_open,
	.release		= vivi_close,
	.read			= vivi_read,
	.poll		= vivi_poll,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */
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
