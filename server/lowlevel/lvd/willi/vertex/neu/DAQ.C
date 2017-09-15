// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	daq.c -- DAQ Input Testprogramm with SIS1100 PCI module

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
#include "daq.h"

char			gigal_ok;
CONFIG		config;		// will be imported at pci.c, pciconf.c and pcibios.c
PLX_REGS		*plx_regs;
u_char		*eeprom;
GIGAL_REGS	*gigal_regs;	// local register
CM_REGS		*cm_reg;			// CMASTER register
BUS1100		*bus1100;

u_int		*abuf[C_WBUF];
char		UCLastKey[20] = {'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};

static char		pup;			// power up
static char		root[100];
static char		ProgName[20];

static void interrupt	(*intsave)();	// save original interrupt handler
static u_int	pci_irq;
static u_char	masksave =0xFF;	// saved interrupt mask
static u_int	IntCtrl;				// IO port, 0x21 or 0xA1
static u_int	int_vec  =0;		// vector for InterruptHandler
											//  0: no InterruptHandler installed
static u_long	deadlocks;
static u_int	timeouts;
static char		pci_req;				// interrupt arrived
static u_int	int_cnt;				// interrupt counter
static u_long	isr_intcsr;			// last plx->intcsr value
static u_int	isr_dma[2];			// Int DMA counter
static u_int	isr_ifsr;			// Interface Status Register
static u_long	doorbell;			// doorbell bits causing interrupt

static u_long	g_perr;
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

u_long	*dmadesc[2]={0};					// DMA descriptor buffer
u_long	dmadesc_phy[2]={0};
//
//--------------------------- display_errcode --------------------------------
//
void display_errcode(int err)
{
	if ((err >= CEN_PROMPT) && (err <= CE_PROMPT)) return;

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
//--------------------------- perr_txt ---------------------------------------
//
// return protocol error text
//
char *perr_txt(u_int perr)
{
static char	buf[40];

	if (perr == 0) return "";

	switch (perr >>8) {
case 0:
		strcpy(buf, "int: ");		// intermediate
		break;

case 1:
		strcpy(buf, "loc: ");
		break;

case 2:
		strcpy(buf, "rem: ");
		break;

default:
		strcpy(buf, "???: ");
		break;
	}

	switch (perr &0xFF) {
case PE_SYNCH:
		strcat(buf, "no Synch"); break;

case PE_NRDY:
		strcat(buf, "not Ready, Inhibit"); break;

case PE_XOFF:
		strcat(buf, "FIFO partial full"); break;

case PE_RESOURCE:
		strcat(buf, "no DMD/PIPE resource"); break;

case PE_DLOCK:
		strcat(buf, "deadlock"); break;

case PE_PROT:
		strcat(buf, "protocol violation"); break;

case PE_TO:
		strcat(buf, "protocol timeout"); break;

case PE_BERR:
		strcat(buf, "memory read/write bus error"); break;

case PE_BLK_ND:
		strcat(buf, "block read, no data at all"); break;

case PE_BLK_BERR:
		strcat(buf, "block read, bus error"); break;

default:
		strcat(buf, "?"); break;
	}

	return buf;
}
//
//--------------------------- clr_prot_error --------------------------------
//
void clr_prot_error(void)
{
	if (gigal_regs->sr &SR_DMA0_BLOCKED) gigal_regs->cr =CR_RESET;

	delay(2);
	gigal_regs->balance =gigal_regs->prot_err;	// clear balance and prot_err
	gigal_regs->sr      =gigal_regs->sr;
	gigal_regs->opt_csr =gigal_regs->opt_csr;
	g_perr=0;
}
//
//--------------------------- protocol_error --------------------------------
//
int protocol_error(u_long lp, char *id)
{
	int		ret;
	u_int		dlk;
	u_int		tout;
	u_long	st;
	u_long	tmp;

	ret=0; dlk=0; tout=0;
	while (1) {
		g_perr=gigal_regs->prot_err;
		if ((g_perr != PE_DLOCK) && (g_perr != PE_TO)) break;

		if (g_perr == PE_DLOCK) {
			deadlocks++;
			if (++dlk >= 10000) break;

			_delay_(500);
		} else {
			timeouts++;
			if (++tout >= 100) break;
		}
	}

	st=gigal_regs->sr;
	if (!(st &(SR_PROT_ERROR|SR_PROT_END)) && !g_perr) return 0;

	tmp=(g_perr >= 0x100) ? (SR_PROT_ERROR|SR_PROT_END) : SR_PROT_END;
	if ((st &(SR_PROT_ERROR|SR_PROT_END)) == tmp)
		gigal_regs->sr=tmp;
	else {
		printf("\r");
		if ((long)lp != -1) printf((id) ? "%10lu" : "%10lu: ", lp);
		if (id) printf(" %s: ", id);

		printf("illegal interrupt, SR=%08lX\n", gigal_regs->sr);
		use_default=0;
		if (WaitKey(0)) ret=CEN_PROMPT;

		gigal_regs->sr=tmp;	//?
	}

	if (g_perr == 0) return ret;

	printf("\r");
	if ((long)lp != -1) printf((id) ? "%10lu" : "%10lu: ", lp);
	if (id) printf(" %s: ", id);

	printf("protocol error 0x%03lX %s", g_perr, perr_txt((u_int)g_perr));
	if (dlk) printf(", %u interlocks", dlk);
	if (tout) printf(", %u timeouts", tout);
	Writeln();
	if ((tmp=gigal_regs->balance) != 0) {
		printf("balance counter: %04X\n", (u_int)tmp);
//			gigal_regs->balance=0;
	}

	return CEN_PROMPT;
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
	if (isr_intcsr &PLX_IDMA0_ACT) {		// DMA 0 active
		plx_regs->dmacsr[0] |=0x8;
		isr_dma[0]++;
	}

	if (isr_intcsr &PLX_IDMA1_ACT) {		// DMA 1 active
		plx_regs->dmacsr[1] |=0x8;
		isr_dma[1]++;
	}

	if (isr_intcsr &PLX_ILOC_ACT) {		// local interrupt
		tmp =gigal_regs->sr;
		gigal_regs->sr=tmp;
		isr_ifsr |=(u_int)tmp;
	}

	if (isr_intcsr &PLX_IDOOR_ACT) {		// doorbell interrupt
		tmp=plx_regs->l2pdbell;
		plx_regs->l2pdbell=tmp;
		doorbell |=tmp;
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
	cprintf("PCI Interrupt %5u: DMA=%u/%u, ifsr=0x%04X, dbell=0x%08lX",
			  int_cnt, isr_dma[0], isr_dma[1], isr_ifsr, doorbell);
	lowvideo(); cprintf("\r\n");
	pci_req=0; int_cnt=0; isr_dma[0]=0; isr_dma[1]=0; isr_ifsr=0; doorbell=0;
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

	pci_req=0; int_cnt=0; isr_dma[0]=0; isr_dma[1]=0; isr_ifsr=0; doorbell=0;
	if (gigal_ok) plx_regs->intcsr &=~PLX_INT_ENA;		// disable PCI Interrupt

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

	outportb(IntCtrl, masksave &~IntBit);
	plx_regs->dmacsr[0] |=0x8;				// clear DMA Interrupt
	plx_regs->dmacsr[1] |=0x8;
	gigal_regs->sr =CSR_INTMASK;			// clear all Interrupt
	plx_regs->intcsr
		=PLX_IDMA1_ENA|PLX_IDMA0_ENA|PLX_ILOC_ENA|PLX_IDOOR_ENA|PLX_INT_ENA;

	return 1;
}
//
//===========================================================================
//
//===========================================================================
//
//--------------------------- err_logging -----------------------------------
//
int err_logging(void)
{
	printf("Error Logging %4d ", pciconf.errlog);
	pciconf.errlog=(u_int)Read_Deci(pciconf.errlog, -1);
	return CEN_PROMPT;
}
//
//--------------------------- _delay_ ---------------------------------------
//
void _delay_(u_long dx)
{
static u_long tmp;

	tmp=0;
	while (tmp++ != dx);
}
//
//--------------------------- send_dmd_eot ----------------------------------
//
int send_dmd_eot(void)
{
	u_int	ix;

	if (plx_regs->dmacsr[0] &0x10) return 0;

	plx_regs->dma[0].dmamode |=0x4000;	// EOT enable
	gigal_regs->sr =SR_SEND_EOT;
	ix=0;
	while (1) {
		_delay_(100);
		if (plx_regs->dmacsr[0] &0x10) return 0;

		if (++ix >= 100) {
			printf("DMA 0 is hanging up, CSR=%02X\n", plx_regs->dmacsr[0]);
			return CE_PROMPT;
		}
	}
}
//
//--------------------------- dma_buffer ------------------------------------
//
int dma_buffer(
	u_int		mode)		// 0: display and clear pages
							// 1: clear pages
{
	u_int		i,j;
	u_long	u;

	if (dmapages == 0) return CE_NO_DMA_BUF;

	for (i=0; i < 2; i++)
		for (j=0; j < SZ_PAGE_L; dmadesc[i][j++]=0xCCCCCCCCL);

	for (u=0; u < (u_long)dmapages*SZ_PAGE_L; u++) DBUF_L(u)=0xCCCCCCCCL;

	if (mode != 0) return CEN_MENU;

	printf("page physical addresses\n");
	printf("  X: %08lX %08lX\n", dmadesc_phy[0], dmadesc_phy[1]);
	i=0;
	while (1) {
		if (!(i &0x7)) printf("%03X:", i);

		printf(" %08lX", dmabuf_phy[i++]);
		if (!(i &0x7)) Writeln();

		if (i >= dmapages) break;

		if (!(i&0xFF)) {
			if (WaitKey(0)) return CEN_PROMPT;
		}
	}

	if (i &0x7) Writeln();

	return CEN_PROMPT;
}
//
//--------------------------- alloc_dma_buffer ------------------------------
//
static int _search_phy(void)
{
#define TEST_PAT	((u_long)0x19472000L)
#define TEST_DESC	0x1000L
#define _ADB_DEL0	10
#define _ADB_DEL1	100

	u_long	lin;
	u_int		ux;
	u_int		pg;		// # pages found
	char		fnd;
	u_int		tmp;
	u_int		tmp0,tmp1;
	u_long	pat;

	lin=0x100000L;
	plx_regs->bigend =0x00;						// no big endian
	plx_regs->dma[1].dmamode =0x0209C3L;	// IS||AC.BU|BT.RY|32
	plx_regs->dma[1].dmaladr =ADDRSPACE0+OFFSET(GIGAL_REGS, d_ad_h);
	plx_regs->dma[1].dmasize =1*4;
	plx_regs->dma[1].dmadpr  =0x0;			// read sysmem
	plx_regs->dmacsr[1] |=0x1;
	for (ux=0; ux < dmapages; ux++) dmabuf[ux>>4][(ux&0xF)<<10]=TEST_PAT+ux;

	*dmadesc[0] =TEST_PAT+TEST_DESC;
	*dmadesc[1] =TEST_PAT+TEST_DESC+1;
	pg=0;
	tmp0=tmp1=0;
	do {
		plx_regs->dma[1].dmapadr =lin;
		plx_regs->dmacsr[1] =0x3;
		tmp=0;
		while (!(plx_regs->dmacsr[1] &0x10)) {
			_delay_(_ADB_DEL0);
			if (++tmp >= _ADB_DEL1) {
				printf("0: DMA 1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
				return CE_PROMPT;
			}
		}

		if (tmp > tmp0) tmp0=tmp;
		pat=gigal_regs->d_ad_h;
		if ((pat &~(TEST_DESC-1)) == TEST_PAT) {
			ux=0; fnd=0;
			do {
				if ((pat &(TEST_DESC-1)) != ux) continue;

				dmabuf[ux>>4][(ux&0xF)<<10]=~TEST_PAT;
				plx_regs->dmacsr[1] =0x3;
				tmp=0;
				while (!(plx_regs->dmacsr[1] &0x10)) {
					_delay_(_ADB_DEL0);
					if (++tmp >= _ADB_DEL1) {
						printf("1: DMA1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
						return CE_PROMPT;
					}
				}

				if (tmp > tmp1) tmp1=tmp;

				if (gigal_regs->d_ad_h == ~TEST_PAT) {
					if (fnd || dmabuf_phy[ux]) {
						printf("suche nicht eindeutig\n");
						return CE_PROMPT;
					}

					if (debug) printf("pg %2u: phy %08lX\n", ux, lin);

					dmabuf_phy[ux]=lin; pg++;
				}

				dmabuf[ux>>4][(ux&0xF)<<10]=TEST_PAT+ux;

			} while (++ux < dmapages);

		} else {
			for (ux=0; ux < 2; ux++) {
				if (pat == TEST_PAT+TEST_DESC+ux) {
					*dmadesc[ux] =~(TEST_PAT+TEST_DESC+ux);
					plx_regs->dmacsr[1] =0x3;
					tmp=0;
					while (!(plx_regs->dmacsr[1] &0x10)) {
						_delay_(_ADB_DEL0);
						if (++tmp >= _ADB_DEL1) {
							printf("1: DMA1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
							return CE_PROMPT;
						}
					}

					if (gigal_regs->d_ad_h == ~(TEST_PAT+TEST_DESC+ux)) {
						printf("pg  X: phy %08lX\n", lin);
						if (dmadesc_phy[ux]) {
							printf("desc suche nicht eindeutig\n");
							return CE_PROMPT;
						}

						dmadesc_phy[ux]=lin;
					}

					*dmadesc[ux] =TEST_PAT+TEST_DESC+ux;
				}
			}
		}

		if (dmadesc_phy[0] && dmadesc_phy[1] && (pg >= dmapages)) {
			if (debug) printf(" %u %u\n", tmp0, tmp1);

			return 0;
		}

		if (!(lin &0x00FFFFFFL) && kbhit()) {
			printf(" abort at lin %08lX\n", lin); return CE_PROMPT;
		}

		lin +=SZ_PAGE;

	} while (lin < sysram);

	if (dmadesc_phy[0]) pg++;
	if (dmadesc_phy[1]) pg++;
	printf("only %03X pages found\n", pg);
	return CE_PROMPT;
}


static int alloc_dma_buffer(
	u_int		mode)		// 0: allocate without dialog
							// 1: dialog for number of pages
{
static u_long	tmp;

	DPMIREGS	regs;
	u_short	sel;
	u_int		ix;
	u_int		pgs,sz;
	u_long	lin;
	u_long	len;

	if (sysram == 0) {
		printf("unknown size of System Memory\n"); return CEN_PROMPT;
	}

	if (mode == 1) {
		printf("# of phys pg 0x%03X ", config.phys_pg);
		config.phys_pg=(u_int)Read_Hexa(config.phys_pg, DMA_PAGES);
		if (errno) return CEN_MENU;

		if (config.phys_pg == 0) return CEN_PROMPT;

		if (dmabuf[0]) {
			printf("restart the program\n");
			return CEN_PROMPT;
		}
	}

	if (!dmadesc[0]) {
		for (ix=0; ix < 2; ix++) {
			//
			//--- allocate DMA buffer
			//
			regs.ax=0x0501;					// Allocate Memory Block
			regs.bx=0;
			regs.cx=SZ_PAGE;
			if (DPMI_call(&regs)) {
				printf("1: %04X\n", regs.ax);
				printf("can't allocate DMA buffer\n");
				dmabuf[0]=0;
				dmapages =0;
				return CEN_PROMPT;
			}

			lin=((u_long)regs.bx <<16) |regs.cx;
			if (debug) printf("lin addr=  %08lX\n", lin);

			if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
				printf("can't allocate selector\n");
				dmabuf[0]=0;
				dmapages =0;
				return CEN_PROMPT;
			}

			SetSelectorLimit(sel, SZ_PAGE-1);
			SetSelectorBase(sel, lin);
			dmadesc[ix]=MAKELP(sel, 0);
		}
	}

	if (!dmabuf[0]) {
		if (config.phys_pg == 0) return CE_NO_DMA_BUF;

		if (config.phys_pg > DMA_PAGES) config.phys_pg=DMA_PAGES;
		//
		//--- allocate DMA buffer
		//
		dmapages=config.phys_pg; pgs=dmapages; sz=16; ix=0;
		do {
			if (sz > pgs) sz =pgs;

			pgs -=sz;
			len=(u_long)sz *SZ_PAGE;
			regs.ax=0x0501;					// Allocate Memory Block
			regs.bx=(u_short)(len >>16);
			regs.cx=(u_short)len;
			if (DPMI_call(&regs)) {
				printf("2: %04X\n", regs.ax);
				printf("can't allocate DMA buffer\n");
				dmabuf[0]=0;
				dmapages =0;
				return CEN_PROMPT;
			}

			lin=((u_long)regs.bx <<16) |regs.cx;
			if (debug) printf("lin addr=  %08lX\n", lin);

			if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
				printf("can't allocate selector\n");
				dmabuf[0]=0;
				dmapages =0;
				return CEN_PROMPT;
			}

			SetSelectorLimit(sel, len-1);
			SetSelectorBase(sel, lin);
			dmabuf[ix++]=MAKELP(sel, 0);
		} while (pgs);
	}

	if (!gigal_ok) return CEN_PROMPT;
	//
	//--- search for physical address
	//
	if (_search_phy()) dmapages =0;

	if (dmapages == 0) {
		printf("no or not all phys pages found\n");
		return CEN_PROMPT;
	}

	dma_buffer(1);
	return CEN_PROMPT;
}
//
// end ---------------------- alloc_dma_buffer -------------------------------
//
//--------------------------- show_dmabuf -----------------------------------
//
static int show_dmabuf(int pg)
{
static int	blk;

	u_int		i;
	u_long	u;
	u_int		ln;

	if (!dmabuf[0]) return CE_NO_DMA_BUF;

	if (pg < -2) {
		if (blk < 0) printf("number of block %5d ", blk);
		else printf("number of block 0x%03X ", blk);
		blk=(u_int)Read_Hexa(blk, -1);
		if (errno) return CEN_MENU;

		pg=blk;
	}

	if (pg < 0) {
		pg=-pg-1;
		if (pg >= 2) return CE_PARAM_ERR;

		i=0; ln=0;
		while (1) {
			if (!(i &0x7)) printf("%04X: ", (pg<<12)+4*i);

			printf(" %08lX", dmadesc[pg][i++]);
			if (i >= SZ_PAGE_L) {
				Writeln();
				break;
			}

			if (!(i &0x7)) {
				printf("\n");
				if (++ln >= 32) {
					if (WaitKey(0)) return CEN_PROMPT;

					ln=0;
				}
			}

		}

		return CEN_PROMPT;
	}

	if ((u_int)pg >= dmapages) pg=dmapages-1;

	u=(u_long)SZ_PAGE_L*pg; ln=0;
	while (1) {
		if (!((u_int)u &0x7)) printf("%06lX:", 4*u);

		printf(" %08lX", DBUF_L(u)); u++;
		if (u >= (u_long)SZ_PAGE_L*dmapages) {
			Writeln();
			break;
		}

		if (!((u_int)u &0x7)) {
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
//--------------------------- set_initiator_window ---------------------------
//
void set_initiator_window(
	u_int	pg)	// number of pages to be fit in
					// 0: disable local to PCI access and initialize DMA buffer
{
	u_int		ux;
	u_long	msk;

	if ((dmapages == 0) || (pg == 0)) {
		plx_regs->dmpbam=0;	// disable local to PCI access
		dma_buffer(1);
		return;
	}

   msk=0xFFFF0000L;
	if (pg > dmapages) pg =dmapages;

	ux=1;
	while (ux < pg) {
		while ((dmabuf_phy[0] ^dmabuf_phy[ux]) &msk) msk <<=1;
		ux++;
	}

	plx_regs->dmrr  =msk;
	plx_regs->dmlbam=dmabuf_phy[0] &msk;
	plx_regs->dmpbam=(dmabuf_phy[0] &msk) |0x1;
}
//
//------------------------------ FPGA_reset ----------------------------------
//
static int FPGA_reset(void)
{

	gigal_regs->cr =CR_RESET;
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

		dma_buffer(1);
		gigal_regs->cr=CR_READY;
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

	gigal_ok=0;
	if (pci_ok != 2) return;

	pci_irq=pciconf.region.int_line;
	if (pci_irq >= 16) pci_irq=0;

	if (debug) {
		printf("int line = %u\n", pciconf.region.int_line);
		WaitKey(0);
	}

	plx_regs=(PLX_REGS*)pciconf.region.r0_regs;
	eeprom  =((u_char*)&plx_regs->cntrl)+3;
	if ((gigal_regs=pci_ptr(&pciconf.region, 2, 0)) == 0) return;

	if ((cm_reg=pci_ptr(&pciconf.region, 2, 0x800)) == 0) return;

	plx_regs->intcsr =PLX_ILOC_ENA;			// enable LINT Interrupt
	gigal_regs->bus_descr[0].hdr=HW_R_BUS;
	gigal_regs->bus_descr[0].ad =0;
	if ((bus1100=pci_ptr(&pciconf.region, 3, 0)) == 0) return;

	gigal_ok=1;
}
//
//--------------------------- tpm_data ---------------------------------------
//
static int tpm_data(void)	// 0: TransparentMode Data read
{
	u_int	uy;
	u_int	cx;

	if (!(gigal_regs->sr &(SR_TP_DATA|SR_TP_SPECIAL))) return 1;

	cx=0;
	do {
		uy=0;
		while (gigal_regs->sr &SR_TP_DATA) {
			if ((uy &7) == 0) {
				if (!uy) printf("da: "); else printf("    ");
			}

			printf(" %08lX", gigal_regs->tp_data);
			if (!(++uy %8)) {
				printf("\n"); cx++;
				if (!(uy %(8*8))) {
					cx=0;
					if (WaitKey(0)) return 0;
				}
			}
		}

		if (uy %8) { printf("\n"); cx++; }
		uy=0;
		while ((gigal_regs->sr &(SR_TP_DATA|SR_TP_SPECIAL)) == SR_TP_SPECIAL) {
			if ((uy &7) == 0) {
				if (!uy) printf("sp: "); else printf("    ");
			}

			printf(" %08lX", gigal_regs->tp_special);
			if (!(++uy %8)) {
				printf("\n"); cx++;
				if (!(uy %(8*8))) {
					cx=0;
					if (WaitKey(0)) return 0;
				}
			}
		}

		if (uy %8) printf("\n"); cx++;

		if (cx >= 40) {
			cx=0;
			if (WaitKey(0)) return 0;
		}

	} while (gigal_regs->sr &(SR_TP_DATA|SR_TP_SPECIAL));

	return 0;
}
//
//--------------------------- bus_addr ---------------------------------------
//
static u_long bus_addr(char *fmt, u_long *cfg)
{
	u_int		pg;

	printf("\"FF0i....\" => i = physical page\n");
	printf((fmt) ? fmt : "sysmem      %08lX ", *cfg);
	*cfg=Read_Hexa(*cfg, -1);
	if (errno) return 0;

	if ((*cfg &0xFFF00000L) == 0xFF000000L) {
		pg=(u_char)(*cfg >>16);
		if (pg >= dmapages) {
			display_errcode(CE_NO_DMA_BUF);
			return 0;
		}

		return dmabuf_phy[pg] |((u_int)*cfg &(SZ_PAGE-1));
	}

	*cfg &=~3;
	return *cfg;
}
//
//------------------------------ temp_prot -----------------------------------
//
// --- temporary protocoll
//
static void temp_prot(
	int	rd)	// -1: full
					//  0: write
					//  1: read
{
	u_long	hdr;
	u_long	am;
	u_long	adr;
	u_long	data;
	u_int		sh;
	u_long	mk;

	if (rd < 0) {
		printf("header       %08lX/%08lX ", gigal_regs->t_hdr, config.x_hdr);
		config.x_hdr=Read_Hexa(config.x_hdr, -1);
		if (errno) return;

		hdr =config.x_hdr;
		if (hdr &HW_AM) {
			printf("address modifier     %04X/%04X ", (u_int)gigal_regs->t_am, (u_int)config.x_am);
			config.x_am=(u_int)Read_Hexa(config.x_am, -1);
			if (errno) return;
		}

		am =config.x_am;
		printf("address      %08lX\n", gigal_regs->t_ad);
		adr=bus_addr("address               %08lX ", &config.x_ad);
		if (errno) return;

		if (config.x_hdr &HW_A64) {
			printf("address high %08lX/%08lX ", gigal_regs->t_ad_h, config.x_ad_h);
			config.x_ad_h=Read_Hexa(config.x_ad_h, -1);
			if (errno) return;
		}

		if ((config.x_hdr &HW_WR) || (config.x_hdr &HW_BT)) {
			printf("data         %08lX/%08lX ", gigal_regs->t_da, config.x_da);
			config.x_da=Read_Hexa(config.x_da, -1);
			if (errno) return;
		}

		data =config.x_da;
		if ((config.x_hdr &HW_WR) && (config.x_hdr &HW_A64)) {
			printf("data high    %08lX/%08lX ", gigal_regs->t_da_h, config.x_da_h);
			config.x_da_h=Read_Hexa(config.x_da_h, -1);
			if (errno) return;
		}

	} else {
		hdr =0x00010000L;
		printf("address modifier %04X ", config.xx_am);
		config.xx_am=(u_int)Read_Hexa(config.xx_am, -1);
		if (errno) return;

		if ((am=config.xx_am) != 0) hdr |=HW_AM;

		printf("address      %08lX ", config.xx_addr);
		config.xx_addr=Read_Hexa(config.xx_addr, -1);
		if (errno) return;

		adr=config.xx_addr;
		printf("size 0/1/2         %2u ", config.xx_size);
		config.xx_size=(u_int)Read_Deci(config.xx_size, 2);
		if (errno) return;

		sh=8*((u_int)adr &3);
		mk=(config.xx_size == 0) ? 0xFFL   :
			(config.xx_size == 1) ? 0xFFFFL : 0xFFFFFFFFL;
		hdr |= ((u_long)((1 << (1 <<config.xx_size)) -1) <<(24+((u_int)adr &3)));
		if (!rd) {
			hdr |=HW_WR;
			printf("data         %08lX ", config.xx_data);
			config.xx_data=Read_Hexa(config.xx_data, -1);
			if (errno) return;
		}

		data =(config.xx_data &mk) <<sh;
	}

	gigal_regs->t_hdr =(hdr &~0xFF) |0x2;
	gigal_regs->t_am  =am;
	gigal_regs->t_ad_h=config.x_ad_h;
	gigal_regs->t_da  =data;
	gigal_regs->t_da_h=config.x_da_h;
	gigal_regs->t_ad  =adr;

	if (protocol_error(-1, 0) || (hdr &HW_WR)) return;

	printf("data =0x%08lX\n", (gigal_regs->tc_da >>sh) &mk);
}
//
//------------------------------ gigal_op_regs -------------------------------
//
// show/modify AddOn Operation Registers
//
static const char *sr_txt[] ={
	"RxSy", "TxSy", "Inh", "Cnf",  "TxCh", "InhCh", "SemCh", "RecV",
	"ResR", "Eot",  "Mbx", "Xof",  "Lem0", "Lem1",  "Pend",  "Perr",
	"DmdR", "PipR", "Rerr","Berr", "Tspc", "Tdat",  "?",     "?"
};

static const char *cr_txt[] ={
	"?",    "Tpm", "Rdy", "Big",  "TxCh", "InhCh", "SemCh", "RecV",
	"ResR", "Eot", "Mbx", "Xof",  "Lem0", "Lem1",  "Pend",  "Perr"
};

static const char *optcr_txt[] ={
	"Tst",  "Xoff", "Wait", "?",   "Lmo0", "Lmo1", "Led0", "Led1",
	"Lmi0", "Lmi1", "?",    "?",   "?",    "?",    "?",    "?",
	"TxWt", "Xon",  "Wunl", "?",   "TBer", "UOer", "DPer", "BDer",
	"BM0",  "BM1",  "BM2",  "BM3", "PEF",  "EF",   "PFF",  "FF"
};

#define _AO_REG	12		// bis 16

int gigal_op_regs(void)
{
	u_int		ux,uy;
	char		hgh;
	char		key;
	u_long	tmp;
	char		hd;
	u_long	*pci;

	hd=1;
	while (1) {
		errno=0;
		if (hd) {
			Writeln();
			if (pci_req) int_logging();

printf("Ident/HWver/FW/FWver          $%08lX  => %u/%u/%u/%u\n",
			gigal_regs->ident,
			(u_char)gigal_regs->ident,        (u_char)(gigal_regs->ident >>8),
			(u_char)(gigal_regs->ident >>16), (u_char)(gigal_regs->ident >>24));
printf("Status Register               $%08lX ..0 ", tmp=gigal_regs->sr);
			ux=24; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("Control Register              $%08lX ..1 ", tmp=gigal_regs->cr);
			ux=16; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("Transp special     write/read $%08lX ..2/s\n",
			config.ao_regs[2]);
printf("Transp data        write/read $%08lX ..3/s\n",
			config.ao_regs[3]);
printf("Optical CSR                   $%08lX ..4 ", tmp=gigal_regs->opt_csr);
			ux=32; hgh=0;
			while (ux--)
				if (tmp &((u_long)1L <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", optcr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("Register write/read                $%03X ..5/6\n",
			(u_int)config.ao_regs[6]);
printf("local to PCI doorbell         $%08lX ..7\n", plx_regs->l2pdbell);
printf("temporary protocol, write/read          ..8/9\n");
tmp=gigal_regs->prot_err_nw;
if (tmp) {
	g_perr=tmp;
//	tmp=gigal_regs->prot_err;
}
printf("last protocol error                $%03lX     %s\n", g_perr, perr_txt((u_int)g_perr));
printf("protocol balance              $%08lX ..a\n", gigal_regs->balance);
printf("t_hdr/t_ad/t_da/d0_bc %08lX/%08lX/%08lX/%08lX\n",
	gigal_regs->t_hdr, gigal_regs->t_ad, gigal_regs->t_da, gigal_regs->d0_bc);
printf("Reset/ClearStatus/MailboxRegs           ..R/C/X\n");
			printf("                                          %c ",
					 pciconf.opr_menu);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return 0;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=pciconf.opr_menu;
		}

		use_default=(key == TAB);
		if (use_default || (key == CR)) key=pciconf.opr_menu;

		printf("%c\n", key);
		if (key == 'R') {
			gigal_regs->cr =CR_RESET;
			g_perr=0;
			plx_regs->l2pdbell=0xFFFFFFFFL;
			uy=0;
			do plx_regs->mbox[uy++]=0; while (uy < 8);

			dma_buffer(1);
			continue;
		}

		if (key == 'C') {
			clr_prot_error();
			continue;
		}

		if (key == 'A') {
			gigal_regs->balance =0;
			continue;
		}

		if (key == ' ') continue;

		if ((key == '8') || (key == '9')) {
			pciconf.opr_menu=key;
			temp_prot(key -'8');
			hd=0;
			continue;
		}

		if (key == 'S') {
			gigal_regs->cr =CR_TRANPARENT;
			tpm_data();
			gigal_regs->cr =CR_TRANPARENT<<16;
			continue;
		}

		if (key == 'X') {
			uy=0; printf("PLX:");
			do {
				printf(" %08lX", plx_regs->mbox[uy]);
			} while (++uy < 8);

			Writeln();
			uy=0;
			do {
				if (!(uy &0x7)) {
					if (!uy) printf("Loc:");
					else printf("    ");
				}

				printf(" %08lX", gigal_regs->mailext[uy++]);
				if (!(uy &0x07)) {
					Writeln();
					if (!(uy &0x3F) && WaitKey(0)) break;
				}

			} while (uy < C_MAILEXT);

			hd=0;
			continue;
		}

		if ((key < '0') || (key  > '7')) continue;

		ux=key-'0';
		pciconf.opr_menu=key;
		if ((ux == 5) || (ux == 6)) {
			printf("RegAddr   $%04X ", (u_int)config.ao_regs[6]);
			config.ao_regs[6]=(u_int)Read_Hexa(config.ao_regs[6], -1);
			if (errno) continue;

			if ((pci=pci_ptr(&pciconf.region, 2, config.ao_regs[6])) == 0) {
				printf("illegal address\n"); hd=0; continue;
			}
		}

		if (ux != 6) {
			printf("Value $%08lX ", config.ao_regs[ux]);
			config.ao_regs[ux]=Read_Hexa(config.ao_regs[ux], -1);
			if (errno) continue;
		}

		switch (key) {

case '0':
			gigal_regs->sr=config.ao_regs[ux]; break;

case '1':
			gigal_regs->cr=config.ao_regs[ux]; break;

case '2':
			gigal_regs->tp_special=config.ao_regs[ux]; break;

case '3':
			gigal_regs->tp_data=config.ao_regs[ux]; break;

case '4':
			gigal_regs->opt_csr=config.ao_regs[ux]; break;

case '5':
			*pci=config.ao_regs[ux]; break;

case '6':
			printf("contents =%08lX\n", *pci); hd=0; break;

case '7':
			plx_regs->l2pdbell=config.ao_regs[ux]; break;

		}
	}
}
//--- end ------------------- gigal_op_regs ---------------------------------
//
//--------------------------- gigal_dma -------------------------------------
//
//--------------------------- get_addr ---------------------------------------
//
static u_long get_addr(u_long cfg)
{
	u_int		pg;

	if ((cfg &0xFF000000L) == 0xFF000000L) {
		pg=(u_char)(cfg >>16);
		if (pg < dmapages)
			return dmabuf_phy[pg] |((u_int)cfg &(SZ_PAGE-1));
	}

	return cfg &=~3;
}
//
//--------------------------- mark_key ---------------------------------------
//
static void mark_key(u_char key)
{
	if (key > ' ') printf("%c", key);
	printf(" ----------------------------\n");
}
//
static int gigal_dma(void)
{
	u_int		ux;
	char		hgh;
	char		key;
	char		hd;
	u_char	dmd_eot;
	u_long	bal;
	u_long	tmp;
#if _DMAWR_
	u_long	raddr;
	u_long	dmamd1;	// DMA mode for channel 1 (due logic scope triggering)

static const char *hdr_txt[] ={
	"Hdr", "Ad", "Da", "?",  "?",   "?",   "?",    "?",
	"?",   "?",  "WR", "AM", "A64", "Blk", "Fifo", "Eot"
};

#endif

	hd=1;
	while (1) {
		errno=0;
		if (hd == 1) {
			hd=0;
			Writeln();
			if (pci_req) int_logging();

printf("Status Register             $%08lX     ", tmp=gigal_regs->sr);
			ux=24; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("Control Register            $%08lX ..M ", tmp=gigal_regs->cr);
			ux=16; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("MBX register 0              $%08lX\n", gigal_regs->mailext[0]);
#if _DMAWR_
			tmp=gigal_regs->d_hdr;
printf("dma header ................ $%08lX ..0 BE:%02X, SP:%02X ",
			tmp, (u_char)(tmp>>24), (u_char)(tmp>>16));
			ux=16; hgh=0;
			while (ux--)
				if ((u_int)tmp &(0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", hdr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");
			if (tmp &HW_WR)			// write block
				dmamd1 =0x0241C3L;	//		IS/EOT/BU/BT.RY/32
			else							// pipelined read request
				dmamd1 =0x0209C3L;	//		IS//AC.BU/BT.RY/32

			if (int_vec) dmamd1 |=0x0400;	// IE

			if (tmp &HW_AM) {
printf("Address modifier ..........     $%04lX ..1\n", gigal_regs->d_am);
			}
printf("Address low ............... $%08lX ..2\n", gigal_regs->d_ad);
			if (tmp &HW_A64) {
printf("Address high .............. $%08lX ..3\n", gigal_regs->d_ad_h);
			}
printf("Byte count ................ $%08lX ..4\n", gigal_regs->d_bc);
			raddr=get_addr(config.dma_regs[7]);
printf("DMA len/addr/strt . $%06lX/$%08lX ..7/8\n", config.dma_regs[8], raddr);
#endif

			if (!(plx_regs->dmacsr[0] &0x10))
printf("stop ");
			else
printf("start");

printf(     " demand DMA channel...       $%02X ..9 ", plx_regs->dmacsr[0]);
			if (!(plx_regs->dmacsr[0] &0x10)) {
				printf("phy. page 1");
				if (plx_regs->dma[0].dmamode &0x4000) printf(", EOT");
			}
			Writeln();

printf("TC buffer ................. $%08lX ..A\n", gigal_regs->d0_bc_adr);
printf("TC buffer length .......... $%08lX ..B\n", gigal_regs->d0_bc_len);
printf("demand byte counter         $%08lX\n", gigal_regs->d0_bc);
printf("temporary protocol ........           ..T\n");
bal=gigal_regs->balance;
if (bal == 0) {
	tmp=gigal_regs->prot_err;
	if (tmp) g_perr=tmp;
}
printf("last protocol error              $%03lX     %s\n", g_perr, perr_txt((u_int)g_perr));
printf("protocol balance            $%08lX\n", bal);
printf("Reset/ClrStatus/PhyAdr/TPregs/SMem .....R/C/P/X/S\n");
			printf("                                        %c ",
					 config.dma_menu);
		}

		if (hd == -1) {
			printf("select> ");

			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) {
				hd=1; continue;
			}

			if (key == ' ') key=config.dma_menu;
		} else {
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return 0;
		}

		hd =1;
		use_default=(key == TAB);
		if (use_default || (key == CR)) key=config.dma_menu;

		if (key == ' ') continue;

		if (key == 'R') {
			mark_key(key);
			tmp=gigal_regs->cr;
			gigal_regs->cr=CR_RESET;
			gigal_regs->cr=tmp;
			g_perr        =0;
			continue;
		}

		if (key == 'C') {
			mark_key(key);
			gigal_regs->sr      =gigal_regs->sr;
			gigal_regs->balance =0;
			g_perr              =0;
			gigal_regs->d0_bc   =0;
			continue;
		}

		if (key == 'M') {
			mark_key(key);
			printf("Value $%08lX ", config.ao_regs[1]);
			config.ao_regs[1]=Read_Hexa(config.ao_regs[1], -1);
			if (errno) continue;

			gigal_regs->cr=config.ao_regs[1];
			continue;
		}

		if (key == 'P') {
			mark_key(key);
			printf("phy:"); ux=0;
			do {
				if (ux  && !(ux &0x7)) printf("\n    ");
				printf(" %08lX", dmabuf_phy[ux++]);
			} while ((ux < dmapages) && (ux < 3*8));

			Writeln();
			hd =-1;
			continue;
		}

		if (key == 'S') {
			mark_key(key);
			show_dmabuf(-1);
			hd =-1;
			continue;
		}

		if (key == 'T') {
			mark_key(key);
			config.dma_menu=key;
			temp_prot(-1);
			continue;
		}

		if (key == 'X') {
			mark_key(key);
			tpm_data();
			hd =-1;
			continue;
		}

		ux=key-'0';
		if (ux > 9) {
			if (ux >= 17) ux=ux-7;
			else ux =255;
		}

		if (ux > 11) { hd=0; continue; }

		mark_key(key);
		config.dma_menu=key;
		if ((ux >= 7) && (ux <= 9) && (dmapages < 3)) {
			display_errcode(CE_NO_DMA_BUF);
			continue;
		}

		if (ux == 7) {
			printf("Byte count     $%08lX ", config.dma_regs[8]);
			config.dma_regs[8]=Read_Hexa(config.dma_regs[8], -1) &~3L;
			if (errno) continue;

			if (config.dma_regs[8] > SZ_PAGE) config.dma_regs[8] =SZ_PAGE;

			bus_addr("Remote address $%08lX ", &config.dma_regs[7]);
			if (errno) continue;

			printf("source is phy. page 0\n");
//			verify_dmabuf(0, (u_int)config.dma_regs[8]);
			continue;
		}

#if _DMAWR_
		if (ux == 8) {
			plx_regs->dma[1].dmamode =
								dmamd1 |((gigal_regs->d_hdr &HW_FIFO) ? 0x0800 : 0);
			plx_regs->dma[1].dmapadr =dmabuf_phy[0];
			plx_regs->dma[1].dmaladr =raddr &~3;
			plx_regs->dma[1].dmasize =config.dma_regs[8];
			plx_regs->dma[1].dmadpr  =0x00;				// PCI2LOCAL

			isr_dma[1]=0; plx_regs->dmacsr[1] =0x3;
			ux=0;
			while (1) {
				_delay_(10000);
				if (int_vec ?
					 isr_dma[1] :
					 (plx_regs->dmacsr[1] &0x10)) break;

				if (++ux >= 100) {
					printf("DMA1 time out CSR=%02X\n", plx_regs->dmacsr[1]);
					if (WaitKey(0)) return 0;

					break;
				}
			}

			continue;
		}
#endif

		if (ux == 9) {
			if (!(plx_regs->dmacsr[0] &0x10)) {
				if (send_dmd_eot()) hd=-1;

				continue;
			}

			printf("destination is phy. page 1\n");
			dmd_eot=(config.dma_regs[9] &0x2) != 0;
			printf("enable EOT      %8u ", dmd_eot);
			dmd_eot=(u_char)Read_Deci(dmd_eot, 1);
			if (errno) continue;

			dma_buffer(1);
			config.dma_regs[9] &=~2;
			if (dmd_eot) config.dma_regs[9] |=2;

			plx_regs->dma[0].dmamode =(dmd_eot) ?
									0x0259C3L :		// IS/EOT.DMD/AC.BU/BT.RY/32
									0x0219C3L;		// IS/DMD/AC.BU/BT.RY/32

			plx_regs->dma[0].dmapadr =dmabuf_phy[1];
			plx_regs->dma[0].dmaladr =0;
			plx_regs->dma[0].dmasize =SZ_PAGE;
			plx_regs->dma[0].dmadpr  =0x08;				// LOCAL2PCI

			plx_regs->dmacsr[0] =0x3;
			continue;
		}

		if ((ux == 5) || (ux == 10)) {
			bus_addr(0, &config.dma_regs[ux]);
		} else {
			printf("Value $%08lX ", config.dma_regs[ux]);
			config.dma_regs[ux]=Read_Hexa(config.dma_regs[ux], -1);
		}

		if (errno) continue;
		switch (ux) {

case 0:
			gigal_regs->d_hdr=config.dma_regs[ux]; break;

case 1:
			gigal_regs->d_am=config.dma_regs[ux]; break;

case 2:
			gigal_regs->d_ad=config.dma_regs[ux]; break;

case 3:
			gigal_regs->d_ad_h=config.dma_regs[ux]; break;

case 4:
			gigal_regs->d_bc=config.dma_regs[ux]; break;

case 10:
			tmp=get_addr(config.dma_regs[ux]);
			gigal_regs->d0_bc_adr=tmp; break;

case 11:
			gigal_regs->d0_bc_len=config.dma_regs[ux]; break;

		}
	}
}
//--- end ------------------- gigal_dma -------------------------------------
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
	u_int		ux;
	char		key;
	int		ret;

	if (gigal_ok) {
		Writeln();
		config.phys_pg=DMA_PAGES;
//		if (pup == 2) config.phys_pg=4;

		ret=alloc_dma_buffer(0);				// allocate
		set_initiator_window(DMA_PAGES);		// set local to PCI window
		if (!dmapages || debug) {
			if (ret > CE_PROMPT) display_errcode(ret);
			WaitKey(0);
		}

		gigal_regs->cr=CR_TRANPARENT <<16;
	}

	if (pup == 2) pciconf.main_menu='5';
	if (gigal_ok && pup && (pciconf.main_menu == '5')) application_dial(pup);

	pup=0;
	while (1) {
		clrscr();
		gotoxy(SP_, 1);
		if (!gigal_ok) {
			printf("no PLX found");
		} else {
			cprintf("%ld count per ms", pciconf.ms_count);
		}

		gotoxy(SP_, 2);
		cprintf("System Memory =%u MByte", (u_int)(sysram >>20));

		if (pci_req) { gotoxy(SP_, 4); int_logging(); }
		else { lowvideo(); cprintf("\r\n"); }

		ux=0;
		gotoxy(SP_, ux+++ZL_);
						printf("Konfigurierungs Menu ............0");
		gotoxy(SP_, ux+++ZL_);
						printf("PCI BIOS Menu ...................1");
		gotoxy(SP_, ux+++ZL_);
						printf("ADD ON Utilities ................2");
		gotoxy(SP_, ux+++ZL_);
						printf("SIS1100 Operation Register ......3");
		gotoxy(SP_, ux+++ZL_);
						printf("SIS1100 DMA Control .............4");
		gotoxy(SP_, ux+++ZL_);
						printf("Application Utilities ...........5");
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
						printf("master reset (PXL) ..............M");
		gotoxy(SP_, ux+++ZL_);
						printf("FPGA reset ......................R");
		gotoxy(SP_, ux+++ZL_);
						printf("special test ....................F");
		gotoxy(SP_, ux+++ZL_);
						printf("Ende ...........................^C");
		gotoxy(SP_, ux+ZL_);
						printf("                                 %c ",
								 pciconf.main_menu);

		key=toupper(getch());
		if (key == CTL_C) {
			if (gigal_ok) {
				gigal_regs->cr=(CR_READY<<16);
				gigal_regs->d0_bc_adr=0;
				gigal_regs->rd_pipe_len=0;
				send_dmd_eot();
			}

			return;
		}

if (!key) printf("%d\n", getch());
		while (1) {
			use_default=(key == TAB);
			if (use_default || (key == CR)) key=pciconf.main_menu;

			clrscr(); gotoxy(1, 2);
			errno=0; ret=CEN_MENU;
			switch (key) {

		case '0':
				Install_Int(0);
				Konfigurierung(FZJ_VENDOR_ID, GIGALINK_DEVICE_ID);
				plx_init();
				break;

		case '1':
				ret=pci_bios_dial();
				break;

		case '2':
				if (pci_ok == 2) ret=addon_dial();
				break;

		case '3':
				if (!gigal_ok) break;

				ret=gigal_op_regs();
				break;

		case '4':
				if (!gigal_ok) break;

				ret=gigal_dma();
				break;

		case '5':
				if (!gigal_ok) break;

				ret=application_dial(0);
				break;

		case '9':
				if (!gigal_ok) break;

				Install_Int(0);
				ret=alloc_dma_buffer(1);
				set_initiator_window(DMA_PAGES);		// set local to PCI window
				break;

		case 'A':
				if (!gigal_ok) break;

				ret=dma_buffer(0);
				break;

		case 'B':
				if (!gigal_ok) break;

				ret=show_dmabuf(-3);
				break;

		case 'F':
				ret=spec_test();
				break;

		case 'I':
				if (!pci_irq) break;

				Install_Int(int_vec == 0);
				break;


		case 'M':
				ret=PLX_reset(); break;

		case 'R':
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
	printf("       /s      standard setup\n");
	printf("       /n      no auto startup\n\n");
	return 1;
}


//----------------------------------------------------------------------------
//										Start
//----------------------------------------------------------------------------
//
static char	savecnf;

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

	if (pci_attach(&pciconf.region, GIGALINK_DEVICE_ID, 0)) return 1;

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
"\n------------ general test program TDC-F1 Readout System ------------\n\n");

	ix=strlen(argv[0]);
	if (ix >= sizeof(root)) {
		printf("can't handle full program name\n");
		return 0;
	}

#if (sizeof(VTEX_RG) != 0x80) || (sizeof(VTEX_BC) != 0x80)
	printf("STRUCT SIZE:\nIN_CARD    : %x\nIN_CARD_BC : %x\n",
			 sizeof(VTEX_RG), sizeof(VTEX_BC));
	printf("BUSY       : %x\nERROR      : %x\n",
			 OFFSET(VTEX_BC,ADR_TRIGGER), OFFSET(VTEX_BC,ADR_ERROR));
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
	pciconf.device_id =GIGALINK_DEVICE_ID;
	pciconf.vendor_id =FZJ_VENDOR_ID;
	pciconf.bmen_device_id=GIGALINK_DEVICE_ID;
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
