/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file:
 * I take the function names and parameters mainly from
 * libusb.sf.net.
 *
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution.
 *   * Neither the name of the FH Augsburg nor the names of its 
 *     contributors may be used to endorse or promote products derived 
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "usb.h"
#include "core.h"
#include "../host/host.h"
#include "../usbspec/usb11spec.h"
#include "../../malloc.h"
#include "../../string.h"


/******************* Device Operations **********************/

/**
 * Open a device with verndor- and product-id for a communication.
 */
usb_device *usb_open(u32 vendor_id, u32 product_id)
{
	usb_device* dev;
	element * iterator = core.devices->head;
	while(iterator != NULL) {
		dev = (usb_device*)iterator->data;
		
		if(dev->idVendor==vendor_id&&dev->idProduct==product_id)
			return dev;

		iterator=iterator->next;
	}

	return NULL;
}


/**
 * Open a device with an class code for a communication.
 */
usb_device *usb_open_class(u8 class)
{
	usb_device* dev;
	element * iterator = core.devices->head;
	while(iterator != NULL) {
		dev = (usb_device*)iterator->data;
		
		if(dev->bDeviceClass==class)
			return dev;

		iterator=iterator->next;
	}
	return NULL;
}

/* Close device after a communication.
 */
s8 usb_close(usb_device *dev)
{

	return 0;
}

s8 usb_reset(usb_device *dev)
{


	return 0;
}


/******************* Control Transfer **********************/

/**
 * Create a control transfer.
 */
s8 usb_control_msg(usb_device *dev, u8 requesttype, u8 request,
		u16 value, u16 index, u16 length, u8 *buf, u16 timeout)
{
	usb_irp *irp = (usb_irp*)malloc(sizeof(usb_irp));
	irp->dev = dev;
	irp->endpoint = 0;
	
	irp->epsize = dev->bMaxPacketSize0;
	irp->type = USB_CTRL;

	buf[0]=(u8)requesttype;
	buf[1]=(u8)request;
	buf[2]=(u8)(value);
	buf[3]=(u8)(value >> 8);
	buf[4]=(u8)(index);
	buf[5]=(u8)(index >> 8);
	buf[6]=(u8)(length);
	buf[7]=(u8)(length >> 8);

	irp->buffer = buf;
	irp->len = length;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}


s8 usb_get_string(usb_device *dev, u8 index, u8 langid, u8 *buf, u8 buflen)
{

	return 0;
}


char *usb_get_string_simple(usb_device *dev, u8 index, u8 *buf, u8 size)
{
	if(size < 8) {
		return (char*) -1;
	}
	usb_get_descriptor(dev, STRING, index, buf, (u8) 8);
	size = size >= (buf[0]) ? (buf[0]) : size;
	usb_get_descriptor(dev, STRING, index, buf, size);

	char *str = (char*)malloc((size/2));
	memset(str, '\0', (size/2));

	u16 i;
	for(i=0; i<(size/2)-1; i++) {
		str[i] = buf[2+(i*2)];
		printf("%c", str[i]);
	}
	printf("\n");

	return str;
}

s8 usb_get_descriptor(usb_device *dev, u8 type, u8 index, u8 *buf, u8 size)
{
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (type << 8) | index, 0, size, buf, 0);
	return 0;
}

/* ask first 8 bytes of device descriptor with this special 
 * GET Descriptor Request, when device address = 0
 */
s8 usb_get_dev_desc_simple(usb_device *dev, u8 *buf, u8 size)
{
	if(size < 8) {
		return -1;
	}
	usb_get_descriptor(dev, DEVICE, 0, buf, 8);

	if(!buf[7]) {
		printf("FU: %d\n", buf[7]);
		return -2;
	}
	dev->bMaxPacketSize0 = buf[7];
	return 0;
}

s8 usb_get_dev_desc(usb_device *dev, u8 *buf, u8 size)
{
	if (size < 0x12 || usb_get_dev_desc_simple(dev, buf, size) < 0) {
		return -1;
	}
	usb_get_descriptor(dev, DEVICE, 0, buf, size >= buf[0] ? buf[0] : size);

	dev->bLength = buf[0];
	dev->bDescriptorType = buf[1];
	dev->bcdUSB = (u16) (buf[3] << 8 | buf[2]);
	dev->bDeviceClass = buf[4];
	dev->bDeviceSubClass = buf[5];
	dev->bDeviceProtocoll = buf[6];
	dev->idVendor = (u16) (buf[9] << 8) | (buf[8]);
	dev->idProduct = (u16) (buf[11] << 8) | (buf[10]);
	dev->bcdDevice = (u16) (buf[13] << 8) | (buf[12]);
	dev->iManufacturer = buf[14];
	dev->iProduct = buf[15];
	dev->iSerialNumber = buf[16];
	dev->bNumConfigurations = buf[17];

	return 0;
}

s8 usb_get_configuration(usb_device *dev, u8 index, u8 *buf, u8 size)
{
	if(size < 8) {
		return -1;
	}
	usb_get_descriptor(dev, CONFIGURATION, index, buf, 8);
	usb_get_descriptor(dev, CONFIGURATION, index, buf, size >= buf[0] ? buf[0] : size);
	return 0;
}

s8 usb_set_address(usb_device *dev, u8 address)
{
	u8 buf[64];
	usb_control_msg(dev, 0x00, SET_ADDRESS, address, 0, 0, buf, 0);
	return 0;
}

s8 usb_set_configuration(usb_device *dev, u8 configuration)
{

	return 0;
}

s8 usb_set_altinterface(usb_device *dev, u8 alternate)
{

	return 0;
}



/******************* Bulk Transfer **********************/

/**
 * Write to an a bulk endpoint.
 */
s8 usb_bulk_write(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	usb_irp * irp = (usb_irp*)malloc(sizeof(usb_irp));
	irp->dev = dev;
	//irp->devaddress = dev->address;
	
	irp->endpoint = ep;
	irp->epsize = dev->epSize[ep]; // ermitteln
	irp->type = USB_BULK;

	irp->buffer = buf;
	irp->len = size;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}

/**
 * Read from an bulk endpoint.
 */
s8 usb_bulk_read(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{
	usb_irp * irp = (usb_irp*)malloc(sizeof(usb_irp));
	//irp->devaddress = dev->address;
	irp->dev = dev;
	
	irp->endpoint = ep | 0x80;	// from device to host
	irp->epsize = dev->epSize[ep]; // ermitteln
	irp->type = USB_BULK;

	irp->buffer = buf;
	irp->len = size;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}


/******************* Interrupt Transfer **********************/
/**
 * Write to an interrupt endpoint.
 */
s8 usb_interrupt_write(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{

	return 0;
}

/**
 * Read from an interrupt endpoint.
 */
s8 usb_interrupt_read(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{

	return 0;
}


/******************* Isochron Transfer **********************/

/**
 * Write to an isochron endpoint.
 */
s8 usb_isochron_write(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{

	return 0;
}

/**
 * Read from an isochron endpoint.
 */
s8 usb_isochron_read(usb_device *dev, u8 ep, u8 *buf, u8 size, u8 timeout)
{


	return 0;
}


//#endif	//_USB_H_
