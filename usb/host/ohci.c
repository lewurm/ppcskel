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
#include "../usbspec/usb11spec.h"

/* macro for accessing u32 variables that need to be in little endian byte order;
 *
 * whenever you read or write from an u32 field that the ohci host controller
 * will read or write from too, use this macro for access!
 */
#define LE(dword) (u32)( (((dword) & 0xFF000000) >> 24) | \
			   (((dword) & 0x00FF0000) >> 8)  | \
			   (((dword) & 0x0000FF00) << 8)  | \
			   (((dword) & 0x000000FF) << 24) )

static struct endpoint_descriptor *allocate_endpoint();
static struct general_td *allocate_general_td();
static void control_quirk();
static void dbg_op_state();
//static void dbg_td_flag(u32 flag);
static void configure_ports(u8 from_init);
static void setup_port(u32 reg, u8 from_init);

static struct ohci_hcca hcca_oh0;


static struct endpoint_descriptor *allocate_endpoint()
{
	struct endpoint_descriptor *ep;
	ep = (struct endpoint_descriptor *)memalign(16, sizeof(struct endpoint_descriptor));
	memset(ep, 0, sizeof(struct endpoint_descriptor));
	ep->flags = LE(OHCI_ENDPOINT_GENERAL_FORMAT);
	ep->headp = ep->tailp = ep->nexted = LE(0);
	return ep;
}

static struct general_td *allocate_general_td()
{
	struct general_td *td;
	td = (struct general_td *)memalign(16, sizeof(struct general_td));
	memset(td, 0, sizeof(struct general_td));
	td->flags = LE(0);
	td->nexttd = LE(0);
	td->cbp = td->be = LE(0);
	return td;
}

