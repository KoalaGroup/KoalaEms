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
	int		mem,
	char		*name)
{
	int		ret=0;
	FILE		*datei;
	char		hstr[200];
	long		adr;
	u_int		d,x;

	datei =fopen(name,"ra");
	if (!datei) {
		printf("file not found -> %s\n",name);
		return 1;
	}

	vtex_rg->ADR_RAM_ADR_LV =0;
	vtex_rg->ADR_RAM_ADR_HV =0;
	while (!feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				vtex_rg->ADR_RAM_ADR_LV=adr;
				vtex_rg->ADR_RAM_ADR_HV =adr;
				break;

			default:		// Datenzeile
				// 0123 5678 0123 5678
				strcpy(&hstr[14],&hstr[15]);
				strcpy(&hstr[9],&hstr[10]);
				strcpy(&hstr[4],&hstr[5]);
				d =BIN2DEC(hstr);
				if (mem == LV) vtex_rg->ADR_RAM_DATA_LV=d;
				else vtex_rg->ADR_RAM_DATA_HV=d;

				break;
		}
	}

	fclose(datei);

	datei = fopen(name,"ra");
	if (!datei) return 1;

	vtex_rg->ADR_RAM_ADR_LV =0;
	vtex_rg->ADR_RAM_ADR_HV =0;
	adr=0;
	while ((ret < 10) && !feof(datei)) {
		if (!fgets(hstr,sizeof(hstr),datei)) break;

		switch (hstr[0]) {
			case '-':	// Kommentar
				break;

			case '#':   // Neue Adresse
				sscanf(&hstr[1],"%ld",&adr);
				vtex_rg->ADR_RAM_ADR_LV=adr;
				vtex_rg->ADR_RAM_ADR_HV=adr;
				break;

			default:		// Datenzeile
				// 0123 5678 0123 5678
				strcpy(&hstr[14],&hstr[15]);
				strcpy(&hstr[9],&hstr[10]);
				strcpy(&hstr[4],&hstr[5]);
				d = BIN2DEC(hstr);
				if (mem == LV) {
					if (d != (x=vtex_rg->ADR_RAM_DATA_LV)) {
						printf("LV ERROR => ADR: %06lX, SOLL: %04X, IST: %04X\n",
								 adr, d, x);
						ret++;
					}
				} else {
					if (d != (x=vtex_rg->ADR_RAM_DATA_HV)) {
						printf("HV ERROR => ADR: %06lX, SOLL: %04X, IST: %04X\n",
								 adr, d, x);
						ret++;
					}
				}

				adr++;
				break;
		}
	}

	fclose(datei);
	return ret ? 1 : 0;
}
