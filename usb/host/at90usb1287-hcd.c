/*
  //
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file: at90usb1287-hcdi.c
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

#include "host.h"
#include <wait.h>
#include <stdlib.h>

#include "at90usb1287.h"
#include <avr/io.h>

#include "uart.h"
#include "wait.h"

#include <core/core.h>
#include <usbspec/usb11spec.h>

void at90usb1287_roothub_probe();
void at90usb1287_roothub_check();

void at90usb1287_start_transfer();

usb_device * device_on_downstream;


/* cuurent transferdescriptor on port a and port b */
usb_transfer_descriptor * td_usba;
usb_transfer_descriptor * td_usbb;

usb_driver at90usb1287_roothub = {
  .name	  = "at90usb1287_roothub",
  .probe  = at90usb1287_roothub_probe,
  .check  = at90usb1287_roothub_check,
  .data	  = NULL,
};

usb_device * device_on_downstream;

/**
 * Find and initial root hub
 */
void at90usb1287_roothub_probe()
{
  // called on n every new enumeration and at usb_register_driver  
  // der sollte nach dem ersten aufruf igonriert werden
  #if DEBUG
  core.stdout("Probe: AT90USB1287 Root Hub\r\n");
  #endif
}

/**
 * This function is called periodical, to notice
 * port changes after an hub
 */
void at90usb1287_roothub_check()
{
  //USART_WriteHex(UHINT);
  //UHINT = 0x00;
  // disconnect 
  if(UHINT & 0x02) {
    device_on_downstream=NULL;
    #if DEBUG
    core.stdout("Device: Removed!\r\n");
    #endif
    //UHINT &= ~(1<<DDISCI);
  } else {
    // if connect 
    if(device_on_downstream!=NULL) {
      // normal transfer check for interrupt and isochonous endpoints
      //USART_WriteHex(0x88);
    } 
    // check if device is connected 
    else {
      if(UHINT & 0x01) {
	// found new device
	#if DEBUG
	core.stdout("Device: Found new one!\r\n");
	#endif

	if((USBSTA &   (1<<SPEED)) &&(USBSTA &   (1<<SPEED))){
	  // fullspeed
	  #if DEBUG
	  core.stdout("FS\r\n");
	  #endif
	} else {
	  //lowspeed
	  #if DEBUG
	  core.stdout("LS\r\n");
	  #endif
	  // mark device as low speed
	}
	
	//UHCON = (1<<SOFEN); /* host generate for FS SOF, and for LS keep-alive! */
	//UHCON = (1<<RESET);

	// SOF on etx
	device_on_downstream = 1;	
	//device_on_downstream = usb_add_device();	
	//UHINT &= ~(1<<DCONNI);
      }
      else {
	// wenn geraet nicht aktiviert ist das immer aufrufen! 
      //USART_WriteHex(0x55);
      }
    }
  }
  UHINT = 0x00;
	/* Usb_disable */
	USBCON &= ~(1<<USBE);
	/* Usb_enable */
	USBCON |= (1<<USBE);
	/* unfreeze clock */
	UsbEnableClock();
	/* usb attach */
	UDCON   &= ~(1<<DETACH);
	/* select host */
	USBCON  |=  (1<<HOST);
	/* disable vbus hw control */
	OTGCON  |=  (1<<VBUSHWC);
	/* enable vbus */
	OTGCON  |=  (1<<VBUSREQ);

 }


void hcdi_init()
{ 
 /* register virtual root hub driver */
  usb_register_driver(&at90usb1287_roothub);

  /* enable regulator */
  UHWCON |= (1<<UVREGE); 
  /* force host mode */
  UHWCON &= ~(1<<UIDE);
  UHWCON &= ~(1<<UIMOD);

  /* pll sart */
  UsbSetPLL_CPU_Frequency();
  UsbEnablePLL();
  UsbWaitPLL_Locked();

  /* Usb_disable */
  USBCON &= ~(1<<USBE);
  
  /* Usb_enable */
  USBCON |= (1<<USBE);
  
  /* unfreeze clock */
  UsbEnableClock();

  /* usb attach */
  UDCON   &= ~(1<<DETACH);

  /* enable uconv pin */
  UsbDisableUVCON_PinControl();
  UsbEnableUID_ModeSelection();

  /* select host */
  USBCON  |=  (1<<HOST);

  /* disable vbus hw control */
  OTGCON  |=  (1<<VBUSHWC);

  // 4
  /* enable vbus */
  OTGCON  |=  (1<<VBUSREQ);
  
  UHIEN |= (1<<DDISCE);
  UHIEN |= (1<<DCONNE);

  UHIEN = 0xff;
 
 }

/**
 * hcdi_enqueue takes usb_irp and split it into
 * several usb packeges (SETUP,IN,OUT)
 * In the usbstack they are transported with the
 * usb_transfer_descriptor data structure.
 */

u8 hcdi_enqueue(usb_transfer_descriptor *td)
{
  #if LIBMODE
  td_usba = td;
  at90usb1287_start_transfer();
  #endif
  return 1;
}


u8 hcdi_dequeue(usb_transfer_descriptor *td)
{
  return 1;
}



void hcdi_irq()
{
}



void at90usb1287_start_transfer()
{
  usb_transfer_descriptor * td;
  //core.stdout("Transfer: Start!\r\n");

  #if LIBMODE
  /* choose next free port */
  td = td_usba;
  #endif

  switch(td->pid) {
    case USB_PID_SETUP:
    break;
    
    case USB_PID_IN:
    break;
    
    case USB_PID_OUT:
    break;
  }


}


