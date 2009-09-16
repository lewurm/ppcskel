/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file:
 *
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *	 * Redistributions of source code must retain the above copyright 
 *		 notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above 
 *		 copyright notice, this list of conditions and the following 
 *		 disclaimer in the documentation and/or other materials provided 
 *		 with the distribution.
 *	 * Neither the name of the FH Augsburg nor the names of its 
 *		 contributors may be used to endorse or promote products derived 
 *		 from this software without specific prior written permission.
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

#ifndef _CORE_H_
#define _CORE_H_

#include "../../types.h"
#include "../lib/list.h"


#include "../../bootmii_ppc.h"
inline static void wait_ms(int ms)
{
	int i=0;
	for(;i<ms;i++)
		udelay(1000);
}

/**
 * Main datastructure of the usbstack, which have to be instanced
 * in the main application.
 */

typedef struct usb_device_t usb_device;
struct usb_device_t {
	u8  address;
	u8  fullspeed;
	u8  bMaxPacketSize0;
	u8  bDeviceClass;
	u8  bDeviceSubClass;
	u8  bDeviceProtocoll;
	u32 idVendor;
	u32 idProduct;
	u32 bcdDevice;
	u8  bNumConfigurations;

	u8 epSize[16];
	u8 epTogl[16];

	usb_device *next;
};


typedef struct usb_endpoint_t usb_endpoint;
struct usb_endpoint_t {
	u8 type;
	u8 size;
	u8 togl;
	usb_endpoint *next;
};




typedef struct usb_transfer_descriptor_ep_t usb_transfer_descriptor_ep;
struct usb_transfer_descriptor_ep_t {
	usb_transfer_descriptor_ep *next;
	u8 device_address;
	u8 endpoint;
	struct usb_transfer_descriptor_t *start;
};

/**
 * USB Driver data structure
 */
typedef struct usb_driver_t usb_driver;
struct usb_driver_t {
	char* name;
	void (*probe)(void);
	void (*check)(void);
	void * data;
	usb_driver *next;
};


/**
 * I/O Request Block
 */

typedef struct usb_irp_t usb_irp;
struct usb_irp_t {
	usb_device * dev;
	u8 endpoint;				/* ep -> bit 7 is for direction 1=from	dev to host */
	u8 epsize;
	u8 type;				/* control, interrupt, bulk or isochron */

	char * buffer;
	u16 len;

	//list * td_list;
	u16 timeout;
};


/**
 * usb transfer descriptor
 */
typedef struct usb_transfer_descriptor_t usb_transfer_descriptor;
struct usb_transfer_descriptor_t {
	u8 devaddress;
	u8 endpoint;
	
	// TODO: zusammenfassen!
	u8 pid;
	u8 iso;
	u8 togl;	
	
	char * buffer;
	u16 actlen;
	
	u8 state;
	usb_transfer_descriptor *next;
	u8 maxp;
};

//typedef struct usb_core_t usb_core;
struct usb_core_t {
	u8 nextaddress;
	void (*stdout)(char * arg); 
	// driver list
	list * drivers;
	list * devices;
} core;

void usb_init();
void usb_periodic();
u8 usb_next_address();


usb_device * usb_add_device();
u8 usb_remove_device(usb_device *dev);
u8 usb_register_driver(usb_driver *driver);
void usb_probe_driver();



usb_irp * usb_get_irp();
u8 usb_remove_irp(usb_irp *irp);
u16 usb_submit_irp(usb_irp *irp);


usb_transfer_descriptor * usb_create_transfer_descriptor(usb_irp *irp);


#define USB_IRP_WAITING		1


#define USB_TRANSFER_DESCR_NONE 1
#define USB_TRANSFER_DESCR_SEND 2

#endif	//_CORE_H_