static void control_quirk()
{
	static struct endpoint_descriptor *ed = 0; /* empty ED */
	static struct general_td *td = 0; /* dummy TD */
	u32 head;
	u32 current;
	u32 status;

	/*
	 * One time only.
	 * Allocate and keep a special empty ED with just a dummy TD.
	 */
	if (!ed) {
		ed = allocate_endpoint();
		if (!ed)
			return;

		td = allocate_general_td(0);
		if (!td) {
			free(ed);
			ed = NULL;
			return;
		}

		ed->tailp = ed->headp = LE(virt_to_phys((void*) ((u32)td & OHCI_ENDPOINT_HEAD_MASK)));
		ed->flags |= LE(OHCI_ENDPOINT_DIRECTION_OUT);
	}

	/*
	 * The OHCI USB host controllers on the Nintendo Wii
	 * video game console stop working when new TDs are
	 * added to a scheduled control ED after a transfer has
	 * has taken place on it.
	 *
	 * Before scheduling any new control TD, we make the
	 * controller happy by always loading a special control ED
	 * with a single dummy TD and letting the controller attempt
	 * the transfer.
	 * The controller won't do anything with it, as the special
	 * ED has no TDs, but it will keep the controller from failing
	 * on the next transfer.
	 */
	head = read32(OHCI0_HC_CTRL_HEAD_ED);
	if (head) {
		printf("head: 0x%08X\n", head);
		/*
		 * Load the special empty ED and tell the controller to
		 * process the control list.
		 */
		sync_after_write(ed, 16);
		sync_after_write(td, 16);
		write32(OHCI0_HC_CTRL_HEAD_ED, virt_to_phys(ed));

		status = read32(OHCI0_HC_CONTROL);
		set32(OHCI0_HC_CONTROL, OHCI_CTRL_CLE);
		write32(OHCI0_HC_COMMAND_STATUS, OHCI_CLF);

		/* spin until the controller is done with the control list */
		current = read32(OHCI0_HC_CTRL_CURRENT_ED);
		while(!current) {
			udelay(10);
			current = read32(OHCI0_HC_CTRL_CURRENT_ED);
		}

		printf("current: 0x%08X\n", current);
			
		/* restore the old control head and control settings */
		write32(OHCI0_HC_CONTROL, status);
		write32(OHCI0_HC_CTRL_HEAD_ED, head);
	} else {
		printf("nohead!\n");
	}
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

static void dbg_td_flag(u32 flag)
{
	printf("**************** dbg_td_flag: 0x%08X ***************\n", flag);
	printf("CC: %X\tshould be 0, see page 32 (ohci spec)\n", (flag>>28)&0xf);
	printf("EC: %X\tsee page 20 (ohci spec)\n", (flag>>26)&3);
	printf(" T: %X\n", (flag>>24)&3);
	printf("DI: %X\n", (flag>>21)&7);
	printf("DP: %X\n", (flag>>19)&3);
	printf(" R: %X\n", (flag>>18)&1);
	printf("********************************************************\n");
}

static void general_td_fill(struct general_td *dest, const usb_transfer_descriptor *src)
{
	if(src->actlen) {
		dest->cbp = LE(virt_to_phys(src->buffer));
		dest->be = LE(LE(dest->cbp) + src->actlen - 1);
		/* save virtual address here */
		dest->bufaddr = (u32) src->buffer;
	}
	else {
		dest->cbp = dest->be = LE(0);
		dest->bufaddr = 0;
	}

	dest->buflen = src->actlen;

	dest->flags &= LE(~OHCI_TD_DIRECTION_PID_MASK);
	switch(src->pid) {
		case USB_PID_SETUP:
			printf("pid_setup\n");
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_SETUP);
			dest->flags |= LE(OHCI_TD_TOGGLE_0);
			dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);
			break;
		case USB_PID_OUT:
			printf("pid_out\n");
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_OUT);
			dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);

			/*
			 * TODO: just temporary solution! (consider it with len?)
			 * there can be also regular PID_OUT pakets
			 */
			dest->flags |= LE(OHCI_TD_TOGGLE_1);
			break;
		case USB_PID_IN:
			printf("pid_in\n");
			dest->flags |= LE(OHCI_TD_DIRECTION_PID_IN);
			dest->flags |= LE(OHCI_TD_BUFFER_ROUNDING);
			/*
			 * let the endpoint do the togglestuff!
			 * TODO: just temporary solution!
			 * there can be also inregular PID_IN pakets (@Status Stage)
			 */
			dest->flags |= LE(OHCI_TD_TOGGLE_CARRY);
#if 0
			/* should be done by HC!
			 * first pid_in start with DATA0 */
			 */
			dummyconfig.headp = LE( src->togl ?
					LE(dummyconfig.headp) | OHCI_ENDPOINT_TOGGLE_CARRY :
					LE(dummyconfig.headp) & ~OHCI_ENDPOINT_TOGGLE_CARRY);
#endif
			break;
	}
	dest->flags |= LE(OHCI_TD_SET_DELAY_INTERRUPT(7));
}

static void dump_address(void *addr, u32 size, const char* str)
{
	printf("%s hexdump (%d) @ 0x%08X:\n", str, size, addr);
	hexdump(addr, size);
}

