// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	daqlvd.c -- LVD BUS / Input LVD BUS

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include "pci.h"
#include "pciconf.h"
#include "pcibios.h"
#include "pcilocal.h"
#include "daq.h"

CONFIG		config;		// will be imported at pci.c, pciconf.c and pcibios.c
PLX_REGS		*plx_regs;
u_char		*eeprom;
LVD_REGS		*lvd_regs;	// local register
void			*lvd_bus;
char			lvd_ok;

u_int		*abuf[C_WBUF];
char		UCLastKey[20] = {'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};

static char		pup;			// power up
static char		root[100];
static char		ProgName[20];

static void interrupt	(*intsave)();	// save original interrupt handler
static u_int		pci_irq;
static u_char		masksave =0xFF;		// saved interrupt mask
static u_int		IntCtrl;					// IO port, 0x21 or 0xA1
u_int		int_vec  =0;			// vector for InterruptHandler
										//  0: no InterruptHandler installed

char		pci_req;					// interrupt arrived
u_int		int_cnt;					// interrupt counter
u_long	isr_intcsr;				// last plx->intcsr value
static u_int		isr_ifsr;				// Interface Status Register
static u_long		isr_dma0;				// Int DMA counter
static u_long		isr_dma1;
//
//
// Achtung: Speicher Eigenschaften
//				XMS   1 MB
//				DPMI  2 MB (minimum)
//
u_long	sysram;

#define L_DMABUF		((u_long)DMA_PAGES <<12)	// length of DMA Buffer bytes
u_long	*dmabuf[DMA_64K]={0};			// DMA buffer
u_int		dmapages=0;
u_long	dmabuf_phy[DMA_PAGES] ={0};	// physical addresses
u_int		phypages=0;							// number of phy pages

u_long	*dmadesc=0;							// DMA descriptor buffer
u_long	dmadesc_phy[2]={0};
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
	printf("Region 0    Region 2\n");
	printf("--------    --------------------------\n");
	printf("PLX         version             = $%03X\n",
			 OFFSET(LVD_REGS, version));
	printf("INTCSR  =68 sr                  = $%03X\n",
			 OFFSET(LVD_REGS, sr));
	printf("DMAMODE0=80 cr                  = $%03X\n",
			 OFFSET(LVD_REGS, cr));
	printf("DMAMODE1=94 temp                = $%03X\n",
			 OFFSET(LVD_REGS, temp));
	printf("DMACSR0 =A8\n");
	Writeln();
}
//
//--------------------------- bus_error -------------------------------------
//
int bus_error(void)
{
	if (!(lvd_regs->sr &SR_LB_TOUT)) return 0;

	lvd_regs->sr =SR_LB_TOUT;
	printf("\n>>> Bus Error <<<\n");
	return CEN_PROMPT;
}
//
//--------------------------- display_errcode --------------------------------
//
void display_errcode(int err)
{
	if (err == CE_PROMPT) return;

	if (err &CE_KEY_PRESSED) {
		printf("key pressed: %03X\n", err &0x1FF);
		return;
	}

	printf("err# %u, ", err);
	switch (err) {

case CE_PARAM_ERR:
		printf("parameter error\n"); break;

case CE_NO_DMA_BUF:
		printf("no DMA buffer allocated\n"); break;

case CE_FATAL:
		printf("fatal error\n"); break;

case CE_NO_SC:
		printf("no system controller\n"); break;

case CE_NO_INPUT:
		printf("no input card found\n"); break;

case CE_F1_NOLOCK:
		printf("F1 DLL will not lock\n"); break;

case CE_NOT_IMPL:
		printf("function net yet implemented\n"); break;

default:
		printf("\n"); break;
	}
}
//
//============================================================================
//                            Interrupt Handler                              =
//============================================================================
//
//--------------------------- pci_isr ----------------------------------------
//
void interrupt pci_isr(void)
{
	u_long	tmp;

	isr_intcsr =plx_regs->intcsr;
	plx_regs->intcsr =isr_intcsr;		// clear Interrupt
	if (isr_intcsr &(1L<<21)) {		// DMA 0 active
		plx_regs->dmacsr[0] |=0x8;
		isr_dma0++;
	}

	if (isr_intcsr &(1L<<22)) {		// DMA 1 active
		plx_regs->dmacsr[1] |=0x8;
		isr_dma1++;
	}

	if (isr_intcsr &(1L<<15)) {		// local interrupt
		tmp =lvd_regs->sr;
		lvd_regs->sr=tmp;
		isr_ifsr |=(u_int)tmp;
	}

	if (IntCtrl == 0xA1) outportb(0xA0, 0x20);	// End Of Interrupt

	outportb(0x20, 0x20);
	pci_req=1; int_cnt++;
}
//
//--------------------------- int_logging ------------------------------------
//
static void int_logging(void)
{
	highvideo();
	cprintf("PCI Interrupt %5u: DMA=%lu/%lu, ifsr=0x%04X",
			  int_cnt, isr_dma0, isr_dma1, isr_ifsr);
	lowvideo(); cprintf("\r\n");
	pci_req=0; int_cnt=0; isr_dma0=0; isr_dma1=0; isr_ifsr=0;
}

