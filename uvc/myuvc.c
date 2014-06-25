#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>

/*
static struct usb_driver myuvc_driver = {
	.probe = ,		//��ʾ����������֧���豸ʱ,�͵���probe����.
	.disconnect = ,	//����USB�豸�����,�����ٹ���.
	.id_table = ,	//��ʾ֧����Щ�豸
};
*/

static struct usb_driver myuvc_driver = {
	.name		= "myuvc",
	.probe		= myuvc_probe,
	.disconnect	= myuvc_disconnect,
#if 0 //��Դ�������:�豸����,���ѵ�.
	.suspend	= uvc_suspend,
	.resume		= uvc_resume,
	.reset_resume	= uvc_reset_resume,
#endif 
	.id_table	= myuvc_ids,
 };


static int myuvc_init(void)
{
	/*1:����һ��usb_driver�ṹ��*/
	/*1:����usb_driver�ṹ��*/
	/*1:ע��usb_driver�ṹ��*/
}

static void myuvc_exit(void)
{
}

module_init(myuvc_init);
module_exit(myuvc_exit);
MODULE_LICENSE("GPL");
