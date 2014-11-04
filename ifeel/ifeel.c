/*
 *  Copyright (c) 2002 Joshua Bobruk
 *
 *  Logitech iFeel USB mouse 'vibrate' support
 *
 *  Adapted usb-skel and usbmouse to create driver to accept 
 *  ioctl commands for buzzing effects.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/pagemap.h>
#include "ifeel.h"

/*
 * Version Information
 */
#define DRIVER_VERSION "v0.51"
#define DRIVER_AUTHOR "Joshua Bobruk <photi@webone.com.au>"
#define DRIVER_DESC "Logitech iFeel USB mouse driver"

#define USB_IFEEL_MINOR_BASE 80
#define MAX_DEVICES 1

#define LOGITECH_VENDOR_ID            0x046d
#define LOGITECH_IFEEL_ID             0xc030

#define SAITEK_VENDOR_ID              0x06a3
#define SAITEK_TOUCH_FORCE_OPTICAL_ID 0xff05

#define BELKIN_VENDOR_ID              0x050d
#define BELKIN_NOSTROMO_N30_ID        0x0804

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

extern devfs_handle_t usb_devfs_handle;
static struct usb_ifeel  *current_dev;

struct usb_ifeel {
    signed char data[8];
    char name[128];
    struct usb_device *usbdev;
    struct input_dev dev;
    struct urb irq;
    struct urb irq_send;
    signed char irq_send_buffer[8];
    int open_count;
    devfs_handle_t		devfs;			/* devfs device node */
    struct semaphore sem;
};

/**
 *	ifeel_ioctl
 */
static int ifeel_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct usb_ifeel *dev;
	unsigned char data[] = {0x11, 0x0a, 0xff, 0xff, 0x00, 0x01, 0x00};
	struct ifeel_command command;
	
	dev = (struct usb_ifeel *)file->private_data;
	
	/* lock this object */
	down (&dev->sem);
	
	/* verify that the device wasn't unplugged */
	if (dev->usbdev == NULL) {
		up (&dev->sem);
		return -ENODEV;
	}
	
	copy_from_user(&command, (void *)arg, sizeof(command));
	
	if (cmd == USB_IFEEL_BUZZ_IOCTL) {
/*	printk("Buzzz!\n");*/
		
		memcpy(dev->irq_send.transfer_buffer, data, 7);
		((char*)dev->irq_send.transfer_buffer)[7] = 0x00;
		((char*)dev->irq_send.transfer_buffer)[2] = command.strength;
		((char*)dev->irq_send.transfer_buffer)[3] = command.delay;
		((char*)dev->irq_send.transfer_buffer)[5] = command.count;
		dev->irq_send.transfer_buffer_length = 7;
		dev->irq_send.dev = dev->usbdev;
		
		/*printk("Sending %x %x %x %x %x %x %x\n",
		  ((unsigned char*)dev->irq_send.transfer_buffer)[0], 
		  ((unsigned char*)dev->irq_send.transfer_buffer)[1],
		  ((unsigned char*)dev->irq_send.transfer_buffer)[2],
		  ((unsigned char*)dev->irq_send.transfer_buffer)[3],
		  ((unsigned char*)dev->irq_send.transfer_buffer)[4],
		  ((unsigned char*)dev->irq_send.transfer_buffer)[5],
		  ((unsigned char*)dev->irq_send.transfer_buffer)[6]
		  );*/
		
		if (usb_submit_urb(&dev->irq_send)) {
			printk("urb_submit error\n");
			return 0;
		}
		
		up (&dev->sem);
		
		return 0;
	}
	/* unlock the device */
	up (&dev->sem);    
	
	/* return that we did not understand this ioctl call */
	return -ENOTTY;
}

static void ifeel_irq_out(struct urb *urb)
{
	/* Not currently used for anything.*/
	return;
}

static void usb_ifeel_irq(struct urb *urb)
{
	struct usb_ifeel *ifeel = urb->context;
	signed char *data = ifeel->data;
	struct input_dev *dev = &ifeel->dev;
	
	if (urb->status) return;
	
	input_report_key(dev, BTN_LEFT,   data[0] & 0x01);
	input_report_key(dev, BTN_RIGHT,  data[0] & 0x02);
	input_report_key(dev, BTN_MIDDLE, data[0] & 0x04);
	input_report_key(dev, BTN_SIDE,   data[0] & 0x08);
	input_report_key(dev, BTN_EXTRA,  data[0] & 0x10);
	
	input_report_rel(dev, REL_X,     data[1]);
	input_report_rel(dev, REL_Y,     data[2]);
	input_report_rel(dev, REL_WHEEL, data[3]);
}

