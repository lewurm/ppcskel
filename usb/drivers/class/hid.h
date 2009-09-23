#ifndef __HID_H
#define __HID_H

#define MOD_lctrl (1<<0)
#define MOD_lshift (1<<1)
#define MOD_lalt (1<<2)
#define MOD_lwin (1<<3)
#define MOD_rctrl (1<<4)
#define MOD_rshift (1<<5)
#define MOD_ralt (1<<6)
#define MOD_rwin (1<<7)

struct kbrep {
	u8 mod;
	u8 reserved;
	u8 keys[6];
};

void usb_hidkb_probe();
void usb_hidkb_check();
void usb_hidkb_init();

struct kbrep *usb_hidkb_getChars();

#endif /* __HID_H */