//--------------------------- Install_Int ------------------------------------
//
// return 1: Handler installted              => int_vec != 0
//        0: no Handler, interrupt disabled, => int_vec == 0
//       -1: error                           => pci_irq == 0
//
static int Install_Int(
	int	install)		// 0:    uninstall
							// else: install
{
	u_int	int_vec_;
	u_int	IntNum;					// 0..7
	u_int	IntBit;					// intmask 0x01..0x80 for IntController

	pci_req=0; int_cnt=0; isr_dma0=0; isr_dma1=0; isr_ifsr=0;
	if (lvd_ok) plx_regs->intcsr &=~0x0100;		// disable PCI Interrupt

	if (int_vec) {
		if (masksave != 0xFF) {
			outportb(IntCtrl, masksave);	// restore Interrupt mask
		}

		_dos_setvect(int_vec, intsave);
		int_vec=0;
	}

	if (!pci_irq || !install) return 0;

	if (pci_irq < 8) {
		IntCtrl=0x21; IntNum =pci_irq;   int_vec_=0x08+IntNum;
	} else {
		IntCtrl=0xA1; IntNum =pci_irq-8; int_vec_=0x70+IntNum;
	}

	IntBit=1 <<IntNum;
	int_vec=int_vec_;
	intsave=_dos_getvect(int_vec);
	_dos_setvect(int_vec, pci_isr);
	masksave=inportb(IntCtrl);
	if (debug) {
		printf("irq=%u, strtmsk=%02X\n", pci_irq, masksave);
		if (WaitKey(0)) {
			_dos_setvect(int_vec, intsave);
			return 0;
		}
	}

	plx_regs->dma[0].dmamode |=0x020000L;	// Int select PCI
	plx_regs->dma[1].dmamode |=0x020000L;	// Int select PCI
	outportb(IntCtrl, masksave &~IntBit);
	plx_regs->intcsr =0x0C0B00L;			// enable DMA0/1/LINT Interrupt
	plx_regs->dmacsr[0] |=0x8;				// clear DMA Interrupt
	plx_regs->dmacsr[1] |=0x8;
	lvd_regs->sr =CR_INTMASK;			// clear all Interrupt

	return 1;
}
//
//============================================================================
//
//--------------------------- err_logging ------------------------------------
//
int err_logging(void)
{
	printf("Error Logging %4d ", pciconf.errlog);
	pciconf.errlog=(u_int)Read_Deci(pciconf.errlog, -1);
	return CEN_PROMPT;
}
//
//============================================================================
//
void _delay_(u_long dx)
{
static u_long tmp;

	tmp=0;
	while (tmp++ != dx);
}
//
//--------------------------- dma_buffer ------------------------------------
//
int dma_buffer(
	u_int		mode)		// 0: display and clear pages
							// 1: clear pages
{
	u_int		ux;

	if (phypages == 0) return CE_NO_DMA_BUF;

	ux=0;
	do dmadesc[ux++]=0xCCCCCCCCL; while (ux < SZ_PAGE_L);

	ux=0;
	do dmabuf[0][ux++]=0xCCCCCCCCL; while (ux < SZ_PAGE_L*dmapages);

	if (mode != 0) return CEN_MENU;

	printf("pg X: phy %08lX\n", dmadesc_phy[0]);
	ux=0;
	do printf("pg %u: phy %08lX\n", ux, dmabuf_phy[ux]);
	while (++ux < phypages);

	return CEN_PROMPT;
}
//
//--------------------------- alloc_dma_buffer ------------------------------
//
static void _search_phy(void)
{
#define TEST_PAT	((u_long)0x19471220L)
#define TEST_PATM	0x1F

	u_long	lin;
	u_int		ux;
	u_int		pg;		// # pages found
	u_int		tmp;
	u_int		tmp0,tmp1;
	u_long	pat;

	lin=0x100000L;
	plx_regs->dma[1].dmamode =0x0249C3L;	// IS|EOT|AC.BU|BT.RY|32
	plx_regs->dma[1].dmaladr =ADDRSPACE0+(u_int)&((LVD_REGS*)0)->temp;
	plx_regs->dma[1].dmasize =1*4;
	plx_regs->dma[1].dmadpr  =0x0;			// read sysmem
	plx_regs->dmacsr[1] |=0x1;
	ux=0;
	do dmabuf[0][ux<<10]=TEST_PAT+ux;
	while (++ux < dmapages);

	*dmadesc =TEST_PAT+0x10; pg=0;
	tmp0=tmp1=0;
	do {
		plx_regs->dma[1].dmapadr =lin;
		plx_regs->dmacsr[1] =0x3;
		tmp=0;
		while (!(plx_regs->dmacsr[1] &0x10)) {
			_delay_(0);
			if (++tmp >= 100) {
				printf("0: DMA 1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
				return;
			}
		}

		if (tmp > tmp0) tmp0=tmp;
		pat=lvd_regs->temp;
		if ((pat &0xFFFFFFF0L) == TEST_PAT) {
			ux=0;
			do {
				if ((pat &0xF) != ux) continue;

				dmabuf[0][ux<<10]=~TEST_PAT;
				plx_regs->dmacsr[1] =0x3;
				tmp=0;
				while (!(plx_regs->dmacsr[1] &0x10)) {
					_delay_(0);
					if (++tmp >= 100) {
						printf("1: DMA1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
						return;
					}
				}

				if (tmp > tmp1) tmp1=ux;

				if (lvd_regs->temp == ~TEST_PAT) {
					printf("pg %u: phy %08lX\n", ux, lin);

					dmabuf_phy[ux]=lin; pg++;
					break;
				}

				dmabuf[0][ux<<10]=TEST_PAT+ux;

			} while (++ux < dmapages);

		} else {
			if (pat == TEST_PAT+0x10) {
				*dmadesc =~(TEST_PAT+0x10);
				plx_regs->dmacsr[1] =0x3;
				tmp=0;
				while (!(plx_regs->dmacsr[1] &0x10)) {
					_delay_(0);
					if (++tmp >= 100) {
						printf("1: DMA1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
						return;
					}
				}

				if (tmp > tmp1) tmp1=ux;

				if (lvd_regs->temp == ~(TEST_PAT+0x10)) {
					printf("pg X: phy %08lX\n", lin);
					dmadesc_phy[0]=lin;
				}

				*dmadesc =TEST_PAT+0x10;
			}
		}

		if (dmadesc_phy[0] && (pg >= dmapages)) {
			if (debug) printf(" %u %u\n", tmp0, tmp1);

			phypages=dmapages;
			return;
		}

		if (!(lin &0x00FFFFFFL) && kbhit()) {
			printf(" abort at lin %08lX\n", lin); return;
		}

		lin +=SZ_PAGE;

	} while (lin < sysram);

	printf("only %u pages found\n", pg);
}


static int alloc_dma_buffer(
	u_int		mode)		// 0: allocate without dialog
							// 1: dialog for number of pages
{
static u_long	tmp;

	DPMIREGS	regs;
	u_int		sel;
	u_long	lin;
	u_long	len;

	if (sysram == 0) {
		printf("unknown size of System Memory\n"); return CEN_PROMPT;
	}

	if (mode == 1) {
		printf("# of phys pg %2u ", config.phys_pg);
		config.phys_pg=(u_int)Read_Deci(config.phys_pg, 16);
		if (errno) return CEN_MENU;

		if (config.phys_pg == 0) return CEN_PROMPT;

		if (dmabuf[0]) {
			printf("restart the program\n");
			return CEN_PROMPT;
		}
	}

	if (!dmadesc) {
		//
		//--- allocate DMA buffer
		//
		regs.ax=0x0501;					// Allocate Memory Block
		regs.bx=0;
		regs.cx=SZ_PAGE;
		if (DPMI_call(&regs)) {
			printf("2: %04X\n", regs.ax);
			return CEN_PROMPT;
		}

		lin=((u_long)regs.bx <<16) |regs.cx;
		if (debug) printf("lin addr=  %08lX\n", lin);

		if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
			printf("can't allocate selector\n");
			return CEN_PROMPT;
		}

		SetSelectorLimit(sel, SZ_PAGE-1);
		SetSelectorBase(sel, lin);
		dmadesc=MAKELP(sel, 0);
	}

	if (!dmabuf[0]) {
		if (config.phys_pg == 0) return CE_NO_DMA_BUF;

		if (config.phys_pg > DMA_PAGES) config.phys_pg=DMA_PAGES;
		//
		//--- allocate DMA buffer
		//
		dmapages=config.phys_pg;
		while (1) {
			len=(u_long)dmapages *SZ_PAGE;
			regs.ax=0x0501;					// Allocate Memory Block
			regs.bx=(u_int)(len >>16);
			regs.cx=(u_int)len;
			if (!DPMI_call(&regs)) break;

			if (!--dmapages) {
				printf("can't allocate DMA buffer\n");
				return CEN_PROMPT;
			}
		}

		if (dmapages != config.phys_pg)
			printf("only %u DMA pages allocated\n", dmapages);

		lin=((u_long)regs.bx <<16) |regs.cx;
		if (debug) printf("lin addr=  %08lX\n", lin);

		if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
			printf("can't allocate selector\n");
			return CEN_PROMPT;
		}

		SetSelectorLimit(sel, len-1);
		SetSelectorBase(sel, lin);
		dmabuf[0]=MAKELP(sel, 0);
	}

	if (!lvd_ok) return CEN_PROMPT;
	//
	//--- search for physical address
	//
	_search_phy();
	if (phypages == 0) {
		printf("no or not all phys pages found\n");
		return CEN_PROMPT;
	}

	dma_buffer(1);
	return CEN_PROMPT;
}
//
// end ---------------------- alloc_dma_buffer -------------------------------
//
//------------------------------ show_dmabuf ---------------------------------
//
static int show_dmabuf(int pg)
{
static int	blk;

	u_int	ix;
	u_int	ln;

	if (!dmabuf[0]) return CE_NO_DMA_BUF;

	if (pg < 0) {
		printf("number of block %2d ", blk);
		blk=(u_int)Read_Deci(blk, -1);
		if (errno) return CEN_MENU;

		pg=blk;
	}

	if (pg == -1) {
		ix=0; ln=0;
		do {
			if (!(ix &0x7)) printf("%04X: ", 4*ix);

			printf(" %08lX", dmadesc[ix++]);
			if (!(ix &0x7)) {
				printf("\n");
				if (++ln >= 32) {
					if (WaitKey(0)) return CEN_PROMPT;

					ln=0;
				}
			}

		} while (ix < SZ_PAGE_L);

		return CEN_PROMPT;
	}

	if ((u_int)pg >= dmapages) pg=dmapages-1;

	ix=SZ_PAGE_L*pg; ln=0;
	while (ix < SZ_PAGE_L*dmapages) {
		if (!(ix &0x7)) printf("%04X: ", 4*ix);

		printf(" %08lX", dmabuf[0][ix++]);
		if (!(ix &0x7)) {
			printf("\n");
			if (++ln >= 32) {
				if (WaitKey(0)) return CEN_PROMPT;

				ln=0;
			}
		}
	}

	return CEN_PROMPT;
}
//
//------------------------------ FPGA_reset ----------------------------------
//
static int FPGA_reset(void)
{

	lvd_regs->sr =SR_MRESET;
	return CEN_MENU;
}
//
//------------------------------ PLX_reset -----------------------------------
//
static int PLX_reset(void)
{
	int		ret;
	u_int		tmp;

// to restore interrupt line (cleared by PLX reset)

	cnf_acc.bus_no   =pciconf.region.bus_no;
	cnf_acc.device_no=pciconf.region.device_no;
	cnf_acc._register=0x3C;
	if ((ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc)) != 0) {
		printf("%s\n", pci_error(ret));
		return CEN_PROMPT;
	}

	*eeprom =0x40;
	*eeprom =0x00;
	*eeprom =0x20;
	tmp=0;
	while ((!plx_regs->las0ba || (tmp < 10)) && (++tmp < 50));

	*eeprom =0x00;
	if (tmp < 50) {
		if ((ret=pci_bios(WRITE_CONFIG_BYTE, &cnf_acc)) != 0) {
			printf("%s\n", pci_error(ret));
			return CEN_PROMPT;
		}

		return CEN_MENU;
	}

	printf("reset failed\n");
	printf("%u: %02X\n", tmp, *eeprom);
	printf("%08lX %08lX\n", plx_regs->las0ba, plx_regs->las1ba);
	return CEN_PROMPT;

}
//
//--------------------------- plx_init ---------------------------------------
//
static void plx_init(void)
{

	lvd_ok=0;
	if (pci_ok != 2) return;

	pci_irq=pciconf.region.int_line;
	if (pci_irq >= 16) pci_irq=0;

	if (debug) {
		printf("int line = %u\n", pciconf.region.int_line);
		WaitKey(0);
	}

	plx_regs=(PLX_REGS*)pciconf.region.r0_regs;
	eeprom  =((u_char*)&plx_regs->cntrl)+3;
	if ((lvd_regs=(LVD_REGS*)pci_ptr(&pciconf.region, 2, 0)) == 0) return;

	if ((lvd_bus=pci_ptr(&pciconf.region, 3, 0)) == 0) return;

	plx_regs->intcsr =0;
	if ((u_char)(lvd_regs->version >>16) != MASTER) return;

	lvd_ok=1;
}
//
//------------------------------ cmc_op_regs --------------------------------
//
// show/modify AddOn Operation Registers
//
const char *sr_txt[] ={
	"Breq", "?", "?", "?", "Back", "Att", "?", "?",
	"?",    "?", "?", "?", "BuEr", "?",   "?", "?"
};

