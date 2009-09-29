/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	bluetooth driver

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "../core/core.h"
#include "../core/usb.h"
#include "../usbspec/usb11spec.h"
#include "../../malloc.h"
#include "../../string.h"

#include "bt.h"

#define USB_CREQ_D2H 0x80
#define USB_CREQ_STANDARD 0x00
#define USB_CREQ_VENDOR 0x40
#define USB_CREQ_INTERFACE 0x01
#define USB_CREQ_ENDPOINT 0x02
#define USB_CREQ_OTHER 0x03

#define SWAB16(x) ((((x)&0xFF)<<8)|((x)>>8))
#define EP_CONTROL 0x00
#define EP_EVENTS 0x81
#define EP_ACL_OUT 0x02
#define EP_ACL_IN 0x82

#define HCI_G_LINKCONTROL 1
#define HCI_G_LINKPOLICY 2
#define HCI_G_CONTROLLER 3
#define HCI_G_INFORMATIONAL 4
#define HCI_G_STATUS 5
#define HCI_G_TESTING 6

#define HCI_C_RESET 0x0003
#define HCI_LC_CONNECT 0x0005

#define HCI_PKTTYPE_DM1 0x0008
#define HCI_PSRM_R2 2
#define HCI_CLKOFF_INVALID 0
#define HCI_NO_ROLESWITCH 0

#define HCI_EV_CONNECTION_COMPLETE 0x03


typedef struct {
	u8 event_code;
	u8 data_length;
	u8 *data;
} HCI_Event;

typedef struct {
	u16 chnd;
	int pb,bc;
	u16 data_length;
	u8 *data;
} HCI_ACL_Data;

int bt_HCI_reset(struct usb_device *dev);
int bt_HCI_recv_event(struct usb_device *dev, HCI_Event *ev);
int bt_HCI_connect(struct usb_device *dev, u8 *bdaddr, u16 pkt_types, u8 psrm, u16 clkoff, u8 roleswitch);
int bt_L2CAP_send(struct usb_device *dev, u16 chnd, u16 cid, u16 length, u8 *data);
int bt_HCI_recv_ACL_async(struct usb_device *dev, HCI_ACL_Data *acl);

struct usb_driver btd = {
	.name	  = "bt",
	.probe  = usb_bt_probe,
	.check  = usb_bt_check,
	.remove = usb_bt_remove,
	.data	  = NULL
};

u8 epi_it, epi_bk, epo_bk;


