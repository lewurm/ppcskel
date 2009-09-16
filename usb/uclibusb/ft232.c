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

#include <lib/types.h>

#include <core/usb.h>
#include <usbspec/usb11spec.h>

#include "ft232.h"

/** 
 * Find and open an FT232 on bus.
 */
usb_device * usb_ft232_open()
{
  usb_device * dev = usb_open(0x0403,0x6001);
  char tmp[8];

  if(dev!=NULL) {
    usb_control_msg(dev, 0x00,SET_CONFIGURATION,0x0100, 0, 0,tmp, 8, 0);

    usb_control_msg(dev, 0x40, 0,0, 0,  0,tmp, 8, 0);
    usb_control_msg(dev, 0x40, 4,8, 0,  0,tmp, 8, 0);
    usb_control_msg(dev, 0x40, 3,0x4138, 0,  0,tmp, 8, 0);
    usb_control_msg(dev, 0x40, 1,0x0303, 0,  0,tmp, 8, 0);
    usb_control_msg(dev, 0x40, 2,0, 0,  0,tmp, 8, 0);
    usb_control_msg(dev, 0x40, 1,0x0303, 0,  0,tmp, 8, 0);
  } else {
    dev = NULL;
  }
  return dev;
}

/**
 * Close the opened FT232.
 */
void usb_ft232_close(usb_device *dev)
{
  return;
}

/**
 * Send a buffer.
 */
u8 usb_ft232_send(usb_device *dev,char *bytes, u8 length)
{
  char tmp[8];
  usb_bulk_write(dev, 2, bytes, length, 0);
  usb_control_msg(dev, 0x40, 2,0, 0,  0,tmp, 8, 0);
  usb_control_msg(dev, 0x40, 1,0x0300, 0,  0,tmp, 8, 0);
  return 1;
}

/**
 * Receive some values.
 */
u8 usb_ft232_receive(usb_device *dev,char *bytes, u8 length)
{
  usb_bulk_read(dev, 1,  bytes, length,0);
  return 1;
}