static int ifeel_open (struct inode *inode, struct file *file)
{
    struct usb_ifeel *dev = NULL;
    int subminor;
    int retval = 0;
	
    subminor = MINOR (inode->i_rdev) - USB_IFEEL_MINOR_BASE;
    if ((subminor < 0) ||
	(subminor >= MAX_DEVICES)) {
	return -ENODEV;
    }

    dev = current_dev;
    if (dev == NULL) {
	return -ENODEV;
    }

    /* lock this device */
    down (&dev->sem);

    /* increment our usage count for the driver */
    ++dev->open_count;

    /* save our object in the file's private structure */
    file->private_data = dev;

    /* unlock this device */
    up (&dev->sem);

    return retval;
}

static int ifeel_release (struct inode *inode, struct file *file)
{
    struct usb_ifeel *dev;
    int retval = 0;

    dev = (struct usb_ifeel *)file->private_data;
    if (dev == NULL) {
		 printk ("%s - object is NULL",__FUNCTION__);
		 return -ENODEV;
    }
	 
    /* lock our device */
    down (&dev->sem);
	 
    if (dev->open_count <= 0) {
		 printk ("%s - device not opened",__FUNCTION__);
		 retval = -ENODEV;
		 goto exit_not_opened;
    }
	 
    if (dev->usbdev == NULL) {
		 /* the device was unplugged before the file was released */
		 up (&dev->sem);
		 current_dev = NULL;
		 return 0;
    }

    /* decrement our usage count for the device */
    --dev->open_count;
    if (dev->open_count <= 0) {
	/* shutdown any bulk writes that might be going on */
	//usb_unlink_urb (dev->write_urb);
	dev->open_count = 0;
    }

 exit_not_opened:
    up (&dev->sem);

    return retval;
}

static int usb_ifeel_open(struct input_dev *dev)
{
    struct usb_ifeel *ifeel = dev->private;

    if (ifeel->open_count++)
		 return 0;
	 
    ifeel->irq.dev = ifeel->usbdev;
    if (usb_submit_urb(&ifeel->irq))
		 return -EIO;
	 
    return 0;
}

static void usb_ifeel_close(struct input_dev *dev)
{
	struct usb_ifeel *ifeel = dev->private;
	
	if (!--ifeel->open_count)
		usb_unlink_urb(&ifeel->irq);
}

static struct file_operations ifeel_fops = {
	owner:  THIS_MODULE,
	ioctl:  ifeel_ioctl,
	open:   ifeel_open,
	release:ifeel_release,
};

