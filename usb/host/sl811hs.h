/*
 * Copyright (c) 2007, Benedikt Sauter <sauter@ixbat.de>
 * All rights reserved.
 *
 * Short descripton of file (sl811hs.h):
 * Register definitions of SL811, created from the datasheet
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

#ifndef _SL811HS_H
#define _SL811HS_H

/* prototypes for communication functions */
void sl811_init();
void sl811_reset();
void sl811_write(u8 addr, u8 data);
u8 sl811_read(u8 addr);

void sl811_write_burst(u8 data);
u8 sl811_read_burst();

void sl811_write_buf(u8 addr, unsigned char *buffer,u8 size);
void sl811_read_buf(u8 addr, unsigned char *buffer,u8 size);

/*
 *  * ScanLogic SL811HS/T USB Host Controller
 *   */

#define SL811_IDX_ADDR (0x00)
#define SL811_IDX_DATA (0x01)
#define SL811_PORTSIZE (0x02)

#define SL811_E0BASE (0x00)    /* Base of Control0 */
#define SL811_E0CTRL (0x00)    /* Host Control Register */
#define SL811_E0ADDR (0x01)    /* Host Base Address */
#define SL811_E0LEN  (0x02)    /* Host Base Length */
#define SL811_E0STAT (0x03)    /* USB Status (Read) */
#define SL811_E0PID  SL811_E0STAT	/* Host PID, Device Endpoint (Write) */
#define SL811_E0CONT (0x04)    /* Transfer Count (Read) */
#define SL811_E0DEV  SL811_E0CONT	/* Host Device Address (Write) */

#define SL811_E1BASE (0x08)    /* Base of Control1 */
#define SL811_E1CTRL (SL811_E1BASE + SL811_E0CTRL)
#define SL811_E1ADDR (SL811_E1BASE + SL811_E0ADDR)
#define SL811_E1LEN  (SL811_E1BASE + SL811_E0LEN)
#define SL811_E1STAT (SL811_E1BASE + SL811_E0STAT)
#define SL811_E1PID  (SL811_E1BASE + SL811_E0PID)
#define SL811_E1CONT (SL811_E1BASE + SL811_E0CONT)
#define SL811_E1DEV  (SL811_E1BASE + SL811_E0DEV)

#define SL811_CTRL (0x05)    /* Control Register1 */
#define SL811_IER  (0x06)    /* Interrupt Enable Register */
#define SL811_ISR  (0x0d)    /* Interrupt Status Register */
#define SL811_DATA (0x0e)    /* SOF Counter Low (Write) */
#define SL811_REV  SL811_DATA /* HW Revision Register (Read) */
#define SL811_CSOF  (0x0f)    /* SOF Counter High(R), Control2(W) */
#define SL811_MEM  (0x10)    /* Memory Buffer (0x10 - 0xff) */

#define SL811_EPCTRL_ARM	  (0x01)
#define SL811_EPCTRL_ENABLE  (0x02)
#define SL811_EPCTRL_DIRECTION (0x04)
#define SL811_EPCTRL_ISO	  (0x10)
#define SL811_EPCTRL_SOF	  (0x20)
#define SL811_EPCTRL_DATATOGGLE	(0x40)
#define SL811_EPCTRL_PREAMBLE  (0x80)

#define SL811_EPPID_PIDMASK  (0xf0)
#define SL811_EPPID_EPMASK (0x0f)

#define SL811_EPSTAT_ACK	  (0x01)
#define SL811_EPSTAT_ERROR (0x02)
#define SL811_EPSTAT_TIMEOUT (0x04)
#define SL811_EPSTAT_SEQUENCE  (0x08)
#define SL811_EPSTAT_SETUP (0x10)
#define SL811_EPSTAT_OVERFLOW  (0x20)
#define SL811_EPSTAT_NAK	  (0x40)
#define SL811_EPSTAT_STALL (0x80)

#define SL811_CTRL_ENABLESOF (0x01)
#define SL811_CTRL_EOF2	  (0x04)
#define SL811_CTRL_RESETENGINE (0x08)
#define SL811_CTRL_JKSTATE (0x10)
#define SL811_CTRL_LOWSPEED  (0x20)
#define SL811_CTRL_SUSPEND (0x40)

#define SL811_IER_USBA	(0x01)	/* USB-A done */
#define SL811_IER_USBB	(0x02)	/* USB-B done */
#define SL811_IER_BABBLE	  (0x04)  /* Babble detection */
#define SL811_IER_SOFTIMER (0x10)  /* 1ms SOF timer */
#define SL811_IER_INSERT	  (0x20)  /* Slave Insert/Remove detection */
#define SL811_IER_RESET	  (0x40)  /* USB Reset/Resume */

#define SL811_ISR_USBA	(0x01)	/* USB-A done */
#define SL811_ISR_USBB	(0x02)	/* USB-B done */
#define SL811_ISR_BABBLE	  (0x04)  /* Babble detection */
#define SL811_ISR_SOFTIMER (0x10)  /* 1ms SOF timer */
#define SL811_ISR_INSERT	  (0x20)  /* Slave Insert/Remove detection */
#define SL811_ISR_RESET	  (0x40)  /* USB Reset/Resume */
#define SL811_ISR_DATA	(0x80)	/* Value of the Data+ pin */

#define SL811_REV_USBA	(0x01)	/* USB-A */
#define SL811_REV_USBB	(0x02)	/* USB-B */
#define SL811_REV_REVMASK  (0xf0)  /* HW Revision */
#define SL811_REV_REVSL811H (0x00)  /* HW is SL811H */
#define SL811_REV_REVSL811HS (0x10)  /* HW is SL811HS */

#define SL811_CSOF_SOFMASK  (0x3f)  /* SOF High Counter */
#define SL811_CSOF_POLARITY (0x40)  /* Change polarity */
#define SL811_CSOF_MASTER (0x80)  /* Master/Slave selection */


#define cMemStart 0x10
#define ubufA	  0x80
#define ubufB	  0xc0
#define	uxferLen  0x40
#define sMemSize  0xc0
#define	cMemEnd	  256


#define EP0Buf	0x40
#define EP0Len	0x40

#define DATA0_WR  0x07
#define DATA1_WR  0x47

#define ZDATA0_WR 0x05
#define ZDATA1_WR 0x45

#define DATA0_RD  0x03
#define DATA1_RD  0x43

#define PID_SOF	  0x50
#define PID_SETUP 0xd0
#define PID_IN	  0x90
#define PID_OUT	  0x10
#define PID_PRE	  0xc0
#define PID_NAK	  0xa0
#define PID_STALL 0xe0
#define PID_DATA0 0x30
#define PID_DATA1 0xb0


#endif /* _SL811HS_H */

