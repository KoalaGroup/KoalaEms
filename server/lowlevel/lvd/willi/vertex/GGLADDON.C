// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	ggladdon.c -- general functions for PCI gigalink controller */

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <fcntl.h>
#include "pci.h"
#include "pcilocal.h"
#include "daq.h"

//-------------------------------
//			other constants
//-------------------------------
//
#define SP_		20
#define ZL_    5

//
//===========================================================================
//                            region access menu                            =
//===========================================================================
//
//--------------------------- reg_info --------------------------------------
//
static void reg_info(void)
{
	u_int	ix;

	clrscr(); gotoxy(1, 3);
	for (ix=0; ix < C_REGION; ix++) {
		if (pciconf.region.regs[ix].selio) {
			printf("region %d, %s len=0x%08lX\n", ix,
					 (pciconf.region.regs[ix].selio &7) ? "mem" :"io ",
					 pciconf.region.regs[ix].len);
		}
	}

	if (pciconf.region.xrom_sel)
		printf("region %d, exp len=0x%08lX\n",
				 C_REGION, pciconf.region.xrom_len);

	Writeln();
	printf("Region 0    Region 2                      Region 3\n");
	printf("--------    --------------------------    ---------\n");
	printf("PLX         ident               = $%03X    bus access\n",
			 (u_int)&((GIGAL_REGS*)0)->ident);
	printf("INTCSR  =68 sr                  = $%03X\n",
			 (u_int)&((GIGAL_REGS*)0)->sr);
	printf("DMAMODE0=80 cr                  = $%03X\n",
			 (u_int)&((GIGAL_REGS*)0)->cr);
	printf("DMAMODE1=94 tp_special          = $%03X\n",
			 (u_int)&((GIGAL_REGS*)0)->tp_special);
	printf("DMACSR0 =A8 tp_data             = $%03X\n",
			 (u_int)&((GIGAL_REGS*)0)->tp_data);
	printf("            prot_err            = $%03X\n",
			 (u_int)&((GIGAL_REGS*)0)->prot_err);
	Writeln();
}
//
//--------------------------- bus_error -------------------------------------
//
static int bus_error(void)
{
	return protocol_error(-1, 0);
}
//
//============================================================================
//
//--------------------------- addon_dial -------------------------------------
//
int addon_dial(void)
{
	return region_access_menu(reg_info, bus_error);
}


