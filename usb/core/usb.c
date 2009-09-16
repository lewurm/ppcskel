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

//wtf??!
//#ifndef _USB_H_
//#define _USB_H_

//#include <stdlib.h>

#include "usb.h"
#include "core.h"
#include "../host/host.h"
#include "../usbspec/usb11spec.h"
#include "../../malloc.h"


/******************* Device Operations **********************/

/**
 * Open a device with verndor- and product-id for a communication.
 */
usb_device * usb_open(u32 vendor_id, u32 product_id)
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
usb_device * usb_open_class(u8 class)
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

/**
 * Close device after a communication.
 */
u8 usb_close(usb_device *dev)
{

	return 0;
}

u8 usb_get_device_descriptor(usb_device *dev, char *buf,u8 size)
{

	return 0;
}

u8 usb_set_address(usb_device *dev, u8 address)
{

	return 0;
}

u8 usb_set_configuration(usb_device *dev, u8 configuration)
{

	return 0;
}

u8 usb_set_altinterface(usb_device *dev, u8 alternate)
{

	return 0;
}

u8 usb_reset(usb_device *dev)
{


	return 0;
}


/******************* Control Transfer **********************/

/**
 * Create a control transfer.
 */
u8 usb_control_msg(usb_device *dev, u8 requesttype, u8 request, u16 value, u16 index, u16 length,char *buf, u16 size, u16 timeout)
{
	usb_irp *irp = (usb_irp*)malloc(sizeof(usb_irp));
	irp->dev = dev;
	//irp->devaddress = dev->address;
	irp->endpoint = 0;
	
	irp->epsize = dev->bMaxPacketSize0;
	irp->type = USB_CTRL;

	buf[0]=(char)requesttype;
	buf[1]=(char)request;		 
	buf[2]=(char)(value >> 8);
	buf[3]=(char)(value);		
	buf[4]=(char)(index >> 8);
	buf[5]=(char)(index);	
	// lenght buf are the only where the order is inverted
	buf[6]=(char)(length);
	buf[7]=(char)(length >> 8);

	irp->buffer = buf;
	irp->len = length;
	irp->timeout = timeout;

	usb_submit_irp(irp);
	free(irp);

	return 0;
}


u8 usb_get_string(usb_device *dev, u8 index, u8 langid, char *buf, u8 buflen)
{

	return 0;
}


u8 usb_get_string_simple(usb_device *dev, u8 index, char *buf, u8 buflen)
{

	return 0;
}

u8 usb_get_descriptor(usb_device *dev, unsigned char type, unsigned char index, void *buf, u8 size)
{

	return 0;
}


/******************* Bulk Transfer **********************/

/**
 * Write to an a bulk endpoint.
 */
u8 usb_bulk_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
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
u8 usb_bulk_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
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
u8 usb_interrupt_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
{

	return 0;
}

/**
 * Read from an interrupt endpoint.
 */
u8 usb_interrupt_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
{

	return 0;
}


/******************* Isochron Transfer **********************/

/**
 * Write to an isochron endpoint.
 */
u8 usb_isochron_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
{

	return 0;
}

/**
 * Read from an isochron endpoint.
 */
u8 usb_isochron_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout)
{


	return 0;
}


//#endif	//_USB_H_
