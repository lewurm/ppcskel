/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file: sl811hs-hcdi.c
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
#include "sl811hs.h"
#include <wait.h>
#include <stdlib.h>

#include "uart.h"
#include "wait.h"

#include <core/core.h>
//#include <class/hub.h>
#include <usbspec/usb11spec.h>

void sl811_roothub_probe();
void sl811_roothub_check();

void sl811_start_transfer();

usb_device * device_on_downstream;


/* cuurent transferdescriptor on port a and port b */
usb_transfer_descriptor * td_usba;
usb_transfer_descriptor * td_usbb;

usb_driver sl811_roothub = {
  .name	  = "sl811_roothub",
  .probe  = sl811_roothub_probe,
  .check  = sl811_roothub_check,
  .data	  = NULL,
};

/**
 * Find and initial root hub
 */
void sl811_roothub_probe()
{
  // called on n every new enumeration and at usb_register_driver  
  // der sollte nach dem ersten aufruf igonriert werden
  // oder diese funktion bleibt einfach leer
  #if DEBUG
  core.stdout("Probe: SL811 Root Hub\r\n");
  #endif
}


/**
 * This function is called periodical, to notice
 * port changes after an hub
 */
void sl811_roothub_check()
{
  /* hier muss man nur dafuer sorgen wenn ein geaert angesteckt
   * wird, dass der entsprechende port auf reset gesetzt wird
   * damit das device die adresse 0 annimmt
   * und dann muss man usb_add_device aufrufen
   *
   * wenn ein geraet entfernt wird muss man nur usb_remove_device aufrufen
   * und es muss dabei das richtige geraet angegeben werden.
   * dieses muss man sich wahrscheinlich intern im treiber
   * merken...
   */
  // check for new device 
  u16 *port_change = (u16*)sl811_roothub.data;
  
  u8 status = sl811_read(SL811_ISR);
  sl811_write(SL811_ISR,SL811_ISR_DATA | SL811_ISR_SOFTIMER);

  #define HUB_PORTSTATUS_C_PORT_CONNECTION 1
  if((status & SL811_ISR_RESET)) {  // TODO und bit x von CTRL
    // remove device if neccessary
    if(device_on_downstream!=NULL){
      #if USBMON
      core.stdout("Remove Device!\r\n");
      #endif
      usb_remove_device(device_on_downstream);
      device_on_downstream=NULL;
    }
    
    sl811_write(SL811_ISR,SL811_ISR_RESET);
  } else {
    if((port_change[0] & HUB_PORTSTATUS_C_PORT_CONNECTION)){
      #if USBMON
      core.stdout("Find new Device!\r\n");
      #endif
      
      /* init sof currently for fullspeed  (datasheet page 11)*/
      sl811_write(SL811_CSOF,0xAE);
      sl811_write(SL811_DATA,0xE0);

      /* reset device that function can answer to address 0 */
      sl811_write(SL811_IER,0x00);
      sl811_write(SL811_CTRL,SL811_CTRL_ENABLESOF|SL811_CTRL_RESETENGINE);
      sl811_write(SL811_ISR,0xff);
      wait_ms(20);

      /* start SOF generation */
      sl811_write(SL811_CTRL,SL811_CTRL_ENABLESOF);
      sl811_write(SL811_ISR,0xff);
      sl811_write(SL811_E0BASE,SL811_EPCTRL_ARM);
      wait_ms(50);
      
      device_on_downstream = usb_add_device();

      /* set internate port state 1=device is online */
      port_change[0]=0x00;

    }
  }

  if((status & SL811_ISR_INSERT)){
    port_change[0] |= HUB_PORTSTATUS_C_PORT_CONNECTION;
    sl811_write(SL811_ISR,SL811_ISR_INSERT);
  }

}


void hcdi_init()
{
  /* find and initial host controller */
  sl811_init();
  u8 rev = sl811_read(SL811_REV)>>4;

  switch(rev) {
    case 1:
      #if USBMON
      core.stdout("Host: SL811HS v1.2 found\r\n");
      #endif
    break;
    case 2:
      #if USBMON
      core.stdout("Host: SL811HS v1.5 found\r\n");
      #endif
    break;
    default:
      #if USBMON 
      core.stdout("Can't find SL811!\r\n"); 
      #endif
      return;
  }
 
  /* Disable interrupt, then wait 40 ms */
  sl811_write(SL811_IER,0x00);

  /* Initialize controller */
  //sl811_write(SL811_CSOF,0xae);
  sl811_write(SL811_CSOF,SL811_CSOF_MASTER);

  /* clear interrupt status register with one read operation */
  sl811_write(SL811_ISR,0xff);

  /* data = hub flags */
  u16 *port_change = (u16*)malloc(sizeof(u16));
  port_change[0] = 0x00;
  port_change[1] = 0x00;
  sl811_roothub.data = (void*)port_change;
  device_on_downstream = NULL;


  /* register virtual root hub driver */
  usb_register_driver(&sl811_roothub);

  
  /* activate interrupts */
  sl811_write(SL811_IER,SL811_IER_USBA);
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
  sl811_start_transfer();
  #endif
  return 1;
}


