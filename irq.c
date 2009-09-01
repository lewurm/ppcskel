/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn IOS.
	IRQ support

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009	Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "irq.h"
#include "hollywood.h"
#include "ipc.h"
#include "bootmii_ppc.h"
//debug only
#include "printf.h"

void irq_initialize(void)
{
	// enable OHCI0 interrupt on hollywood-pic
	write32(HW_PPCIRQMASK, 0);
	write32(HW_PPCIRQFLAG, 0xffffffff);

	// enable RESET and PIC (#14) interrupts on processor interface
	write32(BW_PI_IRQFLAG, 0xffffffff);
#define BW_PI_IRQ_RESET 	1
#define BW_PI_IRQ_HW 		14
	write32(BW_PI_IRQMASK, (1<<BW_PI_IRQ_RESET) | (1<<BW_PI_IRQ_HW));

	//??? -- needed?!
	write32(HW_PPCIRQMASK+0x04, 0);
	write32(HW_PPCIRQMASK+0x20, 0);

	_CPU_ISR_Enable()
}

void irq_shutdown(void)
{
	write32(HW_PPCIRQMASK, 0);
	write32(HW_PPCIRQFLAG, 0xffffffff);
	irq_kill();
}

void irq_handler(void)
{
	u32 enabled = read32(HW_PPCIRQMASK);
	u32 flags = read32(HW_PPCIRQFLAG);
	
	printf("In IRQ handler: 0x%08x 0x%08x 0x%08x\n", enabled, flags, flags & enabled);

	flags = flags & enabled;

	if(flags & IRQF_TIMER) {
		// done by mini already? 
		/*
		if (_alarm_frequency) {
			// currently we use the alarm timer only for lame usbgecko polling
			gecko_timer();
			write32(HW_ALARM, read32(HW_TIMER) + _alarm_frequency);
		}
		*/
		write32(HW_PPCIRQFLAG, IRQF_TIMER);
	}
	if(flags & IRQF_NAND) {
//		printf("IRQ: NAND\n");
		// hmmm... should be done by mini?
		write32(NAND_CMD, 0x7fffffff); // shut it up
		write32(HW_PPCIRQFLAG, IRQF_NAND);
		//nand_irq();
	}
	if(flags & IRQF_GPIO1B) {
//		printf("IRQ: GPIO1B\n");
		// hmmm... should be done by mini?
		write32(HW_GPIO1BINTFLAG, 0xFFFFFF); // shut it up
		write32(HW_PPCIRQFLAG, IRQF_GPIO1B);
	}
	if(flags & IRQF_GPIO1) {
//		printf("IRQ: GPIO1\n");
		// hmmm... should be done by mini?
		write32(HW_GPIO1INTFLAG, 0xFFFFFF); // shut it up
		write32(HW_PPCIRQFLAG, IRQF_GPIO1);
	}
	if(flags & IRQF_RESET) {
//		printf("IRQ: RESET\n");
		write32(HW_PPCIRQFLAG, IRQF_RESET);
	}
	if(flags & IRQF_IPC) {
		//printf("IRQ: IPC\n");
		//not necessary here?
		//ipc_irq();
		write32(HW_PPCIRQFLAG, IRQF_IPC);
	}
	if(flags & IRQF_AES) {
//		printf("IRQ: AES\n");
		write32(HW_PPCIRQFLAG, IRQF_AES);
	}
	if (flags & IRQF_SDHC) {
//		printf("IRQ: SDHC\n");
		write32(HW_PPCIRQFLAG, IRQF_SDHC);
		//sdhc_irq();
	}
	if (flags & IRQF_OHCI0) {
		printf("IRQ: OHCI0\n");
		write32(HW_PPCIRQFLAG, IRQF_OHCI0);
		//TODO: ohci0_irq();
	}
	if (flags & IRQF_OHCI1) {
		printf("IRQ: OHCI1\n");
		write32(HW_PPCIRQFLAG, IRQF_OHCI1);
		//TODO: ohci1_irq();
	}
	
	flags &= ~IRQF_ALL;
	if(flags) {
		printf("IRQ: unknown 0x%08x\n", flags);
		write32(HW_PPCIRQFLAG, flags);
	}
}

void irq_enable(u32 irq)
{
	set32(HW_PPCIRQMASK, 1<<irq);
}

void irq_disable(u32 irq)
{
	clear32(HW_PPCIRQMASK, 1<<irq);
}

inline u32 irq_kill() {
	u32 cookie;
	_CPU_ISR_Disable(cookie);
	return cookie;
}

inline void irq_restore(u32 cookie) {
	_CPU_ISR_Restore(cookie);
}

