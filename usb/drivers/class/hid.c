/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	hid driver

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "../../core/core.h"
#include "../../core/usb.h"
#include "../../usbspec/usb11spec.h"
#include "../../../malloc.h"
#include "../../../string.h"

#include "hid.h"

struct usb_driver hidkb = {
	.name	  = "hidkb",
	.probe  = usb_hidkb_probe,
	.check  = usb_hidkb_check,
	.data	  = NULL
};

void usb_hidkb_init()
{
	usb_register_driver(&hidkb);
}


void usb_hidkb_probe()
{
	struct usb_device *dev;
	struct element *iterator = core.devices->head;
	
	while(iterator != NULL) {
		dev = (struct usb_device*)iterator->data;

		if(dev->conf->intf->bInterfaceClass == HID_CLASSCODE &&
				dev->conf->intf->bInterfaceSubClass == 1 && /* keyboard support boot protocol? */
				dev->conf->intf->bInterfaceProtocol == 1) { /* keyboard? */


			hidkb.data = (void*) dev;
		}

		iterator=iterator->next;
	}
}


void usb_hidkb_check()
{
}

struct kbrep *usb_hidkb_getChars() {
	struct usb_device *dev = (struct usb_device*) hidkb.data;
	struct kbrep *ret = (struct kbrep*) malloc(sizeof(struct kbrep));

	memset(ret, 0, 8);
	s8 epnum = dev->conf->intf->endp->bEndpointAddress & 0xf;
	(void) usb_interrupt_read(dev, epnum, (u8*) ret, 8, 0);
	printf("============\nusb_interrupt_read:\n");
	hexdump((void*)ret, 8);

	return ret;
}

