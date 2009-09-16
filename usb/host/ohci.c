/*
       ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
       ohci hardware support

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "../../bootmii_ppc.h"
#include "../../hollywood.h"
#include "../../irq.h"
#include "../../string.h"
#include "../../malloc.h"
#include "ohci.h"
#include "host.h"

static struct ohci_hcca hcca_oh0;
static struct endpoint_descriptor *dummyconfig;

static struct endpoint_descriptor *allocate_endpoint()
{
	struct endpoint_descriptor *ep;
	ep = (struct endpoint_descriptor *)calloc(sizeof(struct endpoint_descriptor), 16);
	ep->flags = OHCI_ENDPOINT_GENERAL_FORMAT;
	ep->headp = ep->tailp = ep->nexted = 0;
	return ep;
}

static struct general_td *allocate_general_td(size_t bsize)
{
	struct general_td *td;
	td = (struct general_td *)calloc(sizeof(struct general_td), 16);
	td->flags = 0;
	td->nexttd = 0;
	if(bsize == 0) {
		td->cbp = td->be = 0;
	} else {
		td->cbp = (u32)malloc(bsize);
		td->be = td->cbp + bsize - 1;
	}
	return td;
}

static void dbg_op_state() 
{
	switch (read32(OHCI0_HC_CONTROL) & OHCI_CTRL_HCFS) {
		case OHCI_USB_SUSPEND:
			printf("ohci-- OHCI_USB_SUSPEND\n");
			break;
		case OHCI_USB_RESET:
			printf("ohci-- OHCI_USB_RESET\n");
			break;
		case OHCI_USB_OPER:
			printf("ohci-- OHCI_USB_OPER\n");
			break;
		case OHCI_USB_RESUME:
			printf("ohci-- OHCI_USB_RESUME\n");
			break;
	}
}


/**
 * Enqueue a transfer descriptor.
 */
u8 hcdi_enqueue(usb_transfer_descriptor *td) {
	printf("===========================\ndone head (vor sync): 0x%08X\n", hcca_oh0.done_head);
	sync_before_read(&hcca_oh0, 256);
	printf("done head (nach sync): 0x%08X\n", hcca_oh0.done_head);

	struct general_td *tmptd = allocate_general_td(sizeof(td->buffer));
	(void) memcpy((void*) tmptd->cbp, td->buffer, sizeof(td->buffer));

	printf("tmptd hexump (before):\n");
	hexdump((void*) tmptd, sizeof(tmptd));
	printf("tmptd-cbp hexump (before):\n");
	hexdump((void*) (tmptd->cbp), sizeof(tmptd->cbp));

	sync_after_write((void*) (tmptd->cbp), sizeof(tmptd->cbp));
	sync_after_write(tmptd, sizeof(tmptd));

	dummyconfig->headp = virt_to_phys(tmptd);
	sync_after_write(dummyconfig, 64);

	printf("+++++++++++++++++++++++++++++\n");
	udelay(2000);
	udelay(2000);
	udelay(2000);
	udelay(2000);
	udelay(2000);
	udelay(2000);
	udelay(2000);
	udelay(2000);

	sync_before_read(tmptd, sizeof(tmptd));
	printf("tmptd hexump (after):\n");
	hexdump((void*) tmptd, sizeof(tmptd));

	sync_before_read((void*) (tmptd->cbp), sizeof(tmptd->cbp));
	printf("tmptd-cbp hexump (after):\n");
	hexdump((void*) (tmptd->cbp), sizeof(tmptd->cbp));

	printf("done head (vor sync): 0x%08X\n", hcca_oh0.done_head);
	sync_before_read(&hcca_oh0, 256);
	printf("done head (nach sync): 0x%08X\n", hcca_oh0.done_head);
	return 0;
}

/**
 * Remove an transfer descriptor from transfer queue.
 */
u8 hcdi_dequeue(usb_transfer_descriptor *td) {
	return 0;
}

