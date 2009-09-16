#ifndef _AT90USB_H
#define _AT90USB_H
// AT90USB/usb_drv.h
// Macros for access to USB registers of Atmels AT90USB microcontrollers
// This file contains low level register stuff as described in
// Atmels AT90USB datasheet 7593D-AVR-07/06
// S. Salewski 21-MAR-2007
// B. Sauter (modified for usb host stack) 2007


// A few macros for bit fiddling
#define SetBit(adr, bit)                        (adr |=  (1<<bit))
#define ClearBit(adr, bit)                      (adr &= ~(1<<bit))
#define BitIsSet(adr, bit)                      (adr & (1<<bit))
#define BitIsClear(adr, bit)                    (!(adr & (1<<bit)))



// PLL clock for USB interface, section 6.11, page 50
// PLLCSR (PLL Control and Status Register)
// set PLL prescaler according to XTAL crystal frequency
//#define UsbXTALFrequencyIs2MHz()              PLLCSR = (PLLCSR & ~(15<<1))
//#define UsbXTALFrequencyIs4MHz()              PLLCSR = (PLLCSR & ~(15<<1)) | (1<<2)
//#define UsbXTALFrequencyIs6MHz()              PLLCSR = (PLLCSR & ~(15<<1)) | (2<<2)
//#define UsbXTALFrequencyIs8MHz()              PLLCSR = (PLLCSR & ~(15<<1)) | (3<<2)
//#define UsbXTALFrequencyIs12MHz()             PLLCSR = (PLLCSR & ~(15<<1)) | (4<<2)
//#define UsbXTALFrequencyIs16MHz()             PLLCSR = (PLLCSR & ~(15<<1)) | (5<<2)
#if (F_CPU == 2000000)
#define _pre_ 0
#elif (F_CPU == 4000000)
#define _pre_ 1
#elif (F_CPU == 6000000)
#define _pre_ 2
#elif (F_CPU == 8000000)
#define _pre_ 3
#elif (F_CPU == 12000000)
#define _pre_ 4
#elif (F_CPU == 16000000)
#define _pre_ 5
#else
  #error "XTAL-Frequency has to be 2, 4, 6, 8, 12 or 16 MHz for USB devices!"
#endif
#define UsbSetPLL_CPU_Frequency()              PLLCSR = (_pre_<<2)
#define UsbEnablePLL()                          SetBit(PLLCSR, PLLE)
#define UsbDisablePLL()                         ClearBit(PLLCSR, PLLE)
#define UsbIsPLL_Locked()                       BitIsSet(PLLCSR, PLOCK)
#define UsbWaitPLL_Locked()                     while (!(PLLCSR & (1<<PLOCK)));
#define UsbEnableClock()                        ClearBit(USBCON, FRZCLK)

//USB general registers, section 21.12.1, page 263 of datasheet
// UHWCON (UsbHardWareCONfiguration)
#define UsbSetDeviceMode()                SetBit(UHWCON, UIMOD)           // select host or device mode manually
#define UsbSetHostMode()                  ClearBit(UHWCON, UIMOD)
#define UsbEnableUID_ModeSelection()      SetBit(UHWCON, UIDE)            // enable mode selection by UID pin
#define UsbDisableUID_ModeSelection()     ClearBit(UHWCON, UIDE)
#define UsbEnableUVCON_PinControl()       SetBit(UHWCON, UVCONE)          // enable UVCON pin control, figure 21-7
#define UsbDisableUVCON_PinControl()      ClearBit(UHWCON, UVCONE)
#define UsbEnablePadsRegulator()          SetBit(UHWCON, UVREGE)          // USB pads (D+, D-) supply
#define UsbDisablePadsRegulator()         ClearBit(UHWCON, UVREGE)

// USBCON (USB CONfiguration)
#define UsbEnableController()             SetBit(USBCON, USBE)            // USB controller enable
#define UsbDisableController()            ClearBit(USBCON, USBE)          // reset and disable controller
#define UsbIsControllerEnabled()          BitIsSet(USBCON, USBE)
#define UsbSetHostModeReg()               SetBit(USBCON, HOST)            // select multiplexed controller registers
#define UsbSetDeviceModeReg()             ClearBit(USBCON, HOST)          //
#define UsbFreezeClock()                  SetBit(USBCON, FRZCLK)          // reduce power consumption
#define UsbEnableClock()                  ClearBit(USBCON, FRZCLK)
#define UsbIsClockFreezed()               BitIsSet(USBCON, FRZCLK)
#define UsbEnableOTG_Pad()                SetBit(USBCON, OTGPADE)         // ??? is this the UID pad?
#define UsbDisableOTG_Pad()               ClearBit(USBCON, OTGPADE)
#define UsbEnableID_TransitionInt()       SetBit(USBCON, IDTE)            // enable ID transition interrupt generation
#define UsbDisableID_TransitionInt()      ClearBit(USBCON, IDTE)
#define UsbEnableVBUS_TransitionInt()     SetBit(USBCON, VBUSTE)          // enable VBUS transition interrupt
#define UsbDisableVBUS_TransitionInt()    ClearBit(USBCON, VBUSTE)

// USBSTA (USBSTAtus, read only)
#define UsbIsFullSpeedMode()              BitIsSet(USBSTA, SPEED)         // set by hardware if controller is in fullspeed mode,
#define UsbIsLowSpeedMode()               BitIsClear(USBSTA, SPEED)       // use in host mode only, indeterminate in device mode
#define UsbIsUID_PinHigh()		  BitIsSet(USBSTA, ID)            // query UID pad/pin
#define UsbIsVBUS_PinHigh()		  BitIsSet(USBSTA, VBUS)          // query VBUS pad/pin

// USBINT (USBINTerrupt)
#define UsbIsIDTI_FlagSet()		  BitIsSet(USBINT, IDTI)          // set by hardware if ID pin transition detected
#define UsbClearIDTI_Flag()		  ClearBit(USBINT, IDTI)          // shall be cleared by software
#define UsbIsVBUSTI_FlagSet()		  BitIsSet(USBINT, VBUSTI)        // set by hardware if transition on VBUS pad is detected
#define UsbClearVBUSTI_Flag()		  ClearBit(USBINT, VBUSTI)        // shall be cleared by software

#endif /* _AT90USB_H */
