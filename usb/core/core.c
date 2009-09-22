/*
 * Copyright (c) 2006, Benedikt Sauter <sauter@ixbat.de>
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

//#include <stdlib.h>
#include "core.h"
#include "../host/host.h"
#include "usb.h"
#include "../usbspec/usb11spec.h"
#include "../lib/list.h"
#include "../../malloc.h"
#include "../../bootmii_ppc.h" //printf
#include "../../string.h" //memset

/**
 * Initialize USB stack.
 */
void usb_init()
{
	core.drivers = list_create();
	core.devices = list_create();
	core.nextaddress = 1;
	hcdi_init();
}

/**
 * Get next free usb device address.
 */
u8 usb_next_address()
{
	u8 addr = core.nextaddress;
	core.nextaddress++;
	return addr;
}


/**
 * Call this function periodically for 
 * control and transfer management.
 */
void usb_periodic()
{
	// call ever registered driver	
	usb_driver *drv;
	element *iterator = core.drivers->head;
	while (iterator != NULL) {
		drv = (usb_driver *) iterator->data;
		drv->check();
		iterator = iterator->next;
	}
}


/** 
 * Enumerate new device and create data structures 
 * for the core. usb_add_device expected that
 * the device answers to address zero.
 */
usb_device *usb_add_device()
{
	usb_device *dev = (usb_device *) malloc(sizeof(usb_device));
	dev->address = 0;
	/* send at first time only 8 bytes */
	dev->bMaxPacketSize0 = 8;

	dev->epSize[0] = 64;
	dev->epSize[1] = 64;
	dev->epSize[2] = 64;

	dev->epTogl[0] = 0;
	dev->epTogl[1] = 0;
	dev->epTogl[2] = 0;

#define LEN 128
	u8 buf[LEN];
	memset(buf, 0, sizeof(buf));

	s8 ret;
	memset(buf, 0, sizeof(buf));
	ret = usb_get_dev_desc_simple(dev, buf, sizeof(buf));
#ifdef _DU_CORE_ADD
	printf("=============\nbuf: 0x%08X\nafter usb_get_dev_desc_simple(ret: %d):\n", buf, ret);
	hexdump(buf, sizeof(buf));
#endif
	if(ret < 0) {
		return (void*) -1;
	}

	/* set MaxPacketSize */

	u8 address = usb_next_address();
	ret = usb_set_address(dev, address);
	dev->address = address;
#ifdef _DU_CORE_ADD
	printf("set address to %d\n", dev->address);
#endif

	memset(buf, 0, sizeof(buf));
	ret = usb_get_dev_desc(dev, buf, sizeof(buf));
#ifdef _DU_CORE_ADD
	printf("=============\nbuf: 0x%08X\nafter usb_get_dev_desc(ret: %d):\n", buf, ret);
	hexdump(buf, sizeof(buf));
#endif

	char *man, *prod, *serial;
	if(dev->iManufacturer) {
		memset(buf, 0, sizeof(buf));
		man = usb_get_string_simple(dev, dev->iManufacturer, buf, sizeof(buf));
		printf("iManufacturer:\n");
		hexdump(buf, sizeof(buf));
	} else {
		man = (char*) malloc(11);
		memset(man, '\0', sizeof(man));
		strlcpy(man, "no String", 10);
	}
	if(dev->iProduct) {
		memset(buf, 0, sizeof(buf));
		prod = usb_get_string_simple(dev, dev->iProduct, buf, sizeof(buf));
	} else {
		prod = (char*) malloc(11);
		memset(prod, '\0', sizeof(prod));
		strlcpy(prod, "no String", 10);
	}
	if(dev->iSerialNumber) {
		memset(buf, 0, sizeof(buf));
		serial = usb_get_string_simple(dev, dev->iSerialNumber, buf, sizeof(buf));
	} else {
		serial = (char*) malloc(11);
		memset(serial, '\0', sizeof(serial));
		strlcpy(serial, "no String", 10);
	}


	printf(	"bLength 0x%02X\n"
			"bDescriptorType 0x%02X\n"
			"bcdUSB 0x%02X\n"
			"bDeviceClass 0x%02X\n"
			"bDeviceSubClass 0x%02X\n"
			"bDeviceProtocoll 0x%02X\n"
			"idVendor 0x%04X\n"
			"idProduct 0x%04X\n"
			"bcdDevice 0x%04X\n"
			"iManufacturer(0x%02X): \"%s\"\n"
			"iProduct(0x%02X): \"%s\"\n"
			"iSerialNumber(0x%02X): \"%s\"\n"
			"bNumConfigurations 0x%02X\n", dev->bLength, dev->bDeviceClass,
			dev->bcdUSB, dev->bDescriptorType, dev->bDeviceSubClass, 
			dev->bDeviceProtocoll, dev->idVendor, dev->idProduct, dev->bcdDevice, 
			dev->iManufacturer, man,
			dev->iProduct, prod,
			dev->iSerialNumber, serial,
			dev->bNumConfigurations);

	memset(buf, 0, sizeof(buf));
	/* in the most cases usb devices have just one configuration descriptor */
	ret = usb_get_configuration(dev, 0, buf, sizeof(buf));
	printf("=============\nbuf: 0x%08X\nafter usb_get_configuration(ret: %d):\n", buf, ret);
	hexdump(buf, sizeof(buf));


	/*
	usb_get_descriptor(dev, DEVICE, 0, buf, 8);
	memset(buf, 0, 8);
	usb_get_descriptor(dev, DEVICE, 0, buf, size >= buf[0] ? buf[0] : size);
	*/
#if 0
	memset(buf, 0, sizeof(buf));
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (DEVICE << 8) | 0, 0, 8, buf, 0);
	printf("=============\nbuf: 0x%08X\nafter usb control msg:\n", buf);
	hexdump(buf, sizeof(buf));

	memset(buf, 0, sizeof(buf));
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (DEVICE << 8) | 0, 0, buf[0], buf, 0);
	printf("=============\nbuf: 0x%08X\nafter usb control msg:\n", buf);
	hexdump(buf, sizeof(buf));

	memset(buf, 0, sizeof(buf));
	usb_get_string_simple(dev, 1, buf);
	printf("=============\nbuf: 0x%08X\nafter usb control msg:\n", buf);
	hexdump(buf, sizeof(buf));
