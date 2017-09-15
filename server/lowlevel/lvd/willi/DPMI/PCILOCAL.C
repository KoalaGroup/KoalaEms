// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	pcilocal.c	-- for PCI local bus access functions

#include <stddef.h>		// offsetof()
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
//
extern PCI_CONFIG	config;

static PLX_REGS	*plx_regs;
static u_char		*eeprom;
static void			(*_reg_info)(void);
static int			(*_bus_error)(void);
//
// -------------------------- menu position ---------------------------------
//
#define SP_		20
#define ZL_    5
//
// -------------------------- error code ------------------------------------
//
#define CEN_PROMPT		-2		// display no command menu, only prompt
#define CEN_MENU			-1
#define CE_MENU			0
#define CE_PROMPT			1		// display no command menu, only prompt
//
//--------------------------- verify_region ----------------------------------
//
int verify_region(void)
{
	char		wrk[32];
	char		tk0[32];
	char		tk1[32];
	u_char	err;
	int		ix;
	u_long	iy;
	u_int		sz;
	u_long	mx;
	union {
		u_long	lw;
		u_short	sw[2];
	} ofs;
	u_int		ofs_hw;

	u_int		inc;
	u_long	tmp;
	u_long	new;
	u_char	*pci;

	(*_reg_info)();
	printf("Region   %8u ", config.mver_reg);
	config.mver_reg=(u_int)Read_Deci(config.mver_reg, C_REGION);
	if (errno) return 0;

	if (config.mver_reg > C_REGION) config.mver_reg=C_REGION;
	printf("Address   $%06lX ", config.mver_addr[config.mver_reg]);
	config.mver_addr[config.mver_reg]=
									Read_Hexa(config.mver_addr[config.mver_reg], -1);
	if (errno) return 0;

	printf("Size 0/1/2     %2u ", config.mver_size[config.mver_reg]);
	config.mver_size[config.mver_reg]=
						(u_int)Read_Deci(config.mver_size[config.mver_reg], 2);
	if (errno) return 0;

	sz=config.mver_size[config.mver_reg];
	if (sz > 2) sz=2;

	mx=config.region.regs[config.mver_reg].len-1;
	switch (sz) {
case 1:
		mx -=1;
		break;

case 2:
		mx -=3;
		break;
	}

	Writeln();
	printf("@123   = new location is 0x123\n");
	printf("+3     = increment location by 3\n");
	printf("-9     = decrement locatiin by 9\n");
	printf("+=n    = or (and not) <n>\n");
	Writeln();
	ofs.lw=config.mver_addr[config.mver_reg];
	ofs_hw=0xFFFF;
	while (1) {
		if (ofs.lw > mx) ofs.lw=mx;

		if (ofs_hw != ofs.sw[1]) {
			ofs_hw=ofs.sw[1];
			if ((pci=pci_ptr(0, config.mver_reg, ofs.lw &~0xFFFFL)) == 0) {
				printf("illegal region\n"); return CE_PROMPT;
			}
		}

		switch (sz) {
case 0:	tmp=*(pci+ofs.sw[0]);
			break;

case 1:	tmp=*(u_short*)(pci+ofs.sw[0]);
			break;

default:	tmp=*(u_long*)(pci+ofs.sw[0]);
			break;
		}

		_bus_error();
		printf("0x%06lX: %0*lX ", ofs.lw, 2 <<sz, tmp); use_default=0;
		strcpy(wrk, Read_String("", sizeof(wrk)));
		if (errno) return 0;

		do {
			str2up(wrk); ix=0;
			strcpy(tk0, token(wrk, &ix));
			strcpy(tk1, token(wrk, &ix)); err=1; inc=(1<<sz);
			if (ix < strlen(wrk)) break;

			if (tk0[0] && (tk0[0] != '@') &&
				 (((tk0[0] != '+') && (tk0[0] != '-')) || (tk0[1] == '='))) {
				ix=0;
				if (tk0[1] == '=') {
					ix=2;
					if ((tk0[0] != '+') && (tk0[0] != '-')) break;
				}

				new=value(tk0, &ix, 16);
				if (ix < strlen(tk0)) break;

				if (tk0[0] == '+') { new |=tmp; inc=0; }

				if (tk0[0] == '-') { new =tmp &~new; inc=0; }

				switch (sz) {
		case 0:	*(pci+ofs.sw[0])=(u_char)new;
					break;

		case 1:	*(u_short*)(pci+ofs.sw[0])=(u_short)new;
					break;

		default:	*(u_long*)(pci+ofs.sw[0])=new;
					break;
				}

				_bus_error();
				if ((tk1[0] == '-') && !tk1[1]) { err=0; inc=0; break; }

				strcpy(tk0, tk1);
			}

			if (!tk0[0]) { err=0; break; }

			ix=1; iy=value(tk0, &ix, 16);
			if (ix < strlen(tk0)) break;

			err=0; inc=0;
			switch (tk0[0]) {

		case '@':
				ofs.lw=iy; break;

		case '+':
				ofs.lw +=iy; break;

		case '-':
				if (!iy) iy=(1<<sz);

				if (ofs.lw <= iy) ofs.lw=0;
				else ofs.lw -=iy;

				break;

		default:
				err=1; break;
			}

		} while (0);

		if (err) printf("Fehlerhafte Eingabe!\n");
		else ofs.lw +=inc;

	}
}
//
//--------------------------- _cyclic_read -----------------------------------
//
int _cyclic_read(void)
{
	u_long	lp;
	u_long	ret;
	u_long	*pci;
	char		dsp=-1;
	u_long	dsp_val=0;
	u_int		cycle;

	(*_reg_info)();
	printf("Region   %8u ", config.crd_reg);
	config.crd_reg=(u_int)Read_Deci(config.crd_reg, C_REGION);
	if (errno) return 0;

	if (config.crd_reg > C_REGION) config.crd_reg=C_REGION;

	printf("Address   $%06lX ", config.crd_addr[config.crd_reg]);
	config.crd_addr[config.crd_reg]=
									Read_Hexa(config.crd_addr[config.crd_reg], -1);
	if (errno) return 0;

	printf("Size 0/1/2   %4u ", config.crd_size[config.crd_reg]);
	config.crd_size[config.crd_reg]=
						(u_int)Read_Deci(config.crd_size[config.crd_reg], 2);
	if (errno) return 0;

	printf("# of cycles $%04X ", config.crd_cycles[config.crd_reg]);
	config.crd_cycles[config.crd_reg]=
						(u_int)Read_Hexa(config.crd_cycles[config.crd_reg], -1);
	if (errno) return 0;

	lp=0;
	pci=pci_ptr(0, config.crd_reg, config.crd_addr[config.crd_reg]);
	if (!pci) {
		printf("illegal address\n"); return CE_PROMPT;
	}

	cycle=config.crd_cycles[config.crd_reg];
	switch (config.crd_size[config.crd_reg]) {
case 0:
	while (1) {
		ret=*(u_char*)pci;
		if (cycle && !--cycle) {
			printf("\r%10lu %08lX\n", lp, ret);
			return CE_PROMPT;
		}

		if (!dsp && (ret != dsp_val)) { dsp_val=ret; dsp=1; }

		if (!(++lp &0x7FFFFL)) {
			printf("\r%10lu %08lX", lp, ret);
			if (dsp) {
				if (dsp >= 0) printf(" %08lX", dsp_val);

				dsp=0; dsp_val=ret;
			}

			if (_bus_error() && WaitKey(0)) return CE_PROMPT;
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}
	}

case 1:
	while (1) {
		ret=*(u_short*)pci;
		if (cycle && !--cycle) {
			printf("\r%10lu %08lX\n", lp, ret);
			return CE_PROMPT;
		}

		if (!dsp && (ret != dsp_val)) { dsp_val=ret; dsp=1; }

		if (!(++lp &0x7FFFFL)) {
			printf("\r%10lu %08lX", lp, ret);
			if (dsp) {
				if (dsp >= 0) printf(" %08lX", dsp_val);

				dsp=0; dsp_val=ret;
			}

			if (_bus_error() && WaitKey(0)) return CE_PROMPT;
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}
	}

case 2:
	while (1) {
		ret=*pci;
		if (cycle && !--cycle) {
			printf("\r%10lu %08lX\n", lp, ret);
			return CE_PROMPT;
		}

		if (!dsp && (ret != dsp_val)) { dsp_val=ret; dsp=1; }

		if (!(++lp &0x7FFFFL)) {
			printf("\r%10lu %08lX", lp, ret);
			if (dsp) {
				if (dsp >= 0) printf(" %08lX", dsp_val);

				dsp=0; dsp_val=ret;
			}

			if (_bus_error() && WaitKey(0)) return CE_PROMPT;
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}
	}

default:
		return 2;
	}

}
//
//--------------------------- _cyclic_write ----------------------------------
//
static int _cyclic_write(void)
{
	u_long	lp;
	u_long	*pci;
	u_long	val;
	u_long	incr;
	u_int		cycle;

	(*_reg_info)();
	printf("Region       %8u ", config.cwr_reg);
	config.cwr_reg=(u_int)Read_Deci(config.cwr_reg, C_REGION);
	if (errno) return 0;

	if (config.cwr_reg > C_REGION) config.cwr_reg=C_REGION;

	printf("Address       $%06lX ", config.cwr_addr[config.cwr_reg]);
	config.cwr_addr[config.cwr_reg]=
										Read_Hexa(config.cwr_addr[config.cwr_reg], -1);
	if (errno) return 0;

	printf("Size 0/1/2       %4u ", config.cwr_size[config.cwr_reg]);
	config.cwr_size[config.cwr_reg]=
						(u_int)Read_Deci(config.cwr_size[config.cwr_reg], 2);
	if (errno) return 0;

	printf("Start Value $%08lX ", config.cwr_strt[config.cwr_reg]);
	config.cwr_strt[config.cwr_reg]=
										Read_Hexa(config.cwr_strt[config.cwr_reg], -1);
	if (errno) return 0;

	printf("Value Inc   $%08lX ", config.cwr_inc[config.cwr_reg]);
	config.cwr_inc[config.cwr_reg]=
										Read_Hexa(config.cwr_inc[config.cwr_reg], -1);
	if (errno) return 0;

	printf("# of cycles     $%04X ", config.cwr_cycles[config.cwr_reg]);
	config.cwr_cycles[config.cwr_reg]=
							(u_int)Read_Hexa(config.cwr_cycles[config.cwr_reg], -1);
	if (errno) return 0;

	pci=pci_ptr(0, config.cwr_reg, config.cwr_addr[config.cwr_reg]);
	if (!pci) {
		printf("illegal address\n"); return CE_PROMPT;
	}

	val =config.cwr_strt[config.cwr_reg];
	incr=config.cwr_inc[config.cwr_reg];
	lp=0;
	cycle=config.cwr_cycles[config.cwr_reg];
	switch (config.cwr_size[config.cwr_reg]) {
case 0:
		while (1) {
			*(u_char*)pci =(u_char)val;
			if (cycle && !--cycle) {
				printf("\r%10lu\n", lp);
				return CE_PROMPT;
			}

			if (!(++lp &0x7FFFFL)) {
				printf("\r%10lu", lp);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}

			val +=incr;
		}

case 1:
		while (1) {
			*(u_short*)pci =(u_short)val;
			if (cycle && !--cycle) {
				printf("\r%10lu\n", lp);
				return CE_PROMPT;
			}

			if (!(++lp &0x7FFFFL)) {
				printf("\r%10lu", lp);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}

			val +=incr;
		}

case 2:
		while (1) {
			*pci =val;
			if (cycle && !--cycle) {
				printf("\r%10lu\n", lp);
				return CE_PROMPT;
			}

			if (!(++lp &0x7FFFFL)) {
				printf("\r%10lu", lp);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}

			val +=incr;
		}
	}

	return 0;
}
//
//--------------------------- _cyclic_write_verify ---------------------------
//
static int _cyclic_write_verify(void)
{
	u_long	lp;
	u_long	ecnt;
	u_long	*pci;
	u_long	val;
	u_long	incr;
	u_long	ret;
	u_long	mask;

	(*_reg_info)();
	printf("Region       %8u ", config.cwrvfy_reg);
	config.cwrvfy_reg=(u_int)Read_Deci(config.cwrvfy_reg, C_REGION);
	if (errno) return 0;

	if (config.cwr_reg > C_REGION) config.cwr_reg=C_REGION;

	printf("Address       $%06lX ", config.cwrvfy_addr[config.cwrvfy_reg]);
	config.cwrvfy_addr[config.cwrvfy_reg]=
								Read_Hexa(config.cwrvfy_addr[config.cwrvfy_reg], -1);
	if (errno) return 0;

	printf("Size 0/1/2       %4u ", config.cwrvfy_size[config.cwrvfy_reg]);
	config.cwrvfy_size[config.cwrvfy_reg]=
						(u_int)Read_Deci(config.cwrvfy_size[config.cwrvfy_reg], 2);
	if (errno) return 0;

	printf("Start Value $%08lX ", config.cwrvfy_strt[config.cwrvfy_reg]);
	config.cwrvfy_strt[config.cwrvfy_reg]=
								Read_Hexa(config.cwrvfy_strt[config.cwrvfy_reg], -1);
	if (errno) return 0;

	printf("Value Inc   $%08lX ", config.cwrvfy_inc[config.cwrvfy_reg]);
	config.cwrvfy_inc[config.cwrvfy_reg]=
								Read_Hexa(config.cwrvfy_inc[config.cwrvfy_reg], -1);
	if (errno) return 0;

	printf("Mask Verify $%08lX ", config.cwrvfy_msk[config.cwrvfy_reg]);
	config.cwrvfy_msk[config.cwrvfy_reg]=
								Read_Hexa(config.cwrvfy_msk[config.cwrvfy_reg], -1);
	if (errno) return 0;

	pci =pci_ptr(0, config.cwrvfy_reg, config.cwrvfy_addr[config.cwrvfy_reg]);
	if (!pci) {
		printf("illegal address\n"); return CE_PROMPT;
	}

	val =config.cwrvfy_strt[config.cwrvfy_reg];
	incr=config.cwrvfy_inc[config.cwrvfy_reg];
	mask=config.cwrvfy_msk[config.cwrvfy_reg];
	lp=0; ecnt=0;
	if (!config.errlog) printf("no error logging\7\n");

	switch (config.cwrvfy_size[config.cwrvfy_reg]) {
case 0:
		mask =(u_char)mask;
		while (1) {
			*(u_char*)pci =(u_char)val;
			ret  =*(u_char*)pci;
			if ((val ^ret) &mask) {
				if (!config.errlog) ecnt++;
				else {
					printf("\r%10lu, Data Error, is:%08lX/%08lX, exp:$%08lX/%08lX\n",
							 lp, ret, ret &mask, val, val &mask);
					return CE_PROMPT;
				}
			}

			val +=incr;
			if (!(++lp &0x3FFFFL)) {
				printf("\r%10lu%10lu", lp, ecnt);
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

case 1:
		mask =(u_short)mask;
		while (1) {
			*(u_short*)pci =(u_short)val;
			ret  =*(u_short*)pci;
			if ((val ^ret) &mask) {
				if (!config.errlog) ecnt++;
				else {
					printf("\r%10lu, Data Error, is:%08lX/%08lX, exp:$%08lX/%08lX\n",
							 lp, ret, ret &mask, val, val &mask);
					return CE_PROMPT;
				}
			}

			val +=incr;
			if (!(++lp &0x3FFFFL)) {
				printf("\r%10lu%10lu", lp, ecnt);
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

case 2:
		while (1) {
			*pci =val;
			ret  =*pci;
			if ((val ^ret) &mask) {
				if (!config.errlog) ecnt++;
				else {
					printf("\r%10lu, Data Error, is:%08lX/%08lX, exp:$%08lX/%08lX\n",
							 lp, ret, ret &mask, val, val &mask);
					return CE_PROMPT;
				}
			}

			val +=incr;
			if (!(++lp &0x3FFFFL)) {
				printf("\r%10lu %08lX %lu", lp, ret, ecnt);
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

default:
		return 2;
	}

}
//
//--------------------------- _cyclic_write_sequence -------------------------
//
static int _cyclic_write_sequence(void)
{
	int		ix;
	u_long	lp,lpmsk;
	u_long	*pci[C_WSITEMS];
	u_long	ret;

	(*_reg_info)();
	printf("How many Items      %4u ", config.cwrseq_items);
	config.cwrseq_items=(u_int)Read_Deci(config.cwrseq_items, -1);
	if (errno) return 0;

	if (config.cwrseq_items > C_WSITEMS) config.cwrseq_items=C_WSITEMS;
	if (!config.cwrseq_items) config.cwrseq_items=1;

	printf("Size 0/1/2          %4u ", config.cwrseq_size);
	config.cwrseq_size=(u_int)Read_Deci(config.cwrseq_size, 2);
	if (errno) return 0;

	ix=0;
	do {
		printf("Item %d Region       %4d ", ix, config.cwrseq_objs[ix].reg);
		config.cwrseq_objs[ix].reg=
									(u_int)Read_Deci(config.cwrseq_objs[ix].reg, -1);
		if (errno) return 0;

		printf("Item %d Address   $%06lX ", ix, config.cwrseq_objs[ix].addr);
		config.cwrseq_objs[ix].addr=
											Read_Hexa(config.cwrseq_objs[ix].addr, -1);
		if (errno) return 0;

		printf("Item %d read/write 0/1 %2d ", ix, config.cwrseq_objs[ix].wr);
		config.cwrseq_objs[ix].wr=
									(u_int)Read_Hexa(config.cwrseq_objs[ix].wr, 1);
		if (errno) return 0;

		if (config.cwrseq_objs[ix].wr) {
			printf("Item %d Value   $%08lX ", ix, config.cwrseq_objs[ix].value);
			config.cwrseq_objs[ix].value=Read_Hexa(config.cwrseq_objs[ix].value, -1);
			if (errno) return 0;
		}

		if ((pci[ix] =pci_ptr(0, config.cwrseq_objs[ix].reg,
												 config.cwrseq_objs[ix].addr)) == 0)
			printf("illegal address\n");
		else ix++;

	} while (ix < config.cwrseq_items);

	ix=config.cwrseq_items; lpmsk=0xFFFFFL;
	while (ix) { ix >>=1; lpmsk >>=1; }

	lp=0;
	switch (config.cwrseq_size) {
case 0:
		while (1) {
			ix=0;
			do {
				if (config.cwrseq_objs[ix].wr)
					*(u_char*)pci[ix] =(u_char)config.cwrseq_objs[ix].value;
				else
					ret=*(u_char*)pci[ix];

				ix++;
			} while (ix < config.cwrseq_items);

			if (!(++lp &lpmsk)) {
				printf("\r%10lu", lp, ret);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

case 1:
		while (1) {
			ix=0;
			do {
				if (config.cwrseq_objs[ix].wr)
					*(u_short*)pci[ix] =(u_short)config.cwrseq_objs[ix].value;
				else
					ret=*(u_short*)pci[ix];

				ix++;
			} while (ix < config.cwrseq_items);

			if (!(++lp &lpmsk)) {
				printf("\r%10lu", lp, ret);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

case 2:
		while (1) {
			ix=0;
			do {
				if (config.cwrseq_objs[ix].wr)
					*pci[ix] =config.cwrseq_objs[ix].value;
				else
					ret=*pci[ix];

				ix++;
			} while (ix < config.cwrseq_items);

			if (!(++lp &lpmsk)) {
				printf("\r%10lu", lp);
				if (_bus_error()) return CE_PROMPT;
				if (kbhit()) {
					Writeln();
					return CE_PROMPT;
				}
			}
		}

default:
		return 2;
	}
}
//
//--------------------------- region_access_menu -----------------------------
//
int region_access_menu(
	void	(*reg_info)(void),
	int	(*bus_error)(void))
{

	u_int		ux;
	char		key;
	int		ret;

	_reg_info=reg_info;
	_bus_error=bus_error;
	while (1) {
		clrscr();
		ux=0;
		gotoxy(SP_, ux+++ZL_);
						printf("Verify Memory Regions ...........0");
		gotoxy(SP_, ux+++ZL_);
						printf("cyclical Read ...................1");
		gotoxy(SP_, ux+++ZL_);
						printf("cyclical Write ..................2");
		gotoxy(SP_, ux+++ZL_);
						printf("cyclical Write/Verify ...........3");
		gotoxy(SP_, ux+++ZL_);
						printf("cyclical Sequence ...............4");
		gotoxy(SP_, ux+++ZL_);
						printf("Ende .......................^C/ESC");

		gotoxy(SP_, ux+ZL_);
						printf("                                 %c ",
								 config.reg_menu);

		key=toupper(getch());
		if ((key == ESC) || (key == CTL_C)) return 0;

		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) key=config.reg_menu;

			clrscr(); gotoxy(1, 2);
			errno=0; ret=-2;
			switch (key) {

		case '0':
				ret=verify_region();
				break;

		case '1':
				ret=_cyclic_read();
				break;

		case '2':
				ret=_cyclic_write();
				break;

		case '3':
				ret=_cyclic_write_verify();
				break;

		case '4':
				ret=_cyclic_write_sequence();
				break;

			}

			_bus_error();
			while (kbhit()) getch();
			if (ret != -2) config.reg_menu=key;

			if (ret <= 0) break;

			printf("select> ");
			key=getch();
			if (key == CR) break;

			if (key == ' ') key=CR;
		}
	}
}
//
//============================================================================
//
//										plx_eeprom
//
//============================================================================
//
#define EEP_L16	0x80			// length of 16 bit words (2048 Bit)
#define EEP_L8		(2*EEP_L16)	// length of bytes
#define EEP_TM		5				// PLX64: timing constraint

#define EEP_C		0x01			// Clock
#define EEP_S		0x02			// Select
#define EEP_D		0x04			// Data Out
#define EEP_Q		0x08			// Data In
#define EEP_P		0x10			// EEPROM Present
#define EEP_QE		0x80			// PLX64: Data In enable
#define EEP_ALEN	8				// Address Field

static u_short	buffer[0x0100];
static FILE		*edat;
static u_char	line[80];
//
//--------------------------- read_token -------------------------------------
//
static int read_token(void)
{
	u_int		ix;
	u_int		sw;
	int		key;

	ix=0; sw=0; line[0]=0;
	while (1) {
		key=fgetc(edat);
		if (key == EOF) {
			line[ix]=0;
			if (ix) return 0;

			return -1;
		}

		key=toupper(key);
		if (key == ';') sw=2;

		switch (sw) {
case 2:
			if (key != '\n') break;

			if (ix) { line[ix]=0; return 0; }

			sw=0;
case 0:

			if ((key == ' ') || (key == TAB) || (key == '\n')) break;

			sw=1;
case 1:
			if ((key == ' ') || (key == TAB) || (key == '\n')) {
				line[ix]=0; return 0;
			}

			if (ix >= 80-1) { line[80-1]=0; return 1; }

			line[ix++]=key;
			break;
		}
	}
}
//
#ifdef PLX64
//--------------------------- clock_eeprom -----------------------------------
//
static void clock_eeprom(
	u_int d)		// 0 or EEP_D
{
	u_int		i;

	if (d) d=EEP_D;

	for (i=0; i < EEP_TM; i++) *eeprom=(*eeprom &~(EEP_D|EEP_C)) |d;
	for (i=0; i < EEP_TM; i++) *eeprom=(*eeprom &~EEP_D) |EEP_C |d;
	*eeprom=*eeprom &~(EEP_D|EEP_C);
}
//
//--------------------------- deselect_eeprom --------------------------------
//
static void deselect_eeprom(void)
{
	*eeprom=0;
	clock_eeprom(0);
}
//
//--------------------------- setup_eeprom -----------------------------------
//
static void setup_eeprom(
	u_int		len,	// # of bits
	u_long	val)	// bit pattern
{
	u_long	mx;

	mx=1L <<(len-1);
	deselect_eeprom();
	*eeprom =EEP_S;		// select
	do {
		clock_eeprom((val &mx) != 0);
		mx >>=1;
	} while (mx);
}
//
//--------------------------- read_eeprom ------------------------------------
//
static int read_eeprom(u_int adr, u_int *val)
{
	u_int		mx;
	u_int		ix;

	setup_eeprom(3+EEP_ALEN, (0x6 <<EEP_ALEN) |(adr &((1<<EEP_ALEN)-1)));

	*eeprom=EEP_QE |EEP_S;		// select, float the EEDO pin for read
	if (*eeprom &EEP_Q) {
		deselect_eeprom();
		printf("select error\n");
		return CE_PROMPT;
	}

	mx=0; ix=0;
	do {
		clock_eeprom(0);
		mx <<=1;
		if ((*eeprom &EEP_Q) != 0) mx |=1;

	} while (++ix < 16);

	deselect_eeprom();
	*val=mx;

	return 0;
}
//
//--------------------------- write_eeprom -----------------------------------
//
static int write_eeprom(u_int adr, u_int val)
{
	int	ret=0;
	u_int	i;

	deselect_eeprom();
	setup_eeprom(3+EEP_ALEN, (0x4 <<EEP_ALEN) |0xC0);	// write enable
	deselect_eeprom();
	setup_eeprom(3+EEP_ALEN+16,
					 ((u_long)((0x5 <<EEP_ALEN) |(adr &0xFF)) <<16) |val);
	deselect_eeprom();
	for (i=0; i < EEP_TM; i++) *eeprom |=(EEP_QE|EEP_S);	// float the EEDO pin for read
	if (*eeprom &EEP_Q) {
		deselect_eeprom();
		printf("write error\n");
		ret=CE_PROMPT;
	} else {
		while ((*eeprom &EEP_Q) == 0)
			if (kbhit()) {
				deselect_eeprom();
				printf("write error\n");
				ret=CE_PROMPT;
				break;
			}
	}

	deselect_eeprom();
	setup_eeprom(3+EEP_ALEN, (0x4 <<EEP_ALEN) |0x00);	// write disable
	deselect_eeprom();
	return ret;
}
//
#else
//--------------------------- clock_eeprom -----------------------------------
//
static void clock_eeprom(void)
{
	u_int		i;

	for (i=0; i < 2; i++) *eeprom |=EEP_C;
	for (i=0; i < 2; i++) *eeprom &=~EEP_C;
}
//
//--------------------------- deselect_eeprom --------------------------------
//
static void deselect_eeprom(void)
{
	u_int		i;

	for (i=0; i < 5; i++) *eeprom &=0xF0;
	clock_eeprom();
}
//
//--------------------------- setup_eeprom ----------------------------------
//
static void setup_eeprom(
	u_int		len,	// # of bits
	u_long	val)	// bit pattern
{
	u_char	epv;	// base value
	u_long	mx;

	mx=1L <<(len-1);
	deselect_eeprom();
	epv =(*eeprom &0xF0) |EEP_S;		// select
	*eeprom =epv;
	clock_eeprom();

	do {
		if (!(val &mx)) {
			*eeprom =epv;						// reset D(ata), reset C(lock)
		} else {
			*eeprom =epv |EEP_D;				// set D(ata), reset C(lock)
		}

		clock_eeprom();

		mx >>=1;
	} while (mx);

}
//
//--------------------------- setup_eeprom ----------------------------------
//
//--------------------------- read_eeprom -----------------------------------
//
static int read_eeprom(u_int adr, u_int *val)
{
	u_int		mx;
	u_int		ix;

	setup_eeprom(3+EEP_ALEN, (0x6 <<EEP_ALEN) |(adr &0xFF));

	*eeprom |=(EEP_D|EEP_S);		// select, float the EEDO pin for read
	if (*eeprom &EEP_Q) {
		deselect_eeprom();
		printf("select error\n");
		return CE_PROMPT;
	}

	mx=0; ix=0;
	do {
		clock_eeprom();
		mx <<=1;
		if ((*eeprom &EEP_Q) != 0) mx |=1;

	} while (++ix < 16);

	deselect_eeprom();
	*val=mx;

	return 0;
}
//
//--------------------------- read_eeprom ------------------------------------
//
//--------------------------- write_eeprom -----------------------------------
//
static int write_eeprom(u_int adr, u_int val)
{
	u_int		i;

	setup_eeprom(3+EEP_ALEN, (0x4 <<EEP_ALEN) |0xC0);	// write enable
	deselect_eeprom();
	setup_eeprom(3+EEP_ALEN+16,
					 ((u_long)((0x5 <<EEP_ALEN) |(adr &0xFF)) <<16) |val);
	deselect_eeprom();

	for (i=0; i < 4; i++) *eeprom |=(EEP_D|EEP_S);	// float the EEDO pin for read
	if (*eeprom &EEP_Q) {
		deselect_eeprom();
		printf("write error\n");
		return CE_PROMPT;
	}

	while ((*eeprom &EEP_Q) == 0)
		if (kbhit()) {
			deselect_eeprom();
			printf("write error\n");
			return CE_PROMPT;
		}

	deselect_eeprom();
	setup_eeprom(3+EEP_ALEN, (0x4 <<EEP_ALEN) |0x00);	// write disable
	deselect_eeprom();
	return 0;
}
//
#endif
//--------------------------- dsp_eeprom -------------------------------------
//
static int dsp_eeprom(void)
{
	u_int		ix,iy;
	u_int		data;

	iy=0; printf("      ");
	do {
		printf("   -%X", iy); iy +=2;
	} while (iy < 0x10);

	printf("\n");
	ix=0;
	do {
		printf(" %03X: ", ix <<1);
		iy=0;
		do {
			if (read_eeprom(ix+iy, &data)) return CE_PROMPT;

			printf(" %04X", data);
		} while (++iy < 8);

		printf("\n");
		ix+=0x8;
	} while (ix < EEP_L16);

	printf("\n");
	return CE_PROMPT;
}
//
//--------------------------- dsp_eeprom -------------------------------------
//
//--------------------------- verify_eeprom ----------------------------------
//
static int verify_eeprom(void)
{
	char	wrk[32];
	char	tk0[32];
	char	tk1[32];
	int	ix;
	u_int	loc;
	u_int	new;
	u_int	data;
	int	ret;

	dsp_eeprom();
	printf("@123   = new location is 0x123\n");
	printf("+3     = increment location by 3\n");
	printf("-9     = decrement locatiin by 9\n");
	Writeln();
	loc=0;
	while (1) {
		if ((ret=read_eeprom(loc, &data)) != 0) return ret;

		printf("0x%03X: %04X ", loc <<1, data); use_default=0;
		strcpy(wrk, Read_String("", sizeof(wrk)));
		if (errno) return 0;

		str2up(wrk); ix=0;
		strcpy(tk0, token(wrk, &ix));
		strcpy(tk1, token(wrk, &ix));
		if (tk0[0] && (tk0[0] != '@') && (tk0[0] != '+') && (tk0[0] != '-')) {
			ix=0;
			new=(u_int)value(tk0, &ix, 16);
			if (ix < strlen(tk0)) printf("Fehlerhafte Eingabe!\n");
			else {
				if (new != data) {
					if ((ret=write_eeprom(loc, new)) != 0) return ret;
				}
			}

			strcpy(tk0, tk1);
		}

		if (!tk0[0] || ((tk0[0] != '@') && (tk0[0] != '+') && (tk0[0] != '-')))
			loc++;
		else {
			ix=1; new=(u_int)value(tk0, &ix, 16);
			switch (tk0[0]) {

		case '@':
				loc=new >>1; break;

		case '+':
				loc +=(new >>1); break;

		case '-':
				if (!new) new=2;

				loc -=(new >>1);

			}
		}

		if (loc >= EEP_L16) loc=0;
	}
}
//
//--------------------------- verify_eeprom ----------------------------------
//
//--------------------------- save_eeprom ------------------------------------
//
static int save_eeprom(void)
{
	int	ret;
	u_int	loc;
	u_int	data;
	int	hx;

	printf("\nsave EEPROM image to a file\n\n");
	printf("file name ");
	strcpy(config.save_file,
			 Read_String(config.save_file, sizeof(config.save_file)));
	if (errno) return 0;

	if ((hx=_rtl_creat(config.save_file, 0)) == -1) {
		perror("error creating file");
		return CE_PROMPT;
	}

	loc=0;
	do {
		if ((ret=read_eeprom(loc, &data)) != 0) {
			close(hx);
			return ret;
		}

		buffer[loc++]=data;
	} while (loc < EEP_L16);

	if (write(hx, buffer, EEP_L8) != EEP_L8) {
		perror("error writing file");
		close(hx);
		return CE_PROMPT;
	}

	if (close(hx) == -1) {
		perror("error closing file");
		close(hx);
		return CE_PROMPT;
	}

	return 0;
}
//
//--------------------------- save_eeprom ------------------------------------
//
//--------------------------- load_eeprom ------------------------------------
//
static int load_eeprom(void)
{
	int		ret;
	u_int		loc;
	u_int		lx;
	u_int		data;
	u_int		ix;
	u_int		dg;
	u_long	val;
	u_char	key;
	u_int		ux;

	printf("\nload EEPROM image from a file\n\n");
	printf("file name ");
	strcpy(config.load_file,
			 Read_String(config.load_file, sizeof(config.load_file)));
	if (errno) return 0;

	if ((edat=fopen(config.load_file, "r")) == 0) {
		perror("error opening file");
		return CE_PROMPT;
	}

	for (loc=0; loc < EEP_L16; buffer[loc++]=0xFFFF);

	lx=0; ix=0; dg=0; val=0;
	while ((ret=read_token()) == 0) {
		ix=0; dg=0; val=0;
		if ((line[0] == '0') && (line[1] == 'X')) ix=2;

		while ((key=line[ix++]) != 0) {
			ux=key;
			if (ux > '9') {
				if (ux < 'A') break;

				ux -=7;
			}

			ux -='0';
			if (ux >= 16) break;

			val=(val <<4) +ux; dg++;
		}

		if (key) {
			printf("envallid key: 0x%02X\n", key); dg=0;
		}

		switch (dg) {
case 2:
			*(u_char*)((u_char*)buffer +lx) =(u_char)val;
			lx++;
			break;

case 4:
			if (lx &0x1) { ret=1; break; }

			*(u_short*)((u_char*)buffer +lx) =(u_short)val;
			lx +=2;
			break;

case 8:
			if (lx &0x3) { ret=1; break; }

			*(u_long*)((u_char*)buffer +lx) =val;
			lx +=4;
			break;

default:
			ret=1;
			break;
		}

		if (lx >= 2*EEP_L16) {
			ret=read_token(); break;
		}

		if (ret) break;
	}

	fclose(edat);
	if (ret != -1) {
		printf("format error\n%s\n", line);
		return CE_PROMPT;
	}

	printf("read/write EEPROM\n");
	loc=0; lx =(lx+1) >>1;

	do {
		if ((ret=read_eeprom(loc, &data)) != 0) return ret;

		if (buffer[loc] != data) {
			if (write_eeprom(loc, buffer[loc])) return CE_PROMPT;
		}

		loc++;
		printf("\r%03X ", loc);
	} while ((loc < lx) && (loc < EEP_L16));

	printf("\r%03X\n", loc);
	printf("verify EEPROM\n");
	loc=0;
	do {
		if ((ret=read_eeprom(loc, &data)) != 0) return ret;

		if (data != buffer[loc]) {
			printf("data error, adr:0x%03X, exp:0x%04X, is:0x%04\n",
					 loc <<1, buffer[loc], data);
			return CE_PROMPT;
		}

		loc++;
	} while ((loc < lx) && (loc < EEP_L16));

	return CE_PROMPT;
}
//
//--------------------------- load_eeprom ------------------------------------
//
//--------------------------- eeprom_menu ------------------------------------
//
int eeprom_menu(void)
{
	u_int		ux;
	char		key;
	int		ret;

	plx_regs=(PLX_REGS*)config.region.r0_regs;
	eeprom  =((u_char*)&plx_regs->cntrl)+3;

	while (1) {
		clrscr();
		if (!(*eeprom &EEP_P)) {
			gotoxy(SP_, ZL_-2);
			printf("no programmed EEPROM present");
		}

		ux=0;
		gotoxy(SP_, ux+++ZL_);
						printf("zeige EEPROM ....................0");
		gotoxy(SP_, ux+++ZL_);
						printf("verify EEPROM ...................1");
		gotoxy(SP_, ux+++ZL_);
						printf("save EEPROM to file .............2");
		gotoxy(SP_, ux+++ZL_);
						printf("load EEPROM from file ...........3");
		gotoxy(SP_, ux+++ZL_);
						printf("Ende .......................^C/ESC");

		gotoxy(SP_, ux+ZL_);
						printf("                                 %c ",
								 config.epr_menu);

		key=toupper(getch());
		if ((key == ESC) || (key == CTL_C)) return 0;

		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) key=config.epr_menu;

			clrscr(); gotoxy(1, 2);
			errno=0; ret=-2;
			switch (key) {

		case '0':
				ret=dsp_eeprom();
				break;

		case '1':
				ret=verify_eeprom();
				break;

		case '2':
				ret=save_eeprom();
				break;

		case '3':
				ret=load_eeprom();
				break;

			}

			while (kbhit()) getch();
			if (ret != -2) config.epr_menu=key;

			if (ret <= 0) break;

			printf("select> ");
			key=getch();
			if (key == CR) break;

			if (key == ' ') key=CR;
		}
	}
}
//
