// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	input.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <string.h>
#include "math.h"
#include "pci.h"
#include "daq.h"

static u_int BIN2DEC(char *hstr) {
	int 	i;
	u_int	ui;

	ui=0;
	for (i=0; i<16; i++) {
		if (hstr[15-i]=='1') {
			ui += pow(2,i);
		}
	}

	return ui;
}

int LoadSequencer(
	VTEX_RG	*vtex_rg,
	int		hv,
	char		*name)
{
	int		ret=0;
	FILE		*datei;
	char		hstr[200];
	long		adr;
	u_int		d,x;

//printf("LoadSequencer(%08lX, %u, %s\n", vtex_rg, hv, name);
	datei =fopen(name,"ra");
	if (!datei) {
		printf("file not found -> %s\n",name);
		return 1;
	}

	adr=0;
	if (hv) adr=C_HV_SRAM_OFFSET;
	vtex_rg->ADR_SRAM_ADR =adr;
	while (!feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				if (hv) adr +=C_HV_SRAM_OFFSET;
				vtex_rg->ADR_SRAM_ADR=adr;
				break;

			default:		// Datenzeile
				// 0123 5678 0123 5678
				strcpy(&hstr[14],&hstr[15]);
				strcpy(&hstr[9],&hstr[10]);
				strcpy(&hstr[4],&hstr[5]);
				vtex_rg->ADR_SRAM_DATA=BIN2DEC(hstr);

				break;
		}
	}

	fclose(datei);

	datei = fopen(name,"ra");
	if (!datei) return 1;

	adr=0;
	if (hv) adr=C_HV_SRAM_OFFSET;
	vtex_rg->ADR_SRAM_ADR =adr;
	while ((ret < 10) && !feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				if (hv) adr +=C_HV_SRAM_OFFSET;
				vtex_rg->ADR_SRAM_ADR=adr;
				break;

			default:		// Datenzeile
				// 0123 5678 0123 5678
				strcpy(&hstr[14],&hstr[15]);
				strcpy(&hstr[9],&hstr[10]);
				strcpy(&hstr[4],&hstr[5]);
				d = BIN2DEC(hstr);
				if (d != (x=vtex_rg->ADR_SRAM_DATA)) {
					printf("ERROR => ADR: %06lX, SOLL: %04X, IST: %04X\n",
							 adr, d, x);
					ret++;
				}

				adr++;
				break;
		}
	}

	fclose(datei);
	return ret ? 1 : 0;
}
//
//--------------------------- LoadTram --------------------------------------
//
int LoadTram(
	VTEX_RG	*vtex_rg,
	char		*name,
	char		hv)
{
	int		ret=0;
	FILE		*datei;
	char		hstr[200];
	long		adr;
	u_int		d,x;
	u_long	cnt=0;

//printf("LoadTram(%08lX, %s\n", vtex_rg, name);
	datei =fopen(name,"ra");
	if (!datei) {
		printf("file not found -> %s\n",name);
		return CEN_PROMPT;
	}

	adr=0;
	if (hv) adr=C_HV_TRAM_OFFSET;
	vtex_rg->ADR_RAMT_ADR =adr;
	while (!feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				if (hv) adr +=C_HV_TRAM_OFFSET;
				vtex_rg->ADR_RAMT_ADR=adr;
				break;

			default:		// Datenzeile
				sscanf(hstr, "%4x", &d);
				vtex_rg->ADR_RAMT_DATA=d;
				cnt++;
				break;
		}
	}

	fclose(datei);

	datei = fopen(name,"ra");
	if (!datei) return CEN_PROMPT;

	adr=0;
	if (hv) adr=C_HV_TRAM_OFFSET;
	vtex_rg->ADR_RAMT_ADR =adr;
	while ((ret < 10) && !feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				if (hv) adr +=C_HV_TRAM_OFFSET;
				vtex_rg->ADR_RAMT_ADR=adr;
				break;

			default:		// Datenzeile
				sscanf(hstr, "%4x", &d);
				if (d != (x=vtex_rg->ADR_RAMT_DATA)) {
					printf("ERROR => ADR: %06lX, SOLL: %04X, IST: %04X\n",
							 adr, d, x);
					ret++;
				}

				adr++;
				break;
		}
	}

	fclose(datei);
	printf("%04lX TRAM words loaded\n", cnt);
	return ret ? CEN_PROMPT : 0;
}
//
#ifdef SC_M
//--------------------------- sram_copyout ----------------------------------
//
int sram_copyout(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr)
{
	u_long	ud,uw;
	u_long	lb,ll;
	u_int		i;

	if (!(mode &1)) {
		if (len > 0x100000L) {
			printf("max length is 0x100000 (1 Mb)\n");
			return CEN_PROMPT;
		}
	}

	gigal_regs->d0_bc_adr =0;
	gigal_regs->d0_bc_len =0;
	if (mode &2) vtex_rg->ADR_RAMT_ADR=sram_addr;
	else vtex_rg->ADR_SRAM_ADR=sram_addr;

	gigal_regs->d_hdr=HW_R_BUS|HW_FIFO|HW_BT|HW_WR;
	gigal_regs->d_ad =0;
	uw=0; lb=0x100000L;
	while (1) {
		if (lb > len) lb=len;

		if (mode &1)
			for (ud=0, ll=lb>>2; ud < ll; ud++, uw++) DBUF_L(ud)=WBUF_L(uw);

		setup_dma(1, 0, 0, lb, (mode &2) ? (u_int)&vtex_rg->ADR_RAMT_DATA
													: (u_int)&vtex_rg->ADR_SRAM_DATA, 0, 0);	// for DMA write
		gigal_regs->d_bc =lb;
		if (lb > SZ_PAGE) plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[1]=0x3;								// start DMA
		i=0;
		while (1) {
			_delay_(50000L);
			if (plx_regs->dmacsr[1] &0x10) break;

			if (++i >= 1000) {
				printf("DMA1 wr time out CSR=%02X\n", plx_regs->dmacsr[1]);
				return CE_PROMPT;
			}
		}

		if (protocol_error(-1, 0)) return CEN_PROMPT;

		if (lb == len) return 0;

		len-=lb;
	}
}
//
//--------------------------- sram_copyin -----------------------------------
//
int sram_copyin(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr)
{
	u_long	ud,uw;
	u_long	lb,ll;
	u_int		i;

	if (!(mode &1)) {
		if (len > 0x100000L) {
			printf("max length is 0x100000 (1 Mb)\n");
			return CEN_PROMPT;
		}
	}

	gigal_regs->d0_bc_adr =0;
	gigal_regs->d0_bc_len =0;
	if (mode &2) vtex_rg->ADR_RAMT_ADR=sram_addr;
	else vtex_rg->ADR_SRAM_ADR=sram_addr;

	gigal_regs->t_hdr=HW_BE|HW_L_DMD|HW_R_BUS|HW_EOT|HW_FIFO|HW_BT|THW_SO_DA;
	gigal_regs->t_ad =(mode &2) ? (u_int)&vtex_rg->ADR_RAMT_DATA
										 : (u_int)&vtex_rg->ADR_SRAM_DATA;
	uw=0; lb=0x100000L;
	while (1) {
		if (lb > len) lb=len;

		setup_dma(-1, 0, 0, lb, 0, 0, DMA_L2PCI);		// for DMA read
		gigal_regs->d0_bc=0;

		if (lb > SZ_PAGE) plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[0]=0x3;// start DMA
		gigal_regs->t_da =lb;	// start transfer
		i=0;
		while (1) {
			_delay_(50000L);
			if ((plx_regs->dmacsr[0] &0x10)) break;

			if (++i >= 1000) {
				printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
				protocol_error(-1, "1");
				printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
				return CEN_PROMPT;
			}
		}

		if (protocol_error(-1, 0)) return CE_PROMPT;

		if (gigal_regs->d0_bc != lb) {
			printf("illegal len %05lX exp: %08lX\n", gigal_regs->d0_bc, lb);
			return CE_PROMPT;
		}

		if (mode &1)
			for (ud=0, ll=lb>>2; ud < ll; ud++, uw++) WBUF_L(ud)=DBUF_L(uw);

		if (lb == len) return 0;

		len-=lb;
	}
}
//
#else
//--------------------------- sram_copyout ----------------------------------
//
int sram_copyout(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr)
{
	u_long	ud,uw;
	u_long	lb,ll;
	u_int		i;

	if (!(mode &1)) {
		if (len > (u_long)dmapages*SZ_PAGE) {
			printf("max length is %05lX\n", (u_long)dmapages*SZ_PAGE);
			return CEN_PROMPT;
		}
	}

	if (mode &2) vtex_rg->ADR_RAMT_ADR=sram_addr;
	else vtex_rg->ADR_SRAM_ADR=sram_addr;

	uw=0; lb=(u_long)dmapages*SZ_PAGE;
	while (1) {
		if (lb > len) lb=len;

		if (mode &1)
			for (ud=0, ll=lb>>2; ud < ll; ud++, uw++) DBUF_L(ud)=WBUF_L(uw);

		setup_dma(1, 0, 0, lb, (mode &2) ? ADDRSPACE1+(u_int)&vtex_rg->ADR_RAMT_DATA
													: ADDRSPACE1+(u_int)&vtex_rg->ADR_SRAM_DATA, 0, 0);	// for DMA write
		if (lb > SZ_PAGE) plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[1]=0x3;								// start DMA
		i=0;
		while (1) {
			_delay_(50000L);
			if (plx_regs->dmacsr[1] &0x10) break;

			if (++i >= 1000) {
				printf("DMA1 wr time out CSR=%02X\n", plx_regs->dmacsr[1]);
				return CE_PROMPT;
			}
		}

		if (bus_error()) return CEN_PROMPT;

		if (lb == len) return 0;

		len-=lb;
	}
}
//
//--------------------------- sram_copyin -----------------------------------
//
int sram_copyin(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr)
{
	u_long	ud,uw;
	u_long	lb,ll;
	u_int		i;

	if (!(mode &1)) {
		if (len > (u_long)dmapages*SZ_PAGE) {
			printf("max length is %05lX\n", (u_long)dmapages*SZ_PAGE);
			return CEN_PROMPT;
		}
	}

	if (mode &2) vtex_rg->ADR_RAMT_ADR=sram_addr;
	else vtex_rg->ADR_SRAM_ADR=sram_addr;

	uw=0; lb=(u_long)dmapages*SZ_PAGE;
	while (1) {
		if (lb > len) lb=len;

		setup_dma(-1, 0, 0, lb, 0, 0, DMA_FASTEOT|DMA_L2PCI);							// for SRAM read

		if (lb > SZ_PAGE) plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[0]=0x3;// start DMA
		lvd_regs->lvd_scan =0;
		lvd_regs->lvd_blk_max =lb;
		lvd_regs->lvd_block=(mode &2) ? (inp_conf->slv_addr <<8) +OFFSET(VTEX_RG, ADR_RAMT_DATA)/2
												: (inp_conf->slv_addr <<8) +OFFSET(VTEX_RG, ADR_SRAM_DATA)/2;
		i=0;
		while (1) {
			_delay_(50000L);
			if ((plx_regs->dmacsr[0] &0x10)) break;

			if (++i >= 1000) {
				printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
				bus_error();
				printf("len: %04lX\n", lvd_regs->lvd_blk_cnt);
				return CEN_PROMPT;
			}
		}

		if (bus_error()) return CE_PROMPT;

		lvd_regs->sr=lvd_regs->sr;
		if (lvd_regs->lvd_blk_cnt != lb) {
			printf("illegal len %05lX exp: %08lX\n", lvd_regs->lvd_blk_cnt, lb);
			return CE_PROMPT;
		}

		if (mode &1)
			for (ud=0, ll=lb>>2; ud < ll; ud++, uw++) WBUF_L(ud)=DBUF_L(uw);

		if (lb == len) return 0;

		len-=lb;
	}
}
//
#endif
