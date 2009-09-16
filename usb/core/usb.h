/*
 * Copyright (c) 2006, Benedikt Sauter <sauter@ixbat.de>
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

#ifndef _USB_H_
#define _USB_H_

#include "../../types.h"
#include "core.h"


/******************* Device Operations **********************/

// use an own usb device
usb_device * usb_open(u32 vendor_id, u32 product_id);
usb_device * usb_open_class(u8 class);

u8 usb_close(usb_device *dev);



u8 usb_get_device_descriptor(usb_device *dev, char *buf,u8 size);
u8 usb_set_address(usb_device *dev, u8 address);
u8 usb_set_configuration(usb_device *dev, u8 configuration);
u8 usb_set_altinterface(usb_device *dev, u8 alternate);


/**
 * usb_reset resets the specified device by sending a RESET down the port 
 * it is connected to. Returns 0 on success or < 0 on error.
 */

u8 usb_reset(usb_device *dev);


/******************* Control Transfer **********************/


u8 usb_control_msg(usb_device *dev, u8 requesttype, u8 request, u16 value, u16 index, u16 length, char *buf, u16 size, u16 timeout);
u8 usb_get_string(usb_device *dev, u8 index, u8 langid, char *buf, u8 buflen);
u8 usb_get_string_simple(usb_device *dev, u8 index, char *buf, u8 buflen);
u8 usb_get_descriptor(usb_device *dev, unsigned char type, unsigned char index, void *buf, u8 size);


/******************* Bulk Transfer **********************/

u8 usb_bulk_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);
u8 usb_bulk_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);


/******************* Interrupt Transfer **********************/
u8 usb_u8errupt_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);
u8 usb_u8errupt_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);


/******************* Isochron Transfer **********************/
u8 usb_isochron_write(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);
u8 usb_isochron_read(usb_device *dev, u8 ep, char *buf, u8 size, u8 timeout);



#endif	//_USB_H_
