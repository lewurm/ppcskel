/* thanks to marcan for this! */
/* see @ http://www.youtube.com/watch?v=uqLrD8beikg */
#define USB_IOCTL_CTRLMSG 0
#define USB_IOCTL_BULKMSG 1
#define USB_IOCTL_INTRMSG 2
#define USB_IOCTL_GET_DEVICE_LIST 0xC

#define USB_CREQ_H2D 0x00
#define USB_CREQ_D2H 0x80
#define USB_CREQ_STANDARD 0x00
#define USB_CREQ_CLASS 0x20
#define USB_CREQ_VENDOR 0x40
#define USB_CREQ_DEVICE 0x00
#define USB_CREQ_INTERFACE 0x01
#define USB_CREQ_ENDPOINT 0x02
#define USB_CREQ_OTHER 0x03

#define SWAB16(x) ((((x)&0xFF)<<8)|((x)>>8))

#define ALIGNED(n) __attribute__((aligned(n)))


int usb_ctrl_msg(int fd, u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, void* payload, u16 wLength)
{
//	printf("USB control: %x %x %x %x %x LEN %x BUF %p\n", fd, bmRequestType, bRequest, wValue, wIndex, wLength, payload);
	return IOS_Ioctlv_Fmt(fd, USB_IOCTL_CTRLMSG, "bbhhhb:d", bmRequestType, bRequest, SWAB16(wValue), SWAB16(wIndex), SWAB16(wLength), 0, payload, wLength);
}

// the following two functions work for both reads and writes!
int usb_intr_msg(int fd, u8 bEndpoint, void* payload, u16 wLength)
{
//	printf("USB interrupt: EP %x LEN %x BUF %p\n", bEndpoint, wLength, payload);
	return IOS_Ioctlv_Fmt(fd, USB_IOCTL_INTRMSG, "bh:d", bEndpoint, wLength, payload, wLength);
}

int usb_bulk_msg(int fd, u8 bEndpoint, void* payload, u16 wLength)
{
//	printf("USB bulk: EP %x LEN %x BUF %p\n", bEndpoint, wLength, payload);
	return IOS_Ioctlv_Fmt(fd, USB_IOCTL_BULKMSG, "bh:d", bEndpoint, wLength, payload, wLength);
}

int usb_bulk_msg_async(int fd, u8 bEndpoint, void* payload, u16 wLength, ipccallback ipc_cb,void *usrdata)
{
//	printf("USB bulk: EP %x LEN %x BUF %p\n", bEndpoint, wLength, payload);
	return IOS_IoctlvAsync_Fmt(fd, USB_IOCTL_BULKMSG, ipc_cb, usrdata, "bh:d", bEndpoint, wLength, payload, wLength);
}

int usb_get_device_list(int fd, u8 type)
{
	int ret;
	static u8 rcnt[0x100] ALIGNED(0x20);
	static u8 buf[0x80] ALIGNED(0x20);
	memset(buf,0,sizeof(buf));
	rcnt[0] = 0xFF;
	ret = IOS_Ioctlv_Fmt(fd, USB_IOCTL_GET_DEVICE_LIST, "bb:bd", sizeof(buf)>>3, type, rcnt, buf, sizeof(buf));
	printf("USB dev list %d ret %d rcnt %d data:\n",type,ret,rcnt[0]);
	hexdump(buf,8);
	return ret;
}

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

int bt_HCI_command(int fd, int ogf, int ocf, u8 *parameters, u8 parmlength) {
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
	
	return usb_ctrl_msg(fd, USB_CREQ_H2D|USB_CREQ_CLASS|USB_CREQ_DEVICE, 0, 0, 0, buffer, parmlength+3);
}

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

int bt_HCI_recv_event(int fd, HCI_Event *ev) {

	static u8 buffer[0x102] ALIGNED(0x40);
	int res;
	
	res = usb_intr_msg(fd, EP_EVENTS, buffer, sizeof(buffer));
	ev->event_code = buffer[0];
	ev->data_length = buffer[1];
	ev->data = &buffer[2];
	printf("HCI event [%d]: Code 0x%x, length %d, data:\n",res,ev->event_code, ev->data_length);
	hexdump(ev->data, ev->data_length);
	return res;
}

int bt_HCI_reset(int fd) {
	
	return bt_HCI_command(fd, HCI_G_CONTROLLER, HCI_C_RESET, NULL, 0);
}

int bt_HCI_connect(int fd, u8 *bdaddr, u16 pkt_types, u8 psrm, u16 clkoff, u8 roleswitch) {
	
	static u8 data[13];
	int i;
	for(i=0;i<6;i++) data[i] = bdaddr[5-i];
	data[6] = pkt_types & 0xFF;
	data[7] = pkt_types >> 8;
	data[8] = psrm;
	data[9] = 0; //reserved
	data[10] = clkoff & 0xFF;
	data[11] = clkoff >> 8;
	data[12] = roleswitch;
	
	return bt_HCI_command(fd, HCI_G_LINKCONTROL, HCI_LC_CONNECT, data, sizeof(data));
}

