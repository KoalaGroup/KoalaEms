// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	tof.c -- TOF Input Testprogramm with SIS1100 PCI module

#include <stdio.h>
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
#include "f1.h"

char			gigal_ok;
CONFIG		config;		// will be imported at pci.c, pciconf.c and pcibios.c
PLX_REGS		*plx_regs;
GIGAL_REGS	*gigal_regs;	// local register
CM_REGS		*cm_reg;			// CMASTER register
BUS1100		*bus1100;		// SIS1100 remote bus access

static char		root[100];
static char		cnf_name[110];
static char		rootcnf,savecnf;
static char		ProgName[20];

static u_long	deadlocks;
static u_int	timeouts;

static u_long	g_perr;
//
//
// Achtung: Speicher Eigenschaften
//				XMS   1 MB
//				DPMI  2 MB (minimum)
//
u_long	sysram;

#define L_DMABUF		((u_long)DMA_PAGES <<12)	// length of DMA Buffer bytes
u_long	*dmabuf=0;							// DMA buffer
u_int		dmapages=0;

u_long	dmabuf_phy[DMA_PAGES] ={0};	// physical addresses
u_int		phypages=0;							// number of phy pages

u_long	*dmadesc=0;							// DMA descriptor buffer
u_long	dmadesc_phy=0;
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
		if (WaitKey(0)) ret=-1;

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

	return -1;
}
//
//===========================================================================
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
			return -1;
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
	u_int		ux;

	if (phypages == 0) return -1;

	ux=0;
	do dmadesc[ux++]=0xCCCCCCCCL; while (ux < SZ_PAGE_L);

	ux=0;
	do dmabuf[ux++]=0xCCCCCCCCL; while (ux < SZ_PAGE_L*dmapages);

	if (mode != 0) return 0;

	printf("pg X: phy %08lX\n", dmadesc_phy);
	ux=0;
	do printf("pg %u: phy %08lX\n", ux, dmabuf_phy[ux]);
	while (++ux < phypages);

	return -1;
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
	plx_regs->bigend =0x00;						// no big endian
	plx_regs->dma[1].dmamode =0x0209C3L;	// IS||AC.BU|BT.RY|32
	plx_regs->dma[1].dmaladr =ADDRSPACE0+OFFSET(GIGAL_REGS, d_ad_h);
	plx_regs->dma[1].dmasize =1*4;
	plx_regs->dma[1].dmadpr  =0x0;			// read sysmem
	plx_regs->dmacsr[1] |=0x1;
	ux=0;
	do dmabuf[ux<<10]=TEST_PAT+ux;
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
		pat=gigal_regs->d_ad_h;
		if ((pat &0xFFFFFFF0L) == TEST_PAT) {
			ux=0;
			do {
				if ((pat &0xF) != ux) continue;

				dmabuf[ux<<10]=~TEST_PAT;
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

				if (gigal_regs->d_ad_h == ~TEST_PAT) {
					printf("pg %u: phy %08lX\n", ux, lin);

					dmabuf_phy[ux]=lin; pg++;
					break;
				}

				dmabuf[ux<<10]=TEST_PAT+ux;

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

				if (gigal_regs->d_ad_h == ~(TEST_PAT+0x10)) {
					printf("pg X: phy %08lX\n", lin);
					dmadesc_phy=lin;
				}

				*dmadesc =TEST_PAT+0x10;
			}
		}

		if (dmadesc_phy && (pg >= dmapages)) {
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


//--------------------------- alloc_dma_buffer

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
		printf("unknown size of System Memory\n"); return -1;
	}

	if (mode == 1) {
		printf("# of phys pg %2u ", config.phys_pg);
		config.phys_pg=(u_int)Read_Deci(config.phys_pg, 16);
		if (errno) return 0;

		if (config.phys_pg == 0) return -1;

		if (dmabuf) {
			printf("restart the program\n");
			return -1;
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
			return -1;
		}

		lin=((u_long)regs.bx <<16) |regs.cx;
		if (debug) printf("lin addr=  %08lX\n", lin);

		if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
			printf("can't allocate selector\n");
			return -1;
		}

		SetSelectorLimit(sel, SZ_PAGE-1);
		SetSelectorBase(sel, lin);
		dmadesc=MAKELP(sel, 0);
	}

	if (!dmabuf) {
		if (config.phys_pg == 0) return -1;

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

			if (!--dmapages) {				// versuche es mit weniger
				printf("can't allocate DMA buffer\n");
				return -1;
			}
		}

		if (dmapages != config.phys_pg) {
			printf("only %u DMA pages allocated\n", dmapages);
			WaitKey(0);
		}

		lin=((u_long)regs.bx <<16) |regs.cx;
		if (debug) printf("lin addr=  %08lX\n", lin);

		if ((sel=AllocSelector(SELECTOROF(&tmp))) == 0) {
			printf("can't allocate selector\n");
			return -1;
		}

		SetSelectorLimit(sel, len-1);
		SetSelectorBase(sel, lin);
		dmabuf=MAKELP(sel, 0);
	}

	if (!gigal_ok) return -1;
	//
	//--- search for physical address
	//
	_search_phy();
	if (phypages == 0) {
		printf("no or not all phys pages found\n");
		return -1;
	}

	dma_buffer(1);
	return 0;
}
//
//--- end ------------------- alloc_dma_buffer ------------------------------
//
//--------------------------- set_initiator_window ---------------------------
//
void set_initiator_window(
	u_int	pg)	// number of pages to be fit in
					// 0: disable local to PCI access and initialize DMA buffer
{
	u_int		ux;
	u_long	msk;

	if ((phypages == 0) || (pg == 0)) {
		plx_regs->dmpbam=0;	// disable local to PCI access
		dma_buffer(1);
		return;
	}

   msk=0xFFFF0000L;
	if (pg > phypages) pg =phypages;

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
//--------------------------- plx_init ---------------------------------------
//
static void plx_init(void)
{

	gigal_ok=0;
	if (pci_ok != 2) return;

	if (debug) {
		printf("int line = %u\n", pciconf.region.int_line);
		WaitKey(0);
	}

	plx_regs=(PLX_REGS*)pciconf.region.r0_regs;
	if ((gigal_regs=pci_ptr(&pciconf.region, 2, 0)) == 0) return;

	if ((cm_reg=pci_ptr(&pciconf.region, 2, 0x800)) == 0) return;

	plx_regs->intcsr =PLX_ILOC_ENA;			// enable LINT Interrupt
	gigal_regs->bus_descr[0].hdr=HW_R_BUS;
	gigal_regs->bus_descr[0].ad =0;
	if ((bus1100=pci_ptr(&pciconf.region, 3, 0)) == 0) return;

	gigal_ok=1;
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
	return 1;
}
//
//----------------------------------------------------------------------------
//										Start
//----------------------------------------------------------------------------
//
int Start(int argc, char *argv[])
{
	int	ix,iy,iz;
	char	wrk[80];

	sysram=mem_size();
	debug=0; savecnf=0;
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

	case 'D':	debug=1; break;

	case 'I':	iz++;
					if ((wrk[iz] < '0') || (wrk[iz] > '9')) return Usage();

					pciconf.c_index=wrk[iz]-'0';
					break;

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
	int	ix, iy;
	int	len;

   textmode(C4350);      
	printf(
"\n------------ general test program TDC-F1 Readout System ------------\n\n");

	ix=strlen(argv[0]);
	if (ix >= sizeof(root)) {
		printf("can't handle full program name\n");
		return 0;
	}

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
	rootcnf=0;
	strcpy(cnf_name, ProgName);	// first try current directory
	strcat(cnf_name, ".cnf");
	if ((cnf=_rtl_open(cnf_name, O_RDONLY)) == -1) {
		rootcnf=1;
		strcpy(cnf_name, root);
		strcat(cnf_name, ProgName);
		strcat(cnf_name, ".cnf");
		cnf=_rtl_open(cnf_name, O_RDONLY);
	}

	if (cnf != -1) {
		if ((len=read(cnf, &config, sizeof(config))) != sizeof(config)) {
			if (len < 0) perror("error reading config file");
			else printf("illegal config length\n");

         WaitKey(ESC);
		}

		close(cnf);
	}

	if (Start(argc, argv)) return 0;

	if (!gigal_ok) {
		printf("can't map SIS1100\n");
		return 0;
	}

	Writeln();
	config.phys_pg=16;
	if (alloc_dma_buffer(0)) return 0;

	set_initiator_window(DMA_PAGES);		// set local to PCI window
	if (debug) {
		WaitKey(0);
	}

	gigal_regs->cr=CR_TRANPARENT <<16;
	application();

	gigal_regs->cr=(CR_READY<<16);
	gigal_regs->d0_bc_adr=0;
	gigal_regs->rd_pipe_len=0;
	send_dmd_eot();
	Writeln();
	if (savecnf && rootcnf) {
		strcpy(cnf_name, ProgName);	// write to current directory
		strcat(cnf_name, ".cnf");
	}

	if ((cnf=_rtl_creat(cnf_name, 0)) != -1) {
		if (write(cnf, &config, sizeof(config)) != sizeof(config)) {
			perror("error writing config file");
			printf("name =%s", cnf_name);
		}

		close(cnf);
	} else {
		perror("error creating config file");
		printf("name =%s", cnf_name);
	}

	return 0;
}
