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
	.probe = ,	//表示当接上了能支持设备时,就调用probe函数.
	.disconnect = ,//拔下USB设备后调用,如销毁工作.
	.id_table = ,	//表示支持哪些设备
};
*/

static int myuvc_probe(struct usb_interface *intf,
		     const struct usb_device_id *id)
{
	static int cnt = 0;
	//通过usb_interface USB接口得到usb_device结构,usb_device结构中有usb_device_descriptor结构
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_device_descriptor *descriptor = &udev->descriptor;//USB设备描述符

	int i;
	struct usb_host_config *hostconfig;
	struct usb_config_descriptor *config;//配置描述符
	
	printk("myuvc_probe:cnt = %d\n",cnt++);
	//参考lsusb.c->dump_device()打印设备描述符.
	printk("Device Descriptor:\n"
	       "  bLength             %5u\n"//描述符的长度，设备描述符的长度为18个字节。
	       "  bDescriptorType     %5u\n"//描述符的类型，设备描述符的类型为0x01。
	       "  bcdUSB              %2x.%02x\n"//USB设备所遵循的协议版本号，如2.0协议为0x0200。
       /*USB设备类代码，由USB-IF分配，如果该字段为0x00，表示由接口描述符来指定
          有可能该USB设备是一个复合设备，USB设备的各个接口相互独立，分别属于不同的设备类）。
          如果是0x01~0xfe，表示为USB-IF定义的设备类，例如0x03为HID设备，0x09为HUB设备。如果
          是0xff，表示由厂商自定义设备类型。*/
	       "  bDeviceClass        %5u \n"

		/*USB子类代码，由USB-IF分配，如果bDeviceClass为0x00，那么该字段也必须为 0x00，
		其它情况可以参考USB关于对于USB Device Class的定义。*/
	       "  bDeviceSubClass     %5u \n"

		/*协议代码，由USB-IF分配，如果bDeviceClass和bDeviceSubClass定义为0x00，那么该
	       字段也必须为0x00。*/
	       "  bDeviceProtocol     %5u \n"
	       "  bMaxPacketSize0     %5u\n"//端点0最大数据包长度，必须为8、16、32和64.
	       "  idVendor           0x%04x \n"//厂商ID，由USB-IF分配，需要向USB-IF组织申请。
	       "  idProduct          0x%04x \n"//产品ID，由厂商指定。
	       "  bcdDevice           %2x.%02x\n"//设备序列号，由厂商自行设置。
	       "  iManufacturer       %5u \n"//用于描述厂商的字符串描述符索引。
	       "  iProduct            %5u \n"//用于描述产品的字符串描述符索引。

		/*用于描述产品序列号的字符串描述符索引，注意，所有的字符串描述符是可选的，如果没有
	       字符串描述符，指定这些索引为0x00。*/
	       "  iSerial             %5u \n"
	       
	       "  bNumConfigurations  %5u\n",//配置描述符数量。
	       descriptor->bLength, descriptor->bDescriptorType,
	       descriptor->bcdUSB >> 8, descriptor->bcdUSB & 0xff,
	       descriptor->bDeviceClass, /*cls,*/
	       descriptor->bDeviceSubClass, /*subcls,*/
	       descriptor->bDeviceProtocol, /*proto,*/
	       descriptor->bMaxPacketSize0,
	       descriptor->idVendor, /*vendor,*/ descriptor->idProduct, /*product,*/
	       descriptor->bcdDevice >> 8, descriptor->bcdDevice & 0xff,
	       descriptor->iManufacturer, /*mfg,*/
	       descriptor->iProduct, /*prod,*/
	       descriptor->iSerialNumber, /*serial,*/
	       descriptor->bNumConfigurations);

	for (i = 0; i < descriptor->bNumConfigurations; i++)
	{
		hostconfig = &udev->config[i];
		config = &hostconfig->desc;
		printk("  Configuration Descriptor: %d\n"
			   "	bLength 			%5u\n"
			   "	bDescriptorType 	%5u\n"
			   "	wTotalLength		%5u\n"
			   "	bNumInterfaces		%5u\n"
			   "	bConfigurationValue %5u\n"
			   "	iConfiguration		%5u\n"
			   "	bmAttributes		 0x%02x\n",
			   i,
			   config->bLength, config->bDescriptorType,
			   le16_to_cpu(config->wTotalLength),
			   config->bNumInterfaces, config->bConfigurationValue,
			   config->iConfiguration,
			   config->bmAttributes);
	}
	return 0;
}


static void myuvc_disconnect(struct usb_interface *intf)
{
	static int cnt = 0;
	printk("myuvc_disconnect:cnt = %d\n",cnt++);
}

static struct usb_device_id myuvc_ids[] = {
	/* Generic USB Video Class */
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) },//参2为视频控制接口
	//正常只要上面一项视频控制接口,以后可以通过视频控制接口找到USB硬件的视频流接口.
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) },//参2为视频流接口
	{}
};


/*1:分配一个usb_driver结构体*/
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
	/*1:设置usb_driver结构体*/
	/*1:注册usb_driver结构体*/
	usb_register(&myuvc_driver);
	return 0;
}

static void myuvc_exit(void)
{
	usb_deregister(&myuvc_driver);
}

module_init(myuvc_init);
module_exit(myuvc_exit);
MODULE_LICENSE("GPL");