#endif

#if 0
	u8 devdescr_size;

	/* setup real ep0 fifo size */
	dev->bMaxPacketSize0 = (u8) buf[7];
	if(!(u8)buf[7]) {
		printf("FU\n");
		return (void*)1;
	}

	/* save real length of device descriptor */
	devdescr_size = (u8) buf[0];

	/* define new adress */
	memset(buf, 0, sizeof(buf));
	usb_control_msg(dev, 0x00, SET_ADDRESS, address, 0, 0, buf, 8, 0);
	dev->address = address;
	printf("=============\nbuf: 0x%08X\nafter usb control msg:\n", buf);
	hexdump(buf, sizeof(buf));
	printf("address: %d\n", address);


	/* get complete device descriptor */
	memset(buf, 0, sizeof(buf));
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, DEVICE<<8, 0, devdescr_size, buf, 8, 0);

	printf("=============\nbuf: 0x%08X\nafter usb control msg:\n", buf);
	hexdump(buf, sizeof(buf));

	/* save only really neccessary values for this small usbstack */
#endif

#if 0
	memset(buf, 0, sizeof(buf));
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (STRING<<8)|2, 0, 0x1a, buf, 8, 0);
	hexdump(buf, sizeof(buf));
	printf("String Descriptor [1]: ");
	u8 i;
	for (i=2; i<buf[0]; i+=2)
		printf("%c", buf[i]);
	printf("\n");
