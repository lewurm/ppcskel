#ifndef __IRQ_H__
#define __IRQ_H__

/* hollywood-pic registers */
#define HW_PPCIRQFLAG (0x0d800030)
#define HW_PPCIRQMASK (0x0d800034)

/* broadway processor interface registers */
#define BW_PI_IRQFLAG (0x0c003000)
#define BW_PI_IRQMASK (0x0c003004)

/* stolen from libogc - gc/ogc/machine/processor.h */
#define _CPU_ISR_Enable() \
	{ register u32 _val = 0; \
	  __asm__ __volatile__ ( \
		"mfmsr %0\n" \
		"ori %0,%0,0x8000\n" \
		"mtmsr %0" \
		: "=&r" ((_val)) : "0" ((_val)) \
	  ); \
	}

#define _CPU_ISR_Disable( _isr_cookie ) \
  { register u32 _disable_mask = 0; \
	_isr_cookie = 0; \
    __asm__ __volatile__ ( \
	  "mfmsr %0\n" \
	  "rlwinm %1,%0,0,17,15\n" \
	  "mtmsr %1\n" \
	  "extrwi %0,%0,1,16" \
	  : "=&r" ((_isr_cookie)), "=&r" ((_disable_mask)) \
	  : "0" ((_isr_cookie)), "1" ((_disable_mask)) \
	); \
  }

#define _CPU_ISR_Restore( _isr_cookie )  \
  { register u32 _enable_mask = 0; \
	__asm__ __volatile__ ( \
    "    cmpwi %0,0\n" \
	"    beq 1f\n" \
	"    mfmsr %1\n" \
	"    ori %1,%1,0x8000\n" \
	"    mtmsr %1\n" \
	"1:" \
	: "=r"((_isr_cookie)),"=&r" ((_enable_mask)) \
	: "0"((_isr_cookie)),"1" ((_enable_mask)) \
	); \
  }

#endif
