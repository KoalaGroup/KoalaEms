/******************************************************************************
*                                                                             *
* vcc2117.h                                                                   *
*                                                                             *
* created ???                                                                 *
* last changed 25.04.95                                                       *
*                                                                             *
******************************************************************************/

#ifndef _VCC2117_h_
#define _VCC2117_h_

#define CAMAC_START 0xc00000
#define CAMAC_LEN 0x100000

#define Z2_PB 0xe00204
#define JUELICH_BIT 0x40000000 /* BIG ENDIAN */

#define BURST (1<<19)
#define NOWAIT (1<<16)
#define INTERNAL (1<<18)

#define CAMACaddr(n,a,f) ((camadr_t)(CAMACbase+((n)<<6)+((a)<<2)+((f)<<11)))

#define CAMACreadX CAMACread

#define CCCC() CAMACcntl(CAMACaddr(28,9,26))
#define CCCZ() CAMACcntl(CAMACaddr(28,8,26))
#define CCCI(a) CAMACcntl(CAMACaddr(28,9,((a)?26:24)))

#define CAMACstatus() CAMACreadX((camadr_t)((long int)CAMACaddr(30,0,0)+INTERNAL))
#define CAMAClams() CAMACval(CAMACread(CAMACaddr(30,0,0)))

#endif

/*****************************************************************************/
/*****************************************************************************/