#endif

	/*
	usb_control_msg(dev, 0x80, GET_DESCRIPTOR, (STRING<<8) | 2, 0, 0x20, buf, 8, 0);
	printf("String Descriptor [2]: ");
	for (i=2; i<buf[0]; i+=2)
		printf("%c", buf[i]);
	printf("\n");
	*/

	// string descriptoren werden nicht im arbeitsspeicher gehalten -> on demand mit 
	// entprechenden funktionen
	// hier muss man noch mehr abholen, konfigurationene, interfaces und endpunkte

#if 0
	/* add device to device list */
	element *tmp = (element *) malloc(sizeof(element));
	tmp->data = (void *) dev;
	list_add_tail(core.devices, tmp);

	usb_probe_driver();
#endif

	return dev;
}

/**
 * Find currently detached device and remove
 * data structures
 */
u8 usb_remove_device(usb_device * dev)
{
	// FIXME!!!! dieser quatsch ist nur temporaer
	free(core.devices->head);
	free(core.devices);
	core.devices = list_create();
	return 1;
}

/**
 * Register new driver at usb stack.
 */
u8 usb_register_driver(usb_driver * dev)
{
	/* add driver to driver list */
	element *tmp = (element *) malloc(sizeof(element));
	tmp->data = (void *) dev;
	tmp->next = NULL;
	list_add_tail(core.drivers, tmp);


	/** 
	 * first check to find a suitable device 
	 * (root hub drivers need this call here)
	 */
	dev->probe();

	return 1;
}


/**
 * Call every probe function from every registered
 * driver, to check if there is a valid driver
 * for the new device.	
 */
void usb_probe_driver()
{
	// call ever registered driver	
	usb_driver *drv;
	element *iterator = core.drivers->head;
	while (iterator != NULL) {
		drv = (usb_driver *) iterator->data;
		drv->probe();
		iterator = iterator->next;
	}
}

/**
 * Not implemented.
 */
usb_irp *usb_get_irp()
{
	return 0;
}

/**
 * Not implemented.
 */
u8 usb_remove_irp(usb_irp * irp)
{

	return 1;
}

/**
 * Takes usb_irp and split it into
 * several usb packeges (SETUP,IN,OUT)
 * In the usbstack they are transported with the
 * usb_transfer_descriptor data structure.
 */