void usb_bt_init()
{
	usb_register_driver(&btd);

	usb_set_configuration(btd.data, btd.data->conf->bConfigurationValue);
	printf("get_conf: %d\n", usb_get_configuration(btd.data));

	u8 buf[128];
	memset(buf, 0, 8);
	usb_control_msg(btd.data, 0x01, SET_INTERFACE, 0, 1, 0, buf, 0);

	epi_it = btd.data->conf->intf->endp->bEndpointAddress & 0x7F;
	epi_bk = btd.data->conf->intf->endp->next->bEndpointAddress & 0x7F;
	epo_bk = btd.data->conf->intf->endp->next->next->bEndpointAddress & 0x7F;

	btd.data->epSize[1] = btd.data->conf->intf->endp->wMaxPacketSize;
	btd.data->epSize[2] = btd.data->conf->intf->endp->next->wMaxPacketSize;
	btd.data->epSize[3] = btd.data->conf->intf->endp->next->next->wMaxPacketSize;
	

	/* shit */
	// Bluetooth HCI reset command
	u8 puf[8] ALIGNED(0x40);
	memset(buf, 0, sizeof(buf));

	memset(puf, 0, sizeof(puf));
	memcpy(puf,"\x03\x0c\x00",3);
	// Bluetooth request to control endpoint
	u16 ret = usb_control_msg_pl(btd.data, 0x20, 0, 0, 0, 3, buf, 0, puf);
	printf("request control endpoint:\n");
	hexdump((void*) buf, 11);

	ret = bt_HCI_reset(btd.data);
	printf("HCI reset to *dev returned %d\n",ret);

	HCI_Event hciev;
	HCI_ACL_Data acldat;
	ret = bt_HCI_recv_event(btd.data, &hciev);
	printf("after recv event\n");

	ret = bt_HCI_connect(btd.data, (u8*)"\x00\x17\xAB\x33\x37\x65", HCI_PKTTYPE_DM1, HCI_PSRM_R2, HCI_CLKOFF_INVALID, HCI_NO_ROLESWITCH);
	printf("HCI connect to returned %d\n",ret);
	ret = bt_HCI_recv_event(btd.data, &hciev);
	while(1) {
		ret = bt_HCI_recv_event(btd.data, &hciev);
		if(hciev.event_code == HCI_EV_CONNECTION_COMPLETE) {
			break;
		}
	}
	if(hciev.data[0]) {
		printf("Connection failed!\n");
	} else {
		u16 chnd;
		chnd = hciev.data[1] | (hciev.data[2]<<8);
		printf("Connection successful! chnd: 0x%04x\n",chnd);
		ret = bt_L2CAP_send(btd.data, chnd, 1, 8, (u8*)"\x02\x01\x04\x00\x13\x00\x41\x00");
		printf("L2CAP send to returned %d\n", ret);
		bt_HCI_recv_ACL_async(btd.data, &acldat);
		bt_HCI_recv_ACL_async(btd.data, &acldat);

		u8 dcid[256];
		u8 l2pkt[256];
		memset(dcid, 0, 256);
		memset(l2pkt, 0, 256);

		memcpy(&dcid, &acldat.data[8], 2);
		memcpy(l2pkt, "\x04\x01\x04\x00\xAA\xAA\x00\x00", 8);
		memcpy(&l2pkt[4], &dcid, 2);
		ret = bt_L2CAP_send(btd.data, chnd, 1, 8, l2pkt);
		printf("L2CAP send to returned %d\n", ret);
		bt_HCI_recv_ACL_async(btd.data, &acldat);
		bt_HCI_recv_ACL_async(btd.data, &acldat);
		memcpy(l2pkt, "\x05\x01\x06\x00\xAA\xAA\x00\x00\x00\x00", 10);
		memcpy(&l2pkt[4], &dcid, 2);
		ret = bt_L2CAP_send(btd.data, chnd, 1, 10, l2pkt);
		printf("L2CAP send to %d returned %d\n", btd.data, ret);
		while(1) {
			bt_HCI_recv_ACL_async(btd.data, &acldat);
		}
	}
}

void usb_bt_probe()
{
	struct usb_device *dev;
	struct element *iterator = core.devices->head;

	while(iterator != NULL) {
		dev = (struct usb_device*) iterator->data;
		if(dev == NULL) {
			continue;
		}

		if(dev->idVendor == 0x057e && dev->idProduct == 0x0305) {
			btd.data = dev;
		}

		iterator=iterator->next;
	}
}

void usb_bt_check()
{
}

u8 usb_bt_inuse()
{
	return btd.data ? 1 : 0;
}

void usb_bt_remove()
{
	btd.data = NULL;
}

int bt_HCI_command(struct usb_device *dev, int ogf, int ocf, u8 *parameters, u8 parmlength)
{
	int opcode;
	static u8 buffer[0x103] ALIGNED(0x40);
	opcode = (ocf&0x3FF) | ((ogf &0x3F)<<10);
	buffer[0] = opcode&0xFF;
	buffer[1] = opcode>>8;
	buffer[2] = parmlength;
	
	if(parameters && parmlength) {
		memcpy (&buffer[3], parameters, parmlength);
	} else {
		parmlength = 0; //make sure we don't pass around junk
	}
	printf("paramlength(should not >0xFF !): %x\n", parmlength+3);
	
	u8 buf[0x40];
	memset(buf, 0, 0x40);
	return usb_control_msg_pl(dev, 0x20, 0, 0, 0, parmlength+3, buf, 0, buffer);
}


int bt_HCI_recv_event(struct usb_device *dev, HCI_Event *ev)
{
	static u8 buffer[0x102] ALIGNED(0x40);
	s8 res;
	
	printf("WTF1\n");
	res = usb_interrupt_read(dev, epi_it, buffer, sizeof(buffer), 0);
	printf("WTF2\n");
	ev->event_code = buffer[0];
	ev->data_length = buffer[1];
	ev->data = &buffer[2];
	printf("HCI event [%d]: Code 0x%x, length %d, data:\n",res, ev->event_code, ev->data_length);
	hexdump(ev->data, ev->data_length);
	return res;
}