struct endpoint_descriptor *edhead = 0;
void hcdi_fire()
{
	printf("<^>  <^>  <^> hcdi_fire(start)\n");

	if(edhead == 0)
		return;

	control_quirk(); //required? YES! :O ... erm... or no? :/ ... in fact I have no idea
	write32(OHCI0_HC_CTRL_HEAD_ED, virt_to_phys(edhead));

	/* sync it all */
	sync_after_write(edhead, sizeof(struct endpoint_descriptor));
	dump_address(edhead, sizeof(struct endpoint_descriptor), "edhead(before)");

	struct general_td *x = phys_to_virt(LE(edhead->headp) & OHCI_ENDPOINT_HEAD_MASK);
	printf("STRUCT LEN: %d\n", sizeof(struct general_td));
	while(virt_to_phys(x)) {
		sync_after_write(x, sizeof(struct general_td));
		dump_address(x, sizeof(struct general_td), "x(before)");

		if(x->buflen > 0) {
			sync_after_write((void*) phys_to_virt(LE(x->cbp)), x->buflen);
			dump_address((void*) phys_to_virt(LE(x->cbp)), x->buflen, "x->cbp(before)");
		}
		x = phys_to_virt(LE(x->nexttd));
	}

	/* trigger control list */
	set32(OHCI0_HC_CONTROL, OHCI_CTRL_CLE);
	write32(OHCI0_HC_COMMAND_STATUS, OHCI_CLF);

	/* poll until edhead->headp is null */
	do {
		sync_before_read(edhead, sizeof(struct endpoint_descriptor));
		printf("edhead->headp: 0x%08X\n", LE(edhead->headp));
	} while(LE(edhead->headp)&~0xf);

	struct general_td *n = phys_to_virt(read32(OHCI0_HC_DONE_HEAD) & ~1);
	printf("hc_done_head: 0x%08X\n", read32(OHCI0_HC_DONE_HEAD));

	struct general_td *prev = 0, *next = 0;
	/* reverse done queue */
	while(virt_to_phys(n) && edhead->tdcount) {
		sync_before_read((void*) n, sizeof(struct general_td));
		printf("n: 0x%08X\n", n);
		printf("next: 0x%08X\n", next);
		printf("prev: 0x%08X\n", prev);

		next = n;
		n = (struct general_td*) phys_to_virt(LE(n->nexttd));
		next->nexttd = (u32) prev;
		prev = next;

		edhead->tdcount--;
	}

	n = next;
	prev = 0;
	while(virt_to_phys(n)) {
		if(prev) {
			free(prev);
		}

		dump_address(n, sizeof(struct general_td), "n(after)");

		if(n->buflen > 0) {
			sync_before_read((void*) n->bufaddr, n->buflen);
			dump_address((void*) n->bufaddr, n->buflen, "n->bufaddr(after)");
		}
		dbg_td_flag(LE(n->flags));
		prev = n;
		n = (struct general_td*) n->nexttd;
	}
	if(prev) {
		free(prev);
	}

	hcca_oh0.done_head = 0;
	sync_after_write(&hcca_oh0, sizeof(hcca_oh0));

	write32(OHCI0_HC_CONTROL, read32(OHCI0_HC_CONTROL)&~OHCI_CTRL_CLE);

	free(edhead);
	edhead = 0;

	printf("<^>  <^>  <^> hcdi_fire(end)\n");
}

/**
 * Enqueue a transfer descriptor.
 */
u8 hcdi_enqueue(const usb_transfer_descriptor *td) {
	printf("*()*()*()*()*()*()*() hcdi_enqueue(start)\n");
	if(!edhead) {
		edhead = allocate_endpoint();
		edhead->flags = LE(OHCI_ENDPOINT_GENERAL_FORMAT);
		edhead->headp = edhead->tailp = edhead->nexted = LE(0);
		edhead->flags |= LE(OHCI_ENDPOINT_LOW_SPEED |
				OHCI_ENDPOINT_SET_DEVICE_ADDRESS(td->devaddress) |
				OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(td->endpoint) |
				OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(td->maxp));
		edhead->tdcount = 0;
	}

	struct general_td *tdhw = allocate_general_td();
	general_td_fill(tdhw, td);
	edhead->tdcount ++;

	if(!edhead->headp) {
		/* first transfer */
		edhead->headp = LE(virt_to_phys((void*) ((u32)tdhw & OHCI_ENDPOINT_HEAD_MASK)));
	}
	else {
		/* headp in endpoint already exists
		 * => go to list end
		 */
		struct general_td *n = (struct general_td*) phys_to_virt(LE(edhead->headp) & OHCI_ENDPOINT_HEAD_MASK);
		while(LE(n->nexttd)) {
			n = phys_to_virt(LE(n->nexttd));
		}
		n->nexttd = LE(virt_to_phys((void*) ((u32)tdhw & OHCI_ENDPOINT_HEAD_MASK)));
		printf("n: 0x%08X\n", n);
		printf("n->nexttd: 0x%08X\n", phys_to_virt(LE(n->nexttd)));
	}

	printf("*()*()*()*()*()*()*() hcdi_enqueue(end)\n");
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
	write32(OHCI0_HC_CTRL_HEAD_ED, 0);
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
	write32(OHCI0_HC_RH_STATUS, /*RH_HS_DRWE |*/ RH_HS_LPSC);

	/* Choose the interrupts we care about now, others later on demand */
	write32(OHCI0_HC_INT_STATUS, ~0);
	write32(OHCI0_HC_INT_ENABLE, OHCI_INTR_INIT);

	//wtf?
	wait_ms ((read32(OHCI0_HC_RH_DESCRIPTOR_A) >> 23) & 0x1fe);

	configure_ports((u8)1);
	irq_restore(cookie);

	dbg_op_state();
}

