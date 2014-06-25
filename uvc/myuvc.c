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
	.probe = ,		//表示当接上了能支持设备时,就调用probe函数.
	.disconnect = ,	//拔下USB设备后调用,如销毁工作.
	.id_table = ,	//表示支持哪些设备
};
*/

static struct usb_driver myuvc_driver = {
	.name		= "myuvc",
	.probe		= myuvc_probe,
	.disconnect	= myuvc_disconnect,
#if 0 //电源管理相关:设备扶起,唤醒等.
	.suspend	= uvc_suspend,
	.resume		= uvc_resume,
	.reset_resume	= uvc_reset_resume,
#endif 
	.id_table	= myuvc_ids,
 };


static int myuvc_init(void)
{
	/*1:分配一个usb_driver结构体*/
	/*1:设置usb_driver结构体*/
	/*1:注册usb_driver结构体*/
}

static void myuvc_exit(void)
{
}

module_init(myuvc_init);
module_exit(myuvc_exit);
MODULE_LICENSE("GPL");