const char *cr_txt[] ={
	"?", "?", "?", "?", "Back", "Att", "?", "?",
	"?", "?", "?", "?", "BuEr", "?",   "?", "?"
};

static int cmc_op_regs(void)
{
	u_int		ux,ix;
	char		hgh;
	char		key;
	u_long	srg;
	u_long	tmp;
	char		hd;
	u_short	*regs=(u_short*)lvd_bus;

	hd=1;
	while (1) {
		srg=lvd_regs->sr; errno=0;
		if (hd) {
			Writeln();
			if (pci_req) int_logging();

printf("Version                       $%08lX\n",
			lvd_regs->version);
printf("Status Register               $%08lX ..0 ", srg);
			ux=16; hgh=0;
			while (ux--)
				if (srg &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("Control Register              $%08lX ..1 ", tmp=lvd_regs->cr);
			ux=16; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("address for block read (start)     $%03lX ..2\n",
			lvd_regs->lvd_block);
			if (!(srg &SR_BLKTX)) {		// sonst kein access auf piggy back register
printf("LVD SCAN window                  $%05lX ..3", tmp=lvd_regs->lvd_scan_win);
					if (tmp &0x10000L) printf(" DAQ System mode");
					Writeln();
printf("LVD SCAN continuously              $%03lX ..4\n",
				lvd_regs->lvd_scan);
printf("LVD read block length           $%06lX ..5\n",
				lvd_regs->lvd_blk_cnt);
printf("LVD read block length max       $%06lX ..6\n",
			lvd_regs->lvd_blk_max);
printf("LVD read                       at  $%03X ..7\n",
				(u_short)config.ao_regs[7] &0xFFF);
printf("broadcast LVD read             at  $%03X ..8\n",
				(u_short)config.ao_regs[8] &0xFFF);
printf("LVD write               $%04X  to  $%03X ..9\n",
				(u_short)config.ao_regs[15], (u_short)config.ao_regs[9] &0xFFF);
			}

			if (!(plx_regs->dmacsr[0] &0x10))
printf("stop ");
			else
printf("start");

printf(     " demand DMA channel...         $%02X ..d ", plx_regs->dmacsr[0]);
			if (!(plx_regs->dmacsr[0] &0x10)) {
				printf("phy. page 0");
				if (plx_regs->dma[0].dmamode &0x4000) printf(", EOT");
			}
			Writeln();

printf("Reset/ClearStatus/Buf                   ..R/C/B\n");
			printf("                                          %c ",
					 pciconf.op2_menu);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=pciconf.op2_menu;
		}

		use_default=(key == TAB);
		if (use_default || (key == CR)) key=pciconf.op2_menu;
		if (key > ' ') printf("%c\n", key);

		if (key == 'R') {
			lvd_regs->sr =SR_MRESET;
			continue;
		}

		if (key == 'C') {
			lvd_regs->sr =lvd_regs->sr;
			continue;
		}

		if (key == 'B') {
			show_dmabuf(-1);
			continue;
		}

		if (key == 'D') {
			if (!(plx_regs->dmacsr[0] &0x10)) {
				plx_regs->dmacsr[0]=0x00;
				lvd_regs->sr =SR_DMA0_ABORT; ix=0;
				while (plx_regs->dmacsr[0] != 0x10) {
					if (++ix < 4) continue;

					printf("DMA is hanging up\n");
					hd=-1;
				}

				continue;
			}

			if (phypages == 0) {
				display_errcode(CE_NO_DMA_BUF);
				continue;
			}

			printf("destination is phy. page 0\n");
			dma_buffer(1);
			plx_regs->dma[0].dmamode =0x02D9C3L;	// IS|FST.EOT.DMD|AC.BU|BT.RY|32
			plx_regs->dma[0].dmapadr =dmabuf_phy[0];
			plx_regs->dma[0].dmaladr =0;
			plx_regs->dma[0].dmasize =SZ_PAGE;
			plx_regs->dma[0].dmadpr  =0x08;				// LOCAL2PCI

			plx_regs->dmacsr[0] =0x3;
			continue;
		}

		if (key == '5') {
			for (ix=0; ix < 16; ix++) {
				printf(" %04lX", lvd_regs->lvd_scan_cnt[ix]);
			}

			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if (   (srg &SR_BLKTX)		// kein access auf piggy back register
			 && (ux >= 3)) continue;

		if ((ux <= 4) || (ux == 6) || (ux == 10)) {
			pciconf.op2_menu=key;
			printf("Value $%08lX ", config.ao_regs[ux]);
			config.ao_regs[ux]=Read_Hexa(config.ao_regs[ux], -1);
			if (errno) continue;
		} else {
			if ((ux >= 7) && (ux <= 9)) {
				pciconf.op2_menu=key;
				printf("LVD addr  0x%03X ", (u_short)config.ao_regs[ux]);
				config.ao_regs[ux]=Read_Hexa(config.ao_regs[ux], -1) &0xFFF;
				if (errno) continue;

				if (ux == 9) {
					printf("Value $%08lX ", config.ao_regs[15]);
					config.ao_regs[15]=Read_Hexa(config.ao_regs[15], -1);
					if (errno) continue;
				}

			} else continue;
		}

		switch (ux) {

case 0:
			lvd_regs->sr=config.ao_regs[ux]; break;

case 1:
			lvd_regs->cr=config.ao_regs[ux]; break;

case 2:
			lvd_regs->lvd_block=config.ao_regs[ux]; break;

case 3:
			lvd_regs->lvd_scan_win=config.ao_regs[ux]; break;

case 4:
			lvd_regs->lvd_scan=config.ao_regs[ux]; break;

case 6:
			lvd_regs->lvd_blk_max=config.ao_regs[ux]; break;

case 7:
			hd=0;
			printf("register %02X slave %2d is %04X\n",
					 (u_char)config.ao_regs[ux], ((u_short)config.ao_regs[ux] >>8) &0xF,
					 regs[(u_short)config.ao_regs[ux] &0xFFF]);
			break;

case 8:	{
			u_int	wnd;

			hd=0;
			wnd=(u_int)lvd_regs->lvd_scan_win;
			lvd_regs->lvd_scan_win=(0xFF <<8) |config.ao_regs[ux];
			printf("register %02X broadcast is %04X\n",
					 (u_char)config.ao_regs[ux], 
					 regs[(u_char)config.ao_regs[ux]]);
			lvd_regs->lvd_scan_win=wnd;
			break;
			}

case 9:
			printf("register %02X slave %2d set to %04X\n",
					 (u_char)config.ao_regs[ux], ((u_short)config.ao_regs[ux] >>8) &0xF,
					 (u_short)config.ao_regs[15]);
			regs[(u_short)config.ao_regs[ux] &0xFFF] =(u_short)config.ao_regs[15];
			break;

		}
	}
}
//
//--------------------------- spec_test -------------------------------------
//
static int spec_test(void)
{
	return CEN_PROMPT;
}
//
//============================================================================
//                            main menu                                      =
//============================================================================
//
//--------------------------- MainMenu ---------------------------------------
//
static void MainMenu(void)
{

#define SP_		20
#define ZL_    5

	u_int		ux;
	char		key;
	int		ret;
	u_long	ix;
	u_long	iy;

	if ((pciconf.ms_count > 10000) || (pciconf.ms_count < 100))
		pciconf.ms_count=1500;

	if (lvd_ok) {
		Writeln();
		printf("Bestimmung der Zeitbasis l„uft\n");
		iy=pciconf.ms_count *1100;

		do {
			timemark();
			ix=0; key=0;
			do {
				if (plx_regs->las0ba) key++;		// do something
				ix++;
			} while (ix < iy);

			ux=(u_int)ZeitBin();
			if (!ux) ux=1;
			pciconf.ms_count=ix /(10L*ux);
			iy=1100*pciconf.ms_count;
			printf("%6u", ux);
		} while (ux <= 100);

		Writeln();
		ret=alloc_dma_buffer(0);				// allocate
		if (!phypages || debug) {
			if (ret > CE_PROMPT) display_errcode(ret);
			WaitKey(0);
		}
	}

	if (lvd_ok && pup && (pciconf.main_menu == '5')) application_dial(1);

	pup=0;
	while (1) {
		clrscr();
		gotoxy(SP_, 1);
		if (!lvd_ok) {
			printf("no LVD Master found");
		}

		gotoxy(SP_, 2);
		cprintf("System Memory =%u MByte", (u_int)(sysram >>20));

		gotoxy(SP_, 3);
		if (pci_req) { gotoxy(SP_, 4); int_logging(); }
		else { lowvideo(); cprintf("\r\n"); }

		ux=0;
		gotoxy(SP_, ux+++ZL_);
						printf("Konfigurierungs Menu ............0");
		gotoxy(SP_, ux+++ZL_);
						printf("PCI BIOS Menu ...................1");
		gotoxy(SP_, ux+++ZL_);
						printf("PCI BUS Utilities ...............2");
		if (lvd_ok) {
			gotoxy(SP_, ux+++ZL_);
							printf("LVD BUS MASTER Register .........3");
			gotoxy(SP_, ux+++ZL_);
							printf("Application Utilities ...........5");
			gotoxy(SP_, ux+++ZL_);
							printf("special test ....................6");
			gotoxy(SP_, ux+++ZL_);
							printf("set number of DMA pages..........9");
			gotoxy(SP_, ux+++ZL_);
							printf("clear and show DMA pages ........A");
			gotoxy(SP_, ux+++ZL_);
							printf("display DMA buffer ..............B");

			if (pci_irq) {
				gotoxy(SP_, ux+++ZL_);
				if (!int_vec) {
							printf("enable PCI Interrupt ............I");
				} else {
							printf("disable PCI Interrupt ...........I");
				}
			}
			gotoxy(SP_, ux+++ZL_);
							printf("FPGA reset ......................R");
		}
		gotoxy(SP_, ux+++ZL_);
						printf("master reset (PXL) ..............M");
		gotoxy(SP_, ux+++ZL_);
						printf("Error Logging ...................E");
		gotoxy(SP_, ux+++ZL_);
						printf("Ende ...........................^C");
		gotoxy(SP_, ux+ZL_);
						printf("                                 %c ",
								 pciconf.main_menu);

		key=toupper(getch());
		if (key == CTL_C) return;

		while (1) {
			use_default=(key == TAB);
			if (use_default || (key == CR)) key=pciconf.main_menu;

			clrscr(); gotoxy(1, 2);
			errno=0; ret=CEN_MENU;
			switch (key) {

		case '0':
				Install_Int(0);
				Konfigurierung(FZJ_VENDOR_ID, LVDBUS_DEVICE_ID);
				plx_init();
				break;

		case '1':
				ret=pci_bios_dial(); break;

		case '2':
				if (pci_ok != 2) break;

				ret=region_access_menu(reg_info, bus_error);
				break;

		case '3':
				if (!lvd_ok) break;

				ret=cmc_op_regs();
				break;

		case '5':
				if (!lvd_ok) break;

				ret=application_dial(0);
				break;

		case '6':
				if (!lvd_ok) break;

				ret=spec_test();
				break;

		case '9':
				if (!lvd_ok) break;

				Install_Int(0);
				ret=alloc_dma_buffer(1);
				break;

		case 'A':
				if (!lvd_ok) break;

				ret=dma_buffer(0);
				break;

		case 'B':
				if (!lvd_ok) break;

				ret=show_dmabuf(-1);
				break;

		case 'E':
				ret=err_logging(); break;

		case 'I':
				if (!lvd_ok) break;

				if (!pci_irq) break;

				Install_Int(int_vec == 0);
				break;

		case 'M':
				ret=PLX_reset(); break;

		case 'R':
				if (!lvd_ok) break;

				ret=FPGA_reset(); break;

			}

			while (kbhit()) getch();
			if ((ret >= 0) && (ret <= CE_PROMPT)) pciconf.main_menu=key;

			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
//
//----------------------------------------------------------------------------
//										Usage
//----------------------------------------------------------------------------
//
int Usage(void)
{
	printf("usage: %s [/<options>]\n", ProgName);
	printf("       /i      PCI device index\n");
	printf("       /c      save conifg to current directory\n");
	printf("       /d      debug output\n");
	printf("       /n      no auto startup\n\n");
	return 1;
}


//----------------------------------------------------------------------------
//										Start
//----------------------------------------------------------------------------
//
static char		savecnf;

int Start(int argc, char *argv[])
{
	int	ix,iy,iz;
	char	wrk[80];

	sysram=mem_size();
	debug=0; savecnf=0; pup=1;
	setmem(&pdev, sizeof(pdev), 0);		// PCI BIOS Parameter
	ix=1; iy=0;
	while (ix < argc) {
		strcpy(wrk, argv[ix]); str2up(wrk);
		if (wrk[0] == '/') {
			iz=1;
			while (wrk[iz]) {
				switch (wrk[iz]) {
	case '/':	break;

	case '?':	return Usage();

	case 'C':	savecnf=1; break;

	case 'D':	debug=1; pup=0; break;

	case 'I':	iz++;
					if ((wrk[iz] < '0') || (wrk[iz] > '9')) return Usage();

					pciconf.c_index=wrk[iz]-'0';
					break;

	case 'N':	pup=0; break;

	case 'S':	pup=2; break;

	default:
					printf("Option %c ist nicht erlaubt\n", wrk[iz]); Writeln();
					return Usage();
				}

				iz++;
			}
		} else {
			iy++;
		}

		ix++;
	}

	if (iy) return Usage();

	if (pci_attach(&pciconf.region, LVDBUS_DEVICE_ID, 0)) return 1;

	plx_init();
	return 0;
}
//
//----------------------------------------------------------------------------
//										main
//----------------------------------------------------------------------------
//
int main(int argc, char *argv[])
{
	int   cnf;
	int	ix,iy;
	int	len;
	char	cnf0_name[80];
	char	rootcnf0;
	char	cnf1_name[80];
	char	rootcnf1;

	textmode(C4350);
	printf(
"\n------------ test program for LVD DAQ System ----------------\n\n");

	ix=strlen(argv[0]);
	if (ix >= sizeof(root)) {
		printf("can't handle full program name\n");
		return 0;
	}

#if   (sizeof(VTEX_RG) != 0x80)  || (sizeof(VTEX_BC) != 0x80)			//
	|| (sizeof(SYMST_RG) != 0x80) || (sizeof(SYMST_BC) != 0x80)			//
	|| (sizeof(SYSLV_RG) != 0x80) || (sizeof(SYSLV_BC) != 0x80)			//
	|| (sizeof(GPX_RG) != 0x80) || (sizeof(GPX_BC) != 0x80)				//
	|| (sizeof(FQDCT_RG) != 0x80) || (sizeof(FQDCT_BC) != 0x80)
	printf("VTEX_RG:  %02X,  VTEX_BC:  %02X\n", sizeof(VTEX_RG), sizeof(VTEX_BC));
	printf("SYMST_RG: %02X,  SYMST_BC: %02X\n", sizeof(SYMST_RG), sizeof(SYMST_BC));
	printf("SYSLV_RG: %02X,  SYSLV_BC: %02X\n", sizeof(SYSLV_RG), sizeof(SYSLV_BC));
	printf("GPX_RG:   %02X,  GPX_BC:   %02X\n", sizeof(GPX_RG), sizeof(GPX_BC));
	printf("FQDCT_RG:   %02X,  FQDCT_BC:   %02X\n", sizeof(FQDCT_RG), sizeof(FQDCT_BC));
	WaitKey(0);
#endif

#if   (sizeof(FQDC_RG) != 0x80) || (sizeof(FQDC_BC) != 0x80)
	printf("FQDC_RG:  %02X,  FQDC_BC:  %02X\n", sizeof(FQDC_RG), sizeof(FQDC_BC));
	WaitKey(0);
#endif

#if   (sizeof(SQDCT_RG) != 0x80) || (sizeof(SQDCT_BC) != 0x80)
	printf("SQDCT_RG:  %02X,  SQDCT_BC:  %02X\n", sizeof(SQDCT_RG), sizeof(SQDCT_BC));
	WaitKey(0);
#endif

#if   (sizeof(SQDC_RG) != 0x80) || (sizeof(SQDC_BC) != 0x80)
	printf("SQDC_RG:  %02X,  SQDC_BC:  %02X\n", sizeof(SQDC_RG), sizeof(SQDC_BC));
	WaitKey(0);
#endif

	while (ix) {
		ix--;
		if ((argv[0][ix] == '/') || (argv[0][ix] == '\\') ||
			 (argv[0][ix] == ':')) { ix++; break; }
	}

	strcpy(root, argv[0]);
	root[ix]=0;
	iy=0;
	while (1) {
		ProgName[iy]=argv[0][ix++];
		if ((ProgName[iy] == '.') || !ProgName[iy]) { ProgName[iy]=0; break; }

		iy++;
	}
//
//--- Konfigurierung laden
//
//    zuerst default Werte setzten.
//
	setmem(&config, sizeof(config), 0);
	pciconf.ms_count  =1000;
	pciconf.device_id =LVDBUS_DEVICE_ID;
	pciconf.vendor_id =FZJ_VENDOR_ID;
	pciconf.bmen_device_id=LVDBUS_DEVICE_ID;
	pciconf.bmen_vendor_id=FZJ_VENDOR_ID;
	rootcnf0=0;
	strcpy(cnf0_name, ProgName);	// first try current directory
	strcat(cnf0_name, ".cnf");
	if ((cnf=_rtl_open(cnf0_name, O_RDONLY)) == -1) {
		rootcnf0=1;
		strcpy(cnf0_name, root);
		strcat(cnf0_name, ProgName);
		strcat(cnf0_name, ".cnf");
		cnf=_rtl_open(cnf0_name, O_RDONLY);
	}

	if (cnf != -1) {
		if ((len=read(cnf, &config, sizeof(config))) != sizeof(config)) {
			if (len < 0) perror("error reading config file");
			else printf("illegal config length\n");

         WaitKey(ESC);
		}

		close(cnf);
	}

	rootcnf1=0;
	strcpy(cnf1_name, ProgName);	// first try current directory
	strcat(cnf1_name, "_uc");
	strcat(cnf1_name, ".cnf");
	if ((cnf=_rtl_open(cnf1_name, O_RDONLY)) == -1) {
		rootcnf1=1;
		strcpy(cnf1_name, root);
		strcat(cnf1_name, ProgName);
		strcat(cnf1_name, "_uc");
		strcat(cnf1_name, ".cnf");
		cnf=_rtl_open(cnf1_name, O_RDONLY);
	}

	if (cnf != -1) {
		if ((len=read(cnf, &UCLastKey, sizeof(UCLastKey))) != sizeof(UCLastKey)) {
			if (len < 0) perror("(UC) error reading config file");
			else printf("(UC) illegal config length\n");

			WaitKey(ESC);
		}

		close(cnf);
	}

	if (Start(argc, argv)) return 0;

	for (ix=0; ix < C_WBUF; ix++)
		if ((abuf[ix]=calloc(0x8000, 2)) == 0) {
			printf("can't allocate abuf buffer (is %06lX)\n", (u_long)ix<<16);
			return -1;
		}

	MainMenu();
	Writeln();
	Install_Int(0);
	if (savecnf && rootcnf0) {
		strcpy(cnf0_name, ProgName);	// write to current directory
		strcat(cnf0_name, ".cnf");
	}

	if ((cnf=_rtl_creat(cnf0_name, 0)) != -1) {
		if (write(cnf, &config, sizeof(config)) != sizeof(config)) {
			perror("error writing config file");
			printf("name =%s", cnf0_name);
		}

		close(cnf);
	} else {
		perror("error creating config file");
		printf("name =%s", cnf0_name);
	}

	if (savecnf && rootcnf1) {
		strcpy(cnf1_name, ProgName);	// write to current directory
		strcat(cnf1_name, "_uc");
		strcat(cnf1_name, ".cnf");
	}

	if ((cnf=_rtl_creat(cnf1_name, 0)) != -1) {
		if (write(cnf, &UCLastKey, sizeof(UCLastKey)) != sizeof(UCLastKey)) {
			perror("error writing config file");
			printf("name =%s", cnf1_name);
		}

		close(cnf);
	} else {
		perror("error creating config file");
		printf("name =%s", cnf1_name);
	}

	return 0;
}
