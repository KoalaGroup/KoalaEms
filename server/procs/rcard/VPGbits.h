
#ifndef _VPGbits_h_
#define _VPGbits_h_

#define   DONE               0x00010000  // Kramert 00
#define   A3                 0x00020000  // Kramert 01
#define   A2                 0x00040000  // Kramert 02
#define   A1                 0x00080000  // Kramert 03
#define   A0                 0x00100000  // Kramert 04
#define   RW                 0x00200000  // Kramert 05
#define   CLK_TA             0x00400000  // Kramert 06
#define   TEST_PULSE         0x00800000  // Kramert 07, REGin
#define   CLEAR              0x01000000  // Kramert 08, ADC clear
#define   TEST_ON            0x02000000  // Kramert 09, DAC sync
#define   SHIFT_IN           0x04000000  // Kramert 10, DACDAT=DACDin
#define   CLK                0x08000000  // Kramert 11, DACCLK
#define   DRESET             0x10000000  // Kramert 12
#define   CONVERT            0x20000000  // Kramert 13, ADC convert signal
#define   HOLD               0x40000000  // Kramert 14
#define   SPARE1             0x80000000  // Kramert 15, not used

#endif