u16 usb_submit_irp(usb_irp *irp)
{
	usb_transfer_descriptor *td;
	u8 runloop = 1;
	u16 restlength = irp->len;
	u8 *td_buf_ptr = irp->buffer;
	u8 mybuf[64];

	u8 togl = irp->dev->epTogl[(irp->endpoint & 0x7F)];

	switch (irp->type) {
	case USB_CTRL:
		/* alle requests mit dem gleichen algorithmus zerteilen
		 * das einzige ist der spezielle get_Device_descriptor request
		 * bei dem eine laenge von 64 angegeben ist.
		 * wenn man an adresse 0 einen get_device_desciptor schickt
		 * dann reichen die ersten 8 byte.
		 */

		/***************** Setup Stage ***********************/
		td = usb_create_transfer_descriptor(irp);
		td->pid = USB_PID_SETUP;
		td->buffer = irp->buffer;

		/* control message are always 8 bytes */
		td->actlen = 8;

		togl = 0;
		/* start with data0 */
		td->togl = togl;
		togl = togl ? 0 : 1;

		/**** send token ****/
		hcdi_enqueue(td);

		/***************** Data Stage ***********************/
		/**
		 * You can see at bit 7 of bmRequestType if this stage is used,
		 * default requests are always 8 byte greate, from
		 * host to device. Stage 3 is only neccessary if the request
		 * expected datas from the device.
		 * bit7 - 1 = from device to host -> yes we need data stage
		 * bit7 - 0 = from host to device -> no send zero packet
		 *
		 * nach einem setup token kann nur ein IN token in stage 3 folgen
		 * nie aber ein OUT. Ein Zero OUT wird nur als Bestaetigung benoetigt.
		 *
		 *
		 * bit7 = 1
		 *	Device to Host
		 *	- es kommen noch Daten mit PID_IN an
		 *	- host beendet mit PID_OUT DATA1 Zero
		 * bit7 - 0
		 *	Host zu Device (wie set address)
		 *	- device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
		 */
		memcpy(mybuf, irp->buffer, td->actlen);
		usb_device_request *setup = (usb_device_request *) mybuf;
		u8 bmRequestType = setup->bmRequestType;
		free(td);

		/* check bit 7 of bmRequestType */
		if (bmRequestType & 0x80) { 
			/* schleife die die tds generiert */
			while (runloop && (restlength > 0)) {
				td = usb_create_transfer_descriptor(irp);
				td->actlen = irp->epsize;
				/* stop loop if all bytes are send */
				if (restlength < irp->epsize) {
					runloop = 0;
					td->actlen = restlength;
				}

				td->buffer = td_buf_ptr;
				/* move pointer for next packet */
				td_buf_ptr += irp->epsize;

				td->pid = USB_PID_IN;
				td->togl = togl;
				togl = togl ? 0 : 1;

				/* wenn device descriptor von adresse 0 angefragt wird werden nur
				 * die ersten 8 byte abgefragt
				 */
				if (setup->bRequest == GET_DESCRIPTOR && (setup->wValue & 0xff) == 1
						&& td->devaddress == 0) {
					/* stop loop */
					runloop = 0;
				}

				/**** send token ****/
				hcdi_enqueue(td);

				/* pruefe ob noch weitere Pakete vom Device abgeholt werden muessen */
				restlength = restlength - irp->epsize;
				free(td);
			}
		}


		/***************** Status Stage ***********************/
		/* Zero packet for end */
		td = usb_create_transfer_descriptor(irp);
		td->togl = 1;								/* zero data packet = always DATA1 packet */
		td->actlen = 0;
		td->buffer = NULL;

		/**
		 * bit7 = 1, host beendet mit PID_OUT DATA1 Zero
		 * bit7 = 0, device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
		 */
		/* check bit 7 of bmRequestType */
		if (bmRequestType & 0x80) {
			td->pid = USB_PID_OUT;
		} else {
			td->pid = USB_PID_IN;
		}
		/**** send token ****/
		hcdi_enqueue(td);
		free(td);
		break;

	case USB_BULK:
		core.stdout("bulk\r\n");
		//u8 runloop=1;
		//u16 restlength = irp->len;
		//char * td_buf_ptr=irp->buffer;

		/* schleife die die tds generiert */
		while (runloop) {

			td = usb_create_transfer_descriptor(irp);
			td->endpoint = td->endpoint & 0x7F;				/* clear direction bit */

			/* max packet size for given endpoint */
			td->actlen = irp->epsize;

			/* Generate In Packet  */
			if (irp->endpoint & 0x80)
				td->pid = USB_PID_IN;
			else
				/* Generate Out Packet */
				td->pid = USB_PID_OUT;

			/* stop loop if all bytes are send */
			if (restlength <= irp->epsize) {
				runloop = 0;
				td->actlen = restlength;
			}

			td->buffer = td_buf_ptr;
			/* move pointer for next packet */
			td_buf_ptr = td_buf_ptr + irp->epsize;

			td->togl = togl;
			if (togl == 0)
				togl = 1;
			else
				togl = 0;
				/**** send token ****/
			hcdi_enqueue(td);
			free(td);
		}
		/* next togl */
		//if(td->pid == USB_PID_OUT) {
		//if(togl==0) togl=1; else togl=0;
		//}
		irp->dev->epTogl[(irp->endpoint & 0x7F)] = togl;

		break;
	}
	hcdi_fire();

	return 1;
}



/** 
 * Create a transfer descriptor with an parent irp.
 */
usb_transfer_descriptor *usb_create_transfer_descriptor(usb_irp * irp)
{
	usb_transfer_descriptor *td =
			(usb_transfer_descriptor *) malloc(sizeof(usb_transfer_descriptor));

	td->devaddress = irp->dev->address;
	td->endpoint = irp->endpoint;
	td->iso = 0;
	td->state = USB_TRANSFER_DESCR_NONE;
	td->maxp = irp->epsize;

	return td;
}

