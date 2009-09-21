/*
        BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
        Requires mini.

Copyright (C) 2008, 2009        Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009              Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2008, 2009        Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009        Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009              John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "bootmii_ppc.h"
#include "string.h"
#include "ipc.h"
#include "mini_ipc.h"
#include "nandfs.h"
#include "fat.h"
#include "malloc.h"
#include "diskio.h"
#include "printf.h"
#include "video_low.h"
#include "input.h"
#include "console.h"

#define MINIMUM_MINI_VERSION 0x00010001

/* AES registers for our example */
#define		AES_REG_BASE		0xd020000
#define		AES_CMD			(AES_REG_BASE + 0x000)
#define		AES_SRC			(AES_REG_BASE + 0x004)
#define		AES_DEST		(AES_REG_BASE + 0x008)
#define		AES_KEY			(AES_REG_BASE + 0x00c)
#define		AES_IV			(AES_REG_BASE + 0x010)

otp_t otp;
seeprom_t seeprom;

static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

static char ascii(char s) {
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

void hexdump(void *d, int len) {
  u8 *data;
  int i, off;
  data = (u8*)d;
  for (off=0; off<len; off += 16) {
    printf("%08x  ",off);
    for(i=0; i<16; i++)
      if((i+off)>=len) printf("   ");
      else printf("%02x ",data[off+i]);

    printf(" ");
    for(i=0; i<16; i++)
      if((i+off)>=len) printf(" ");
      else printf("%c",ascii(data[off+i]));
    printf("\n");
  }
}
	
void testOTP(void)
{
	printf("reading OTP...\n");
	getotp(&otp);
	printf("read OTP!\n");
	printf("OTP:\n");
	hexdump(&otp, sizeof(otp));

	printf("reading SEEPROM...\n");
	getseeprom(&seeprom);
	printf("read SEEPROM!\n");
	printf("SEEPROM:\n");
	hexdump(&seeprom, sizeof(seeprom));
}

/* idea of this simple AES engine test:
 * show how the malloc/free pair fails concerning dma access from i/o devices -
 * we don't use real encryption mode here but just copy the data from source to
 * destination - this is achieved by setting the ENA flag in AES_CMD to zero. */
void simple_aes_copy(void)
{
	/* first, allocate memory for source and destination */
	u8* source = memalign(16, 16); // 16 byte aligned, 16 byte length
	u8* destination = memalign(16, 16); // 16 byte aligned, 16 byte length

	/* fill source with some evil data, and set destination to all the same bytes ('?')
	 * to see quickly see whether the copying has worked or not */
	memset(source, '\0', 16);
	strlcpy(source, "please copy me", 16);
	memset(destination, '?', 16);

	printf("----- source addr: %08X, dest addr: %08X -----\n", source, destination);
	hexdump(source, 16);
	hexdump(destination, 16);

	/* reset aes controller */
	write32(AES_CMD, 0);
	while(read32(AES_CMD) != 0);

	/* flush buffers and tell the controller their addresses */
	sync_after_write(source, 16);
	sync_after_write(destination, 16);
	write32(AES_SRC, virt_to_phys(source));
	write32(AES_DEST, virt_to_phys(destination));

	/* fire up copying! */
	write32(AES_CMD, 0x80000000);
	while(read32(AES_CMD) & 0x80000000);

	/* show result */
	printf("result is there:\n");
	sync_before_read(source, 16); // should not be needed, but just to get sure...
	sync_before_read(destination, 16);
	hexdump(source, 16);
	hexdump(destination, 16);

	/* now THIS should show that something is wrong with memory (MMU stuff etc.?) */
	free(source);
	free(destination);
}

int main(void)
{
	int vmode = -1;
	exception_init();
	dsp_reset();

	// clear interrupt mask
	write32(0x0c003004, 0);

	ipc_initialize();
	ipc_slowping();

	gecko_init();
	input_init();
	init_fb(vmode);

	VIDEO_Init(vmode);
	VIDEO_SetFrameBuffer(get_xfb());
	VISetupEncoder();

	u32 version = ipc_getvers();
	u16 mini_version_major = version >> 16 & 0xFFFF;
	u16 mini_version_minor = version & 0xFFFF;
	printf("Mini version: %d.%0d\n", mini_version_major, mini_version_minor);

	if (version < MINIMUM_MINI_VERSION) {
		printf("Sorry, this version of MINI (armboot.bin)\n"
			"is too old, please update to at least %d.%0d.\n", 
			(MINIMUM_MINI_VERSION >> 16), (MINIMUM_MINI_VERSION & 0xFFFF));
		for (;;) 
			; // better ideas welcome!
	}

	/* perform 3 tests... */
	u8 i;
	for (i=0; i<3; i++) {
		printf("======== aes copy test (%d) ========\n", i+1);
		simple_aes_copy();
		printf("====================================\n\n");
	}

	return 0;
}