int bt_HCI_send_ACL(int fd, u16 chnd, int pb, int bc, u16 length, u8 *data) {
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
	return usb_bulk_msg(fd, EP_ACL_OUT, buffer, length+4);
}

int bt_HCI_recv_ACL(int fd, HCI_ACL_Data *acl) {
	static u8 buffer[0x40] ALIGNED(0x40);
	int res;
	
	res = usb_bulk_msg(fd, EP_ACL_IN, buffer, sizeof(buffer));
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

static volatile int flag = 0;
static volatile int res;

s32 _bt_cb(int r, void *data) {
	res = r;
	flag = 1;
	return 0;
}

int bt_HCI_recv_ACL_async(int fd, HCI_ACL_Data *acl) {
	static u8 buffer[0x40] ALIGNED(0x40);
	int res;
	flag = 0;
	res = usb_bulk_msg_async(fd, EP_ACL_IN, buffer, sizeof(buffer), _bt_cb, NULL);
	while(!flag) {
		VIDEO_WaitVSync();
		PAD_ScanPads();
		int buttonsDown = PAD_ButtonsHeld(0);
		if( (buttonsDown & PAD_TRIGGER_Z) && (buttonsDown & PAD_BUTTON_START)) {
			loader();
		}
	}
	flag = 0;
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

int bt_L2CAP_send(int fd, u16 chnd, u16 cid, u16 length, u8 *data)
{
	static u8 buffer[0x1000] ALIGNED(0x20);
	memcpy(&buffer[4],data,length);
	buffer[0] = length & 0xFF;
	buffer[1] = length >> 8;
	buffer[2] = cid & 0xFF;
	buffer[3] = cid >> 8;
	return bt_HCI_send_ACL(fd, chnd, 2, 0, length+4, buffer);
}

void checkAndReload(void) {
	PAD_ScanPads();
	int buttonsDown = PAD_ButtonsHeld(0);
	if( (buttonsDown & PAD_TRIGGER_Z) && (buttonsDown & PAD_BUTTON_START)) {
		loader();
	}
}
------------
	
	/*
	// Bluetooth HCI reset command
	memcpy(buf,"\x03\x0c\x00",3);
	// Bluetooth request to control endpoint
	ret = IOS_Ioctlv_Fmt(btm_fd, 0, "bbhhhb:d", 0x20, 0, 0, 0, 0x0300, 0, buf, 3);
	printf("IOS ioctlv USB: %d\n",ret);
	*/
	/*
	if(btm_fd>0) {
		ret = bt_HCI_reset(btm_fd);
		printf("HCI reset to %d returned %d\n",btm_fd,ret);
		ret = bt_HCI_recv_event(btm_fd, &hciev);
		ret = bt_HCI_connect(btm_fd, (u8*)"\x00\x17\xAB\x33\x37\x65", HCI_PKTTYPE_DM1, HCI_PSRM_R2, HCI_CLKOFF_INVALID, HCI_NO_ROLESWITCH);
		printf("HCI connect to %d returned %d\n",btm_fd,ret);
		ret = bt_HCI_recv_event(btm_fd, &hciev);
		while(1) {
			checkAndReload();
			ret = bt_HCI_recv_event(btm_fd, &hciev);
			if(hciev.event_code == HCI_EV_CONNECTION_COMPLETE) {
				break;
			}
			VIDEO_WaitVSync();
		}
		if(hciev.data[0]) {
			printf("Connection failed!\n");
		} else {
			u16 chnd;
			chnd = hciev.data[1] | (hciev.data[2]<<8);
			printf("Connection successful! chnd: 0x%04x\n",chnd);
			ret = bt_L2CAP_send(btm_fd, chnd, 1, 8, (u8*)"\x02\x01\x04\x00\x13\x00\x41\x00");
			printf("L2CAP send to %d returned %d\n",btm_fd,ret);
			bt_HCI_recv_ACL_async(btm_fd, &acldat);
			bt_HCI_recv_ACL_async(btm_fd, &acldat);
			memcpy(&dcid, &acldat.data[8], 2);
			memcpy(l2pkt, "\x04\x01\x04\x00\xAA\xAA\x00\x00", 8);
			memcpy(&l2pkt[4], &dcid, 2);
			ret = bt_L2CAP_send(btm_fd, chnd, 1, 8, l2pkt);
			printf("L2CAP send to %d returned %d\n",btm_fd,ret);
			bt_HCI_recv_ACL_async(btm_fd, &acldat);
			bt_HCI_recv_ACL_async(btm_fd, &acldat);
			memcpy(l2pkt, "\x05\x01\x06\x00\xAA\xAA\x00\x00\x00\x00", 10);
			memcpy(&l2pkt[4], &dcid, 2);
			ret = bt_L2CAP_send(btm_fd, chnd, 1, 10, l2pkt);
			printf("L2CAP send to %d returned %d\n",btm_fd,ret);
			while(true) {
				bt_HCI_recv_ACL_async(btm_fd, &acldat);
				checkAndReload();
			}
		}
	}
	*/
	
	/*ret = IOS_Close(btm_fd);
	printf("IOS close USB: %d\n",ret);*/

