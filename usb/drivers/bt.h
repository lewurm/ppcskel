/*
	ppcskel - a Free Software replacement for the Nintendo/BroadOn bootloader.
	bluetooth driver

Copyright (C) 2009     Bernhard Urban <lewurm@gmx.net>
Copyright (C) 2009     Sebastian Falbesoner <sebastian.falbesoner@gmail.com>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#ifndef __BT_H
#define __BT_H

void usb_bt_init();
void usb_bt_probe();
void usb_bt_check();
u8 usb_bt_inuse();
void usb_bt_remove();

#endif /* __BT_H */

