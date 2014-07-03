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
	.probe = ,	//��ʾ����������֧���豸ʱ,�͵���probe����.
	.disconnect = ,//����USB�豸�����,�����ٹ���.
	.id_table = ,	//��ʾ֧����Щ�豸
};
*/

static int myuvc_probe(struct usb_interface *intf,
		     const struct usb_device_id *id)
{
	static int cnt = 0;
	//ͨ��usb_interface USB�ӿڵõ�usb_device�ṹ,usb_device�ṹ����usb_device_descriptor�ṹ
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_device_descriptor *descriptor = &udev->descriptor;//USB�豸������

	int i;
	struct usb_host_config *hostconfig;
	struct usb_config_descriptor *config;//����������
	
	printk("myuvc_probe:cnt = %d\n",cnt++);
	//�ο�lsusb.c->dump_device()��ӡ�豸������.
	printk("Device Descriptor:\n"
	       "  bLength             %5u\n"//�������ĳ��ȣ��豸�������ĳ���Ϊ18���ֽڡ�
	       "  bDescriptorType     %5u\n"//�����������ͣ��豸������������Ϊ0x01��
	       "  bcdUSB              %2x.%02x\n"//USB�豸����ѭ��Э��汾�ţ���2.0Э��Ϊ0x0200��
       /*USB�豸����룬��USB-IF���䣬������ֶ�Ϊ0x00����ʾ�ɽӿ���������ָ��
          �п��ܸ�USB�豸��һ�������豸��USB�豸�ĸ����ӿ��໥�������ֱ����ڲ�ͬ���豸�ࣩ��
          �����0x01~0xfe����ʾΪUSB-IF������豸�࣬����0x03ΪHID�豸��0x09ΪHUB�豸�����
          ��0xff����ʾ�ɳ����Զ����豸���͡�*/
	       "  bDeviceClass        %5u \n"

		/*USB������룬��USB-IF���䣬���bDeviceClassΪ0x00����ô���ֶ�Ҳ����Ϊ 0x00��
		����������Բο�USB���ڶ���USB Device Class�Ķ��塣*/
	       "  bDeviceSubClass     %5u \n"

		/*Э����룬��USB-IF���䣬���bDeviceClass��bDeviceSubClass����Ϊ0x00����ô��
	       �ֶ�Ҳ����Ϊ0x00��*/
	       "  bDeviceProtocol     %5u \n"
	       "  bMaxPacketSize0     %5u\n"//�˵�0������ݰ����ȣ�����Ϊ8��16��32��64.
	       "  idVendor           0x%04x \n"//����ID����USB-IF���䣬��Ҫ��USB-IF��֯���롣
	       "  idProduct          0x%04x \n"//��ƷID���ɳ���ָ����
	       "  bcdDevice           %2x.%02x\n"//�豸���кţ��ɳ����������á�
	       "  iManufacturer       %5u \n"//�����������̵��ַ���������������
	       "  iProduct            %5u \n"//����������Ʒ���ַ���������������

		/*����������Ʒ���кŵ��ַ���������������ע�⣬���е��ַ����������ǿ�ѡ�ģ����û��
	       �ַ�����������ָ����Щ����Ϊ0x00��*/
	       "  iSerial             %5u \n"
	       
	       "  bNumConfigurations  %5u\n",//����������������
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
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) },//��2Ϊ��Ƶ���ƽӿ�
	//����ֻҪ����һ����Ƶ���ƽӿ�,�Ժ����ͨ����Ƶ���ƽӿ��ҵ�USBӲ������Ƶ���ӿ�.
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) },//��2Ϊ��Ƶ���ӿ�
	{}
};


/*1:����һ��usb_driver�ṹ��*/
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
	/*1:����usb_driver�ṹ��*/
	/*1:ע��usb_driver�ṹ��*/
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
