/*

reg_acc.c
********************************************************************
                  VME-bus CPU register Access

    copyright:    W-Ie-Ne-R, PleinBaus GmbH, A. Ruben  20.12.94
********************************************************************

*/

void vme_init(void);
void put_reg(unsigned long int ,unsigned long int );
unsigned long int get_reg(unsigned long int);

#define vc16_base   0x800000    /* for VC16 rotary switches */
#define eltec_base  0xff000000  /* for ELTEC CPU, AM$39 */
#define cesfic_base 0xf0000000  /* for CES FIC8232 CPU, 16bit data, AM$39 */

#define am 0x39                 /* Address modifier for ATVME system
				   jumper to AM$39 on VC16 board */

/* ################  ELTEC 68X00 CPU System  ################
 AM $39 by address offset
*/
#ifdef ELTEC

void put_reg(unsigned long int regadr,unsigned long int data)
{
	regadr+=eltec_base;
	* ( short * ) regadr = data;
}

unsigned long int get_reg(unsigned long int regadr)
{
	regadr+=eltec_base;
	return  ( * ( unsigned short * ) regadr);
}

void vme_init(void)
{
}
#endif

/*################  CES FIC8232 CPU System  ################
 AM $39 by address offset
*/
#ifdef CES


void put_reg(unsigned long int regadr,unsigned long int data)
{
	regadr+=cesfic_base;
	* ( short * ) regadr = data;
}

unsigned long int get_reg(unsigned long int regadr)
{
	regadr+=cesfic_base;
	return  ( * ( unsigned short * ) regadr);
}

void vme_init(void)
{
}
#endif

/* ################  PRO_VME PC-VME System ############
*/

#ifdef ATVME
#include "VMELIB.C"

void put_reg(unsigned long int regadr,unsigned long int data)
{
	WriteWordExt(regadr,data,am);
}

unsigned long int get_reg(unsigned long int regadr)
{
	return ReadWordExt(regadr, am);
}

void vme_init(void)
{
	Init_AT_VME();
}
#endif