void hcdi_init() 
{
	dummyconfig = allocate_endpoint();
	printf("ohci-- init\n");
	dbg_op_state();

	/* disable hc interrupts */
	set32(OHCI0_HC_INT_DISABLE, OHCI_INTR_MIE);

	/* save fmInterval and calculate FSMPS */
#define FSMP(fi) (0x7fff & ((6 * ((fi) - 210)) / 7))
#define FI 0x2edf /* 12000 bits per frame (-1) */
	u32 fmint = read32(OHCI0_HC_FM_INTERVAL) & 0x3fff;
	if(fmint != FI)
		printf("ohci-- fminterval delta: %d\n", fmint - FI);
	fmint |= FSMP (fmint) << 16;

	/* enable interrupts of both usb host controllers */
	set32(EHCI_CTL, EHCI_CTL_OH0INTE | EHCI_CTL_OH1INTE | 0xe0000);

	/* reset HC */
	write32(OHCI0_HC_COMMAND_STATUS, OHCI_HCR);

	/* wait max. 30us */
	u32 ts = 30;
	while ((read32(OHCI0_HC_COMMAND_STATUS) & OHCI_HCR) != 0) {
		 if(--ts == 0) {
			printf("ohci-- FAILED");
			return;
		 }
		 udelay(1);
	}

	/* disable interrupts; 2ms timelimit here! 
	   now we're in the SUSPEND state ... must go OPERATIONAL
	   within 2msec else HC enters RESUME */

	u32 cookie = irq_kill();

	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */
	sync_after_write(dummyconfig, 64);
	write32(OHCI0_HC_CTRL_HEAD_ED, virt_to_phys(dummyconfig));
	write32(OHCI0_HC_BULK_HEAD_ED, 0);

	/* set hcca adress */
	sync_after_write(&hcca_oh0, 256);
	write32(OHCI0_HC_HCCA, virt_to_phys(&hcca_oh0));

	/* set periodicstart */
#define FIT (1<<31)
	u32 fmInterval = read32(OHCI0_HC_FM_INTERVAL) &0x3fff;
	u32 fit = read32(OHCI0_HC_FM_INTERVAL) & FIT;

	write32(OHCI0_HC_FM_INTERVAL, fmint | (fit ^ FIT));
	write32(OHCI0_HC_PERIODIC_START, ((9*fmInterval)/10)&0x3fff);

	/* testing bla */
	if ((read32(OHCI0_HC_FM_INTERVAL) & 0x3fff0000) == 0 || !read32(OHCI0_HC_PERIODIC_START)) {
		printf("ohci-- w00t, fail!! see ohci-hcd.c:669\n");
	}
	
	/* start HC operations */
	write32(OHCI0_HC_CONTROL, OHCI_CONTROL_INIT | OHCI_USB_OPER);

	/* wake on ConnectStatusChange, matching external hubs */
	set32(OHCI0_HC_RH_STATUS, RH_HS_DRWE);

	/* Choose the interrupts we care about now, others later on demand */
	write32(OHCI0_HC_INT_STATUS, ~0);
	write32(OHCI0_HC_INT_ENABLE, OHCI_INTR_INIT);

	irq_restore(cookie);

	dbg_op_state();
}

void hcdi_irq()
{
	/* read interrupt status */
	u32 flags = read32(OHCI0_HC_INT_STATUS);

	/* when all bits are set to 1 some problem occured */
	if (flags == 0xffffffff) {
		printf("ohci-- Houston, we have a serious problem! :(\n");
		return;
	}

	/* only care about interrupts that are enabled */
	flags &= read32(OHCI0_HC_INT_ENABLE);

	/* nothing to do? */
	if (flags == 0)
		return;

	printf("OHCI Interrupt occured: ");
	/* UnrecoverableError */
	if (flags & OHCI_INTR_UE) {
		printf("UnrecoverableError\n");
		/* TODO: well, I don't know... nothing,
		 *       because it won't happen anyway? ;-) */
	}

	/* RootHubStatusChange */
	if (flags & OHCI_INTR_RHSC) {
		printf("RootHubStatusChange\n");
		/* TODO: set some next_statechange variable... */
		write32(OHCI0_HC_INT_STATUS, OHCI_INTR_RD | OHCI_INTR_RHSC);
	}
	/* ResumeDetected */
	else if (flags & OHCI_INTR_RD) {
		printf("ResumeDetected\n");
		write32(OHCI0_HC_INT_STATUS, OHCI_INTR_RD);
		/* TODO: figure out what the linux kernel does here... */
	}

	/* WritebackDoneHead */
	if (flags & OHCI_INTR_WDH) {
		printf("WritebackDoneHead\n");
		/* TODO: figure out what the linux kernel does here... */
	}

	/* TODO: handle any pending URB/ED unlinks... */

#define HC_IS_RUNNING() 1 /* dirty, i know... just a temporary solution */
	if (HC_IS_RUNNING()) {
		write32(OHCI0_HC_INT_STATUS, flags);
		write32(OHCI0_HC_INT_ENABLE, OHCI_INTR_MIE);
	}
}