static void configure_ports(u8 from_init)
{
	printf("OHCI0_HC_RH_DESCRIPTOR_A:\t0x%08X\n", read32(OHCI0_HC_RH_DESCRIPTOR_A));
	printf("OHCI0_HC_RH_DESCRIPTOR_B:\t0x%08X\n", read32(OHCI0_HC_RH_DESCRIPTOR_B));
	printf("OHCI0_HC_RH_STATUS:\t\t0x%08X\n", read32(OHCI0_HC_RH_STATUS));
	printf("OHCI0_HC_RH_PORT_STATUS_1:\t0x%08X\n", read32(OHCI0_HC_RH_PORT_STATUS_1));
	printf("OHCI0_HC_RH_PORT_STATUS_2:\t0x%08X\n", read32(OHCI0_HC_RH_PORT_STATUS_2));

	setup_port(OHCI0_HC_RH_PORT_STATUS_1, from_init);
	setup_port(OHCI0_HC_RH_PORT_STATUS_2, from_init);
	printf("configure_ports done\n");
}

static void setup_port(u32 reg, u8 from_init)
{
	u32 port = read32(reg);
	if((port & RH_PS_CCS) && ((port & RH_PS_CSC) || from_init)) {
		write32(reg, RH_PS_CSC);

		wait_ms(120);

		/* clear CSC flag, set PES and start port reset (PRS) */
		write32(reg, RH_PS_PES);
		while(!(read32(reg) & RH_PS_PES)) {
			printf("fu\n");
			return;
		}

		write32(reg, RH_PS_PRS);

		/* spin until port reset is complete */
		while(!(read32(reg) & RH_PS_PRSC)); // hint: it may stuck here
		printf("loop done\n");

		(void) usb_add_device();
	}
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
	if (flags == 0) {
		printf("OHCI Interrupt occured: but not for you! WTF?!\n");
		return;
	}

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
		configure_ports(0);
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
		/* basically the linux irq handler reverse TDs to their urbs
		 * and set done_head to null.
		 * since we are polling atm, just should do the latter task.
		 * however, this won't work for now (i don't know why...)
		 * TODO!
		 */
#if 0
		sync_before_read(&hcca_oh0, 256);
		hcca_oh0.done_head = 0;
		sync_after_write(&hcca_oh0, 256);
#endif
	}

	/* TODO: handle any pending URB/ED unlinks... */

#define HC_IS_RUNNING() 1 /* dirty, i know... just a temporary solution */
	if (HC_IS_RUNNING()) {
		write32(OHCI0_HC_INT_STATUS, flags);
		write32(OHCI0_HC_INT_ENABLE, OHCI_INTR_MIE);
	}
}

void show_frame_no()
{
	sync_before_read(&hcca_oh0, 256);
	printf("***** frame_no: %d *****\n", LE(hcca_oh0.frame_no));
}