static void *usb_ifeel_probe(struct usb_device *dev, unsigned int ifnum,
									  const struct usb_device_id *id)
{
    struct usb_interface *iface;
    struct usb_interface_descriptor *interface;
    struct usb_endpoint_descriptor *endpoint, *sndendpoint;
    struct usb_ifeel *ifeel;
    int pipe, maxp; /* pipes for receive */
    int sndpipe, sndmaxp; /* pipes for send (buzz) */
    char *buf;
	 
    iface = &dev->actconfig->interface[ifnum];
    interface = &iface->altsetting[iface->act_altsetting];

    /* iFeel has 2 endpoints, one for irq in and one for irq out */
    if (interface->bNumEndpoints != 2) return NULL;

    endpoint = interface->endpoint + 0;
    if (!(endpoint->bEndpointAddress & 0x80)) return NULL;
    if ((endpoint->bmAttributes & 3) != 3) return NULL;

    pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
    maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

    sndendpoint = interface->endpoint + 1;

    sndpipe = usb_sndintpipe(dev, sndendpoint->bEndpointAddress);
    sndmaxp = usb_maxpacket(dev, sndpipe, usb_pipein(sndpipe));

    usb_set_idle(dev, interface->bInterfaceNumber, 0, 0);

    if (!(ifeel = kmalloc(sizeof(struct usb_ifeel), GFP_KERNEL))) return NULL;
    memset(ifeel, 0, sizeof(struct usb_ifeel));
	 
    ifeel->usbdev = dev;
	 
    ifeel->devfs = devfs_register (usb_devfs_handle, "ifeel0",
				   DEVFS_FL_DEFAULT, 180,
				   USB_IFEEL_MINOR_BASE,
				   S_IFCHR | S_IRUSR | S_IWUSR |
				   S_IRGRP | S_IWGRP | S_IROTH,
				   &ifeel_fops, NULL);


    ifeel->dev.evbit[0] = BIT(EV_KEY) | BIT(EV_REL);
    ifeel->dev.keybit[LONG(BTN_MOUSE)] = BIT(BTN_LEFT) | BIT(BTN_RIGHT) | BIT(BTN_MIDDLE);
    ifeel->dev.relbit[0] = BIT(REL_X) | BIT(REL_Y);
    ifeel->dev.keybit[LONG(BTN_MOUSE)] |= BIT(BTN_SIDE) | BIT(BTN_EXTRA);
    ifeel->dev.relbit[0] |= BIT(REL_WHEEL);

    ifeel->dev.private = ifeel;
    ifeel->dev.open = usb_ifeel_open;
    ifeel->dev.close = usb_ifeel_close;
	 
    ifeel->dev.name = ifeel->name;
    ifeel->dev.idbus = BUS_USB;
    ifeel->dev.idvendor = dev->descriptor.idVendor;
    ifeel->dev.idproduct = dev->descriptor.idProduct;
    ifeel->dev.idversion = dev->descriptor.bcdDevice;

    current_dev = ifeel;
	
    init_MUTEX (&ifeel->sem);

    if (!(buf = kmalloc(63, GFP_KERNEL))) {
		 kfree(ifeel);
		 return NULL;
    }
	 
    if (dev->descriptor.iManufacturer &&
		  usb_string(dev, dev->descriptor.iManufacturer, buf, 63) > 0)
	 {
		 strcat(ifeel->name, buf);
	 }
	 
    if (dev->descriptor.iProduct &&
		  usb_string(dev, dev->descriptor.iProduct, buf, 63) > 0)
	 {
		 sprintf(ifeel->name, "%s %s", ifeel->name, buf);
	 }
	 
	 if (!strlen(ifeel->name))
	 {
		 sprintf(ifeel->name, "USB iFeel %04x:%04x",
					ifeel->dev.idvendor, ifeel->dev.idproduct);
	 }
	 
	 
    kfree(buf);
	 
    FILL_INT_URB(&ifeel->irq, dev, pipe, ifeel->data, maxp > 8 ? 8 : maxp,
					  usb_ifeel_irq, ifeel, endpoint->bInterval);
	 
    FILL_INT_URB(&ifeel->irq_send, dev, sndpipe, ifeel->irq_send_buffer, 7, 
					  ifeel_irq_out, ifeel, 0);
	 
    input_register_device(&ifeel->dev);
	 
/* input_dev.number no longer exists */
/*     printk(KERN_INFO "input%d: %s on usb%d:%d.%d\n", */
/* 	   ifeel->dev.number, ifeel->name, dev->bus->busnum, dev->devnum, ifnum); */
	 
    printk(KERN_INFO "input: %s on usb%d:%d.%d\n",
	   ifeel->name, dev->bus->busnum, dev->devnum, ifnum);
	 
    return ifeel;
}

static void usb_ifeel_disconnect(struct usb_device *dev, void *ptr)
{
    struct usb_ifeel *ifeel = ptr;
    usb_unlink_urb(&ifeel->irq);
    input_unregister_device(&ifeel->dev);
    devfs_unregister(ifeel->devfs);
    kfree(ifeel);
}

static struct usb_device_id usb_ifeel_id_table [] = {
	{ USB_INTERFACE_INFO(3, 1, 2) },
	{ USB_DEVICE( SAITEK_VENDOR_ID, SAITEK_TOUCH_FORCE_OPTICAL_ID ) },
	{ USB_DEVICE( BELKIN_VENDOR_ID, BELKIN_NOSTROMO_N30_ID ) }
/* this causes a compiler warning */
/*     { }						/\* Terminating entry *\/ */
};

MODULE_DEVICE_TABLE (usb, usb_ifeel_id_table);


static struct usb_driver usb_ifeel_driver = {
	name:		"ifeel",
	probe:		usb_ifeel_probe,
	disconnect:	        usb_ifeel_disconnect,
	id_table:	        usb_ifeel_id_table,
	minor:		USB_IFEEL_MINOR_BASE,
	fops:               &ifeel_fops,
};

static int __init usb_ifeel_init(void)
{
	usb_register(&usb_ifeel_driver);
	info(DRIVER_VERSION ":" DRIVER_DESC);
	return 0;
}

static void __exit usb_ifeel_exit(void)
{
	usb_deregister(&usb_ifeel_driver);
}

module_init(usb_ifeel_init);
module_exit(usb_ifeel_exit);