int bt_HCI_reset(struct usb_device *dev)
{
	return bt_HCI_command(dev, HCI_G_CONTROLLER, HCI_C_RESET, NULL, 0);
}

int bt_HCI_connect(struct usb_device *dev, u8 *bdaddr, u16 pkt_types, u8 psrm, u16 clkoff, u8 roleswitch)
{
	static u8 data[13];
	int i;
	for(i=0;i<6;i++)
		data[i] = bdaddr[5-i];
	data[6] = pkt_types & 0xFF;
	data[7] = pkt_types >> 8;
	data[8] = psrm;
	data[9] = 0; //reserved
	data[10] = clkoff & 0xFF;
	data[11] = clkoff >> 8;
	data[12] = roleswitch;
	
	return bt_HCI_command(dev, HCI_G_LINKCONTROL, HCI_LC_CONNECT, data, sizeof(data));
}

int bt_HCI_send_ACL(struct usb_device *dev, u16 chnd, int pb, int bc, u16 length, u8 *data)
{
	static u8 buffer[0x100] ALIGNED(0x40);
	printf("<ACL chnd %04x pb %d bc %d len %d data:\n",chnd,pb,bc,length);
	hexdump(data,length);
	chnd &= 0x0FFF;
	chnd |= pb<<12;
	chnd |= bc<<14;
	memcpy(&buffer[4],data,length);
	buffer[0] = chnd & 0xFF;
	buffer[1] = chnd >> 8;
	buffer[2] = length & 0xFF;
	buffer[3] = length >>8;
	return usb_bulk_write(dev, EP_ACL_OUT, buffer, length+4, 0);
}

int bt_HCI_recv_ACL(struct usb_device *dev, HCI_ACL_Data *acl)
{
	static u8 buffer[0x40] ALIGNED(0x40);
	int res;
	
	res = usb_bulk_read(dev, EP_ACL_IN, buffer, sizeof(buffer), 0);
	acl->chnd = buffer[0] | (buffer[1]<<8);
	acl->pb = (acl->chnd & 0x3000)>>12;
	acl->bc = (acl->chnd & 0xC000)>>14;
	acl->chnd &= 0x0FFF;
	acl->data_length = buffer[2] | (buffer[3]<<8);
	acl->data = &buffer[4];
	printf(">ACL [%d]: chnd %04x pb %d bc %d len %d data:\n",res,acl->chnd, acl->pb, acl->bc, acl->data_length);
	hexdump(acl->data, acl->data_length);
	return res;
}

int bt_HCI_recv_ACL_async(struct usb_device *dev, HCI_ACL_Data *acl)
{
	static u8 buffer[0x40] ALIGNED(0x40);
	int res;

	res = usb_bulk_read(btd.data, EP_ACL_IN, buffer, sizeof(buffer), 0);

	acl->chnd = buffer[0] | (buffer[1]<<8);
	acl->pb = (acl->chnd & 0x3000)>>12;
	acl->bc = (acl->chnd & 0xC000)>>14;
	acl->chnd &= 0x0FFF;
	acl->data_length = buffer[2] | (buffer[3]<<8);
	acl->data = &buffer[4];
	printf(">ACL [%d]: chnd %04x pb %d bc %d len %d data:\n",res,acl->chnd, acl->pb, acl->bc, acl->data_length);
	hexdump(acl->data, acl->data_length);
	return res;
}

int bt_L2CAP_send(struct usb_device *dev, u16 chnd, u16 cid, u16 length, u8 *data)
{
	static u8 buffer[0x1000] ALIGNED(0x20);
	memcpy(&buffer[4],data,length);
	buffer[0] = length & 0xFF;
	buffer[1] = length >> 8;
	buffer[2] = cid & 0xFF;
	buffer[3] = cid >> 8;
	return bt_HCI_send_ACL(dev, chnd, 2, 0, length+4, buffer);
}

