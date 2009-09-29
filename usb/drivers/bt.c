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

struct usb_driver btdriv = {
	.name	  = "bt",
	.probe  = usb_bt_probe,
	.check  = usb_bt_check,
	.remove = usb_bt_remove,
	.data	  = NULL
};

u8 epi_it, epi_bk, epo_bk;


void usb_bt_init()
{
	usb_register_driver(&btdriv);

	usb_set_configuration(btdriv.data, btdriv.data->conf->bConfigurationValue);
	printf("get_conf: %d\n", usb_get_configuration(btdriv.data));

	u8 buf[8];
	memset(buf, 0, 8);
	usb_control_msg(btdriv.data, 0x01, SET_INTERFACE, 0, 1, 0, buf, 0);

	epi_it = btdriv.data->conf->intf->endp->bEndpointAddress & 0x7F;
	epi_bk = btdriv.data->conf->intf->endp->next->bEndpointAddress & 0x7F;
	epo_bk = btdriv.data->conf->intf->endp->next->next->bEndpointAddress & 0x7F;

	btdriv.data->epSize[1] = btdriv.data->conf->intf->endp->wMaxPacketSize;
	btdriv.data->epSize[2] = btdriv.data->conf->intf->endp->next->wMaxPacketSize;
	btdriv.data->epSize[3] = btdriv.data->conf->intf->endp->next->next->wMaxPacketSize;
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
			btdriv.data = dev;
		}

		iterator=iterator->next;
	}
}

void usb_bt_check()
{
}

u8 usb_bt_inuse()
{
	return btdriv.data ? 1 : 0;
}

void usb_bt_remove()
{
	btdriv.data = NULL;
}