u8 hcdi_dequeue(usb_transfer_descriptor *td)
{
  return 1;
}



void hcdi_irq()
{
  core.stdout("interrupt\r\n");
  u8 state;
  state = sl811_read(SL811_ISR);
  if(state & SL811_ISR_USBA) {
    core.stdout("a done\r\n");
  }
  if(state & SL811_ISR_USBB) {
    core.stdout("b done\r\n");
  }
  if(state & SL811_ISR_RESET) {
    core.stdout("reset\r\n");
  }
  
  if(state & SL811_ISR_INSERT) {
    core.stdout("insert\r\n");
  }

  sl811_write(SL811_ISR,0xFF);
}



void sl811_start_transfer()
{
  usb_transfer_descriptor * td;

  #if LIBMODE
  /* choose next free port */
  td = td_usba;
  /* disable a done interrupt */
  sl811_write(SL811_IER,0x00);	
  #endif

  #if USBMON
    //core.stdout("");
  #endif

  sl811_write(SL811_E0CONT,td->devaddress);	/* device address */
  sl811_write(SL811_E0LEN,td->actlen);		/* number of bytes to transfer */
  sl811_write(SL811_E0ADDR,cMemStart);		/* set address to buffer in sl811 ram */

  switch(td->pid) {
    case USB_PID_SETUP:
      //core.stdout("*setup\r\n");
      /* copy data into ram of sl811 */
      sl811_write_buf(cMemStart,(unsigned char *)td->buffer,td->actlen);
      
      sl811_write(SL811_E0STAT,PID_SETUP|td->endpoint); /* set pid and ep */
      sl811_write(SL811_E0CTRL,DATA0_WR);		/* send setup packet with DATA0 */
 
      td->state = USB_TRANSFER_DESCR_SEND;

      /* wait ack */
      #if LIBMODE
      while((sl811_read(SL811_ISR)&SL811_ISR_USBA)==0);
      //wait_ms(1);
      #endif

    break;
    
    case USB_PID_IN:
      //core.stdout("*in\r\n");
      #if LIBMODE
      wait_ms(2);
      #endif
      
      sl811_write(SL811_E0STAT,PID_IN|td->endpoint); /* set pid and ep */
      sl811_write(SL811_ISR,0xff);
     
      /* choose data0 or data1 */
      if(td->togl)
	sl811_write(SL811_E0CTRL,DATA1_RD);		/* send setup packet with DATA0 */
      else
	sl811_write(SL811_E0CTRL,DATA0_RD);		/* send setup packet with DATA0 */

      td->state = USB_TRANSFER_DESCR_SEND;
  
      /* wait ack */
      #if LIBMODE
      while((sl811_read(SL811_ISR)&SL811_ISR_USBA)==0);
      //wait_ms(1);
      
      /* copy received data from internal sl811 ram */
      sl811_read_buf(cMemStart,(unsigned char *)td->buffer,td->actlen);
      
      #endif

    break;
    
    case USB_PID_OUT:
      //core.stdout("*out\r\n");
      #if LIBMODE
      wait_ms(2);
      #endif

      /* copy data into ram of sl811 */
      if(td->actlen>0)
	sl811_write_buf(cMemStart,(unsigned char *)td->buffer,td->actlen);

      sl811_write(SL811_E0STAT,PID_OUT|td->endpoint); /* set pid and ep */
  
      /* choose data0 or data1 */
      if(td->togl)
	sl811_write(SL811_E0CTRL,DATA1_WR);		/* send setup packet with DATA0 */
      else
	sl811_write(SL811_E0CTRL,DATA0_WR);		/* send setup packet with DATA0 */

      td->state = USB_TRANSFER_DESCR_SEND;

      /* wait ack */
      #if LIBMODE
      while((sl811_read(SL811_ISR)&SL811_ISR_USBA)==0);
      //wait_ms(1);
      #endif

    break;


  }


}


