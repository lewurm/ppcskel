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

//#include <stdlib.h>
#include "core.h"
#include "../host/host.h"
#include "usb.h"
#include "../usbspec/usb11spec.h"
#include "../lib/list.h"
#include "../../malloc.h"

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
  dev->bMaxPacketSize0 = 8;     /* send at first time only 8 bytes */

  dev->epSize[0] = 64;
  dev->epSize[1] = 64;
  dev->epSize[2] = 64;

  dev->epTogl[0] = 0;
  dev->epTogl[1] = 0;
  dev->epTogl[2] = 0;

  char buf[64];
  u8 devdescr_size;
  u8 address = usb_next_address();

  /* ask first 8 bytes of device descriptor with this special 
   * GET Descriptor Request, when device address = 0
   */
  usb_control_msg(dev, 0x80, GET_DESCRIPTOR, 1, 0, 64, buf, 8, 0);

  dev->bMaxPacketSize0 = (u8) buf[7];   /* setup real ep0 fifo size */
  devdescr_size = (u8) buf[0];  /* save real length of device descriptor */

  /* define new adress */
  usb_control_msg(dev, 0x00, SET_ADDRESS, address << 8, 0, 0, buf, 8, 0);
  dev->address = address;

  /* get complete device descriptor */
  usb_control_msg(dev, 0x80, GET_DESCRIPTOR, 1, 0, devdescr_size, buf, 8,
                  0);

  /* save only really neccessary values for this small usbstack */
  dev->bDeviceClass = (u8) buf[4];
  dev->bDeviceSubClass = (u8) buf[5];
  dev->bDeviceProtocoll = (u8) buf[6];
  dev->idVendor = (u16) (buf[9] << 8) | (buf[8]);
  dev->idProduct = (u16) (buf[11] << 8) | (buf[10]);
  dev->bcdDevice = (u16) (buf[13] << 8) | (buf[12]);

  // string descriptoren werden nicht im arbeitsspeicher gehalten -> on demand mit 
  // entprechenden funktionen
  // hier muss man noch mehr abholen, konfigurationene, interfaces und endpunkte

  /* add device to device list */
  element *tmp = (element *) malloc(sizeof(element));
  tmp->data = (void *) dev;
  list_add_tail(core.devices, tmp);

  usb_probe_driver();

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
u16 usb_submit_irp(usb_irp * irp)
{
  usb_transfer_descriptor *td;
  u8 runloop = 1;
  u16 restlength = irp->len;
  char *td_buf_ptr = irp->buffer;

  //u8 togl=irp->dev->epTogl[(irp->endpoint & 0x7F)];
  u8 togl = irp->dev->epTogl[(irp->endpoint & 0x7F)];
  //u8 togl=0;

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
    td->actlen = 8;             /* control message are always 8 bytes */

    togl = 0;
    td->togl = togl;            /* start with data0 */
    if (togl == 0)
      togl = 1;
    else
      togl = 0;
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
       *  Device to Host
       *  - es kommen noch Daten mit PID_IN an
       *  - host beendet mit PID_OUT DATA1 Zero
       * bit7 - 0
       *  Host zu Device (wie set address)
       *  - device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
       */
    usb_device_request *setup = (usb_device_request *) irp->buffer;
    u8 bmRequestType = setup->bmRequestType;

    if (bmRequestType & 0x80) { /* check bit 7 of bmRequestType */

      /* schleife die die tds generiert */
      while (runloop) {
        td = usb_create_transfer_descriptor(irp);
        td->actlen = irp->epsize;
        /* stop loop if all bytes are send */
        if (restlength <= irp->epsize) {
          runloop = 0;
          td->actlen = restlength;
        }

        td->buffer = td_buf_ptr;
        /* move pointer for next packet */
        td_buf_ptr = td_buf_ptr + irp->epsize;

        td->pid = USB_PID_IN;
        td->togl = togl;
        if (togl == 0)
          togl = 1;
        else
          togl = 0;

        /* wenn device descriptor von adresse 0 angefragt wird werden nur
         * die ersten 8 byte abgefragt
         */
        if (setup->bRequest == GET_DESCRIPTOR && (setup->wValue >> 8) == 1
            && td->devaddress == 0) {
          runloop = 0;          /* stop loop */
        }

          /**** send token ****/
        hcdi_enqueue(td);

        /* pruefe ob noch weitere Pakete vom Device abgeholt werden muessen */
        restlength = restlength - irp->epsize;
      }
    }




      /***************** Status Stage ***********************/
    /* Zero packet for end */
    td = usb_create_transfer_descriptor(irp);
    td->togl = 1;               /* zero data packet = always DATA1 packet */
    td->actlen = 0;
    td->buffer = NULL;

      /**
       * bit7 = 1, host beendet mit PID_OUT DATA1 Zero
       * bit7 = 0, device sendet ein PID_IN DATA1 Zero Packet als bestaetigung
       */
    if (bmRequestType & 0x80) { /* check bit 7 of bmRequestType */
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
      td->endpoint = td->endpoint & 0x7F;       /* clear direction bit */

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

  return td;
}
