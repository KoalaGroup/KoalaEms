// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	input.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>
#include <time.h>
#include <windows.h>
#include <fcntl.h>
#include "pci.h"
#include "math.h"
#include "daq.h"

#define	MEN_SEQ				0
#define	MEN_ADC				1
#define	MEN_SLOWCTRL		2

#define	MEN_MEM				4
#define	MEN_MAIN				5

#define	CLEARERROR			8

extern VTEX_BC		*inp_bc;
extern VTEX_RG		*inp_unit;
//
//===========================================================================
//										vertex_menu                                   =
//===========================================================================
//
//--------------------------- jt_inp_getdata --------------------------------
//
static u_long jt_inp_getdata(void)
{
	return inp_unit->ADR_JTAG_DATA;
}
//
//--------------------------- jt_inp_getcsr ---------------------------------
//
static u_int jt_inp_getcsr(void)
{
	return inp_unit->ADR_JTAG_CSR;
}

unsigned int	BIN2DEC(char *hstr) {
	int 				i;
	unsigned int	ui;

	ui=0;
	for (i=0; i<16; i++) {
		if (hstr[15-i]=='1') {
			ui += pow(2,i);
		}
	}

	return ui;
}

void	LoadBitFileToBuffer( char *name, int *anz, u_short *buffer) {
	FILE				*datei;
	char				hstr[200];
	unsigned int	d=0;

	*anz=0;
	datei = fopen(name,"ra");
	if (datei) {
		while (!feof(datei)) {
			if (!fgets(hstr,sizeof(hstr),datei)) break;
			switch (hstr[0]) {
				case '-':	// Kommentar
					break;
				default:		// Datenzeile
					// 0123 5678 0123 5678
					strcpy(&hstr[14],&hstr[15]);
					strcpy(&hstr[9],&hstr[10]);
					strcpy(&hstr[4],&hstr[5]);
					d = BIN2DEC(hstr);
					buffer[(*anz)++]=d;
					break;
			}
		}
		fclose(datei);
	}
}


void	LoadFileToMem( int mem, char *name) {
	FILE				*datei;
	char				hstr[200];
	long				adr=0l;
	unsigned int	d=0;


	datei = fopen(name,"ra");
	if (datei) {
		while (!feof(datei)) {
			if (!fgets(hstr,sizeof(hstr),datei)) break;
			switch (hstr[0]) {
				case '-':	// Kommentar
					break;
				case '#':   // Neue Adresse
					sscanf(&hstr[1],"%ld",&adr);
					printf("Aktuelle Adresse : %ld\n",adr);
					if (mem==LV) {
						inp_unit->ADR_SRAM_ADR=adr;
					} else {
						inp_unit->ADR_SRAM_ADR=0x80000l | adr;
					}
					break;
				default:		// Datenzeile
					// 0123 5678 0123 5678
					strcpy(&hstr[14],&hstr[15]);
					strcpy(&hstr[9],&hstr[10]);
					strcpy(&hstr[4],&hstr[5]);
					d = BIN2DEC(hstr);
					inp_unit->ADR_SRAM_DATA=d;
/*
					if (mem==LV) {
						inp_unit->ADR_RAM_ADR_LV=adr;
						inp_unit->ADR_SRAM_DATA=d;
					} else {
						inp_unit->ADR_RAM_ADR_HV=adr;
						inp_unit->ADR_SRAM_DATA=d;
					}
*/
//					adr++;

					break;
			}
		}
		fclose(datei);


		printf("\nverifying...\n");
		datei = fopen(name,"ra");
		if (datei) {
			while (!feof(datei)) {
				if (!fgets(hstr,sizeof(hstr),datei)) break;
				switch (hstr[0]) {
					case '-':	// Kommentar
						break;
					case '#':   // Neue Adresse
						sscanf(&hstr[1],"%ld",&adr);
						printf("Aktuelle Adresse : %ld\n",adr);
						if (mem==LV) {
							inp_unit->ADR_SRAM_ADR=adr;
						} else {
							inp_unit->ADR_SRAM_ADR=0x80000l|adr;
						}
						break;
					default:		// Datenzeile
						// 0123 5678 0123 5678
						strcpy(&hstr[14],&hstr[15]);
						strcpy(&hstr[9],&hstr[10]);
						strcpy(&hstr[4],&hstr[5]);
						d = BIN2DEC(hstr);
						if (d!=inp_unit->ADR_SRAM_DATA) {
							printf("ERROR => ADR: %ld, SOLL: %d, IST: %d",adr,d,inp_unit->ADR_SRAM_DATA);
						}
/*
						if (mem==LV) {
//							inp_unit->ADR_RAM_ADR_LV=adr;
							if (d!=inp_unit->ADR_RAM_DATA_LV) {
								printf("ERROR => ADR: %ld, SOLL: %d, IST: %d",adr,d,inp_unit->ADR_RAM_DATA_LV);
							}
						} else {
//							inp_unit->ADR_RAM_ADR_HV=adr;
							if (d!=inp_unit->ADR_SRAM_DATA) {
								printf("ERROR => ADR: %ld, SOLL: %d, IST: %d",adr,d,inp_unit->ADR_RAM_DATA_HV);
							}
						}
*/
//						adr++;

						break;
				}
			}
			fclose(datei);
		}

	} else {
		printf("file not found -> %s\n",name);
	}

	printf("\n");
	printf("\n");
}



int WriteSRAM(int mode) {	// 0: ok
	long				l;

	randomize();
	switch (mode) {
		case 0 :	for (l=0; l<0x100000l; l++ ) {
						WBUF_S(l)=(unsigned int)(((unsigned int)random(256)<<8)|random(256));
					}
					break;

		case 1 :	for (l=0; l<0x80000l; l++ ) {
						WBUF_S(l)=(unsigned int)l;
					}
					for (l=0x80000l; l<0x100000l; l++ ) {
						WBUF_S(l)=(unsigned int)(0x80000l-l);
					}
					break;
	}

	inp_unit->ADR_CTRL     = MODE_SLOW;
	printf("write\n");
	return sram_copyout(inp_unit, 0x1, 0x200000L, 0);
}



void ReadSRAM(void) {
	u_long	i;
	u_long	l,e;
	char	c;
	int	err;

	printf("read\n");
	inp_unit->ADR_CTRL 	 	= MODE_SLOW;

	e=0; err=0; l=0;
	do {
		if (sram_copyin(inp_unit, 0x0, 0x100000L, l)) return;

		for (i=0; i < 0x080000l; i++, l++) {
			if (DBUF_S(i) != WBUF_S(l)) {
				gotoxy(1,7+err);
				e++;
				if (err<40) err++; else err = 0;
				printf("Reading - ADR: %06ld, DATA: %04X, ERR: %ld, Soll: %04X, Diff: %04X\n",
						 l,DBUF_S(i),e,WBUF_S(l),(DBUF_S(i) ^ WBUF_S(l)));
				gotoxy(1,5);
				if (kbhit()){
					c=toupper(getch());
					if ((c == CTL_C) || (c == ESC)) return;
				}
			}
		}

	} while (l < 0x100000L);
}









int WriteTRAM(int mode) {
	long l;

	inp_unit->ADR_CTRL = MODE_SLOW;

	randomize();
	randomize();
	switch (mode) {
		case 0 :	for (l=0; l<0x200000l; l++ ) {
						WBUF_S(l)=(unsigned int)(((unsigned int)random(256)<<8)|random(256));
					}
					break;

		case 1 :	/*
					for (l=0; l<0x100000l; l++ ) {
						WBUF_S(l)=(unsigned int)l;
					}
					for (l=0x100000l; l<0x200000l; l++ ) {
						WBUF_S(l)=(unsigned int)(0x100000l-l);
					}
					*/
					for (l=0; l<0x200000l; l++ ) {
						WBUF_S(l)=(unsigned int)(l);;
					}
					break;
	}

	printf("write\n");
	return sram_copyout(inp_unit, 0x3, 0x400000L, 0);
}


void ReadTRAM(void)  {
	u_long	i,l;
	unsigned int e=0;
	char	c;
	u_short	first=0;

	inp_unit->ADR_CTRL 			= MODE_SLOW;
	printf("read\n");
	l=0;
	do {
		if (sram_copyin(inp_unit, 0x2, 0x100000L, l)) return;

		for (i=0; i < 0x080000l; i++, l++) {
			if (DBUF_S(i) != WBUF_S(l)) {
				if (first==0)
					{ first = 1;
					  gotoxy(1,14+e);
					  printf("ERROR von %06lX bis             ",l);
					  gotoxy(1,13);
					}
//			printf("Reading - ADR: %06ld, DATA: %04X, ERR: %ld, Soll: %04X, Diff: %04X\n",
//					 l,DBUF_S(i),e,WBUF_S(l),(DBUF_S(i) ^ WBUF_S(l)));
//			printf("\rReading - ADR: %06ld, DATA: %04X, ERR: %ld, Soll: %04X, Diff: %04X\r",
//					 l,DBUF_S(i),e,WBUF_S(l),(DBUF_S(i) ^ WBUF_S(l)));
//			gotoxy(1,5);
				if (kbhit()){
					c=toupper(getch());
					if ((c == CTL_C) || (c == ESC)) break;
				}
			} else
				if (first==1) {
					gotoxy(22,14+e);
					printf("%06lX",l);
					if (e<20) e++; else e = 0;
					first = 0;
					gotoxy(1,13);
				}
		}
	} while (l < 0x200000L);
}




//
//--------------------------- jt_inp_putcsr ---------------------------------
//
static void jt_inp_putcsr(u_int val)
{
	inp_unit->ADR_JTAG_CSR = val;
	inp_unit->ADR_IDENT    = 0;			// nur delay
}






/*

int DumpLVRAM(void) {
	long  l;
	char	key;
	gotoxy(1,5);
	inp_unit->ADR_CTRL = MODE_SLOW;

	inp_unit->ADR_RAM_ADR_LV=0;

	clrscr();
	printf("\nLV RAM Dump...\n");

	for (l=0; l<524288l; l++) {
		if (((l%(8*24))==0)&&(l>0)) {
			while (!kbhit());
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return 0;
		}
		if ((l%8)==0) 	printf("\n%05X : ",l);
		printf("%04X ",inp_unit->ADR_RAM_DATA_LV);
	}
	return 1;
}


int DumpHVRAM(void) {
	long  l;
	char	key;
	gotoxy(1,5);
	inp_unit->ADR_CTRL = MODE_SLOW;

	inp_unit->ADR_RAM_ADR_HV=0;

	clrscr();
	printf("\nLV RAM Dump...\n");

	for (l=0; l<524288l; l++) {
		if (((l%(8*24))==0)&&(l>0)) {
			while (!kbhit());
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return 0;
		}
		if ((l%8)==0) 	printf("\n%05X : ",l);
		printf("%04X ",inp_unit->ADR_RAM_DATA_HV);
	}
	return 1;
}

*/

static int mem_menu(void)
{
	char		key;
	char		hd;

	inp_unit->ADR_CTRL = MODE_SLOW;

	hd=1;
	while (1) {
		clrscr();
		while (kbhit()) getch();

		errno=0;
		if (hd) {
			Writeln();
//			inp_assign(sc_conf, 1);
			printf("Memory Test");
			printf("\n");
			printf("\n");
			printf("Sequencer RAM (random)     ..1\n");
			printf("Sequencer RAM (rampe)      ..2\n");
			printf("Tabellen RAM (random)      ..3\n");
			printf("Tabellen RAM (rampe)       ..4\n");
			printf("\n");
			printf("                             %c ", UCLastKey[MEN_MEM]);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=UCLastKey[MEN_MEM];
		}

		use_default = (key == TAB);
		if (use_default || (key == CR)) key=UCLastKey[MEN_MEM];
		if (key > ' ') printf("%c\n", key);

		if (!inp_unit) continue;


//		ux=key-'0';

		switch (key) {
			case '1':	printf("\nSequencer RAM random...\n");
							if (!WriteSRAM(0)) ReadSRAM();
							printf("Press key...");
							toupper(getch());
							break;

			case '2':	printf("\nSequencer RAM rampe...\n");
							if (!WriteSRAM(1)) ReadSRAM();
							printf("Press key...");
							toupper(getch());
							break;

			case '3': 	printf("\nTabellen RAM random ...\n");
							if (!WriteTRAM(0)) ReadTRAM();
							printf("Press key...");
							toupper(getch());
							break;

			case '4': 	printf("\nTabellen RAM rampe ...\n");
							if (!WriteTRAM(1)) ReadTRAM();
							printf("Press key...");
							toupper(getch());
							break;

		}

		UCLastKey[MEN_MEM]=key;
	}
}


static int sequencer_menu(void)
{
//	u_int		ux;
	char		key;
	char		hd;
	long		l;


	inp_unit->ADR_CTRL = MODE_SLOW;

	hd=1;
	while (1) {
		clrscr();
		while (kbhit()) getch();

		errno=0;
		if (hd) {
			Writeln();
			printf("Sequencer Test\n");
			printf("\n");
			printf("Sequencer Status (LV & HV)            %04X  ..S\n",inp_unit->ADR_SEQSTATE);
			printf("Load Sequencer (LV & HV )                   ..L\n");
			printf("Reset Sequencer (LV & HV )                  ..R\n");

			printf("\n");
			printf("LV Sequencer                                   \n");
			printf("  Output Polarity                      %03X  ..0\n",inp_unit->ADR_SEQPOL_LV);
			printf("  Chip Select Polarity                 %03X  ..1\n",inp_unit->ADR_CSPOL_LV);
			printf("  ADV Offset Clocks                      %1X  ..2\n",inp_unit->ADR_ADCOFFSET_LV);
			printf("  Chip Mask                            %03X  ..3\n",inp_unit->ADR_CHIPMASK_LV);
			printf("  MaxData                             %04X  ..4\n",inp_unit->ADR_MAX_DATA_LV);
			printf("  Trigger LV Sequencer (single)             ..5\n");
			printf("  Trigger LV Sequencer (cont.)              ..6\n");
			printf("  Trigger & Read LV Sequencer (single)      ..7\n");
			printf("  Trigger & Read LV Sequencer (cont.)       ..8\n");
			printf("  Show Counter                              ..9\n");
			printf("  Clear all Counter                         ..A\n");
			printf("\n");
			printf("HV Sequencer                                   \n");
			printf("  Output Polarity                      %03X  ..D\n",inp_unit->ADR_SEQPOL_HV);
			printf("  Chip Select Polarity                 %03X  ..E\n",inp_unit->ADR_CSPOL_HV);
			printf("  ADV Offset Clocks                      %1X  ..F\n",inp_unit->ADR_ADCOFFSET_HV);
			printf("  Chip Mask                            %03X  ..G\n",inp_unit->ADR_CHIPMASK_HV);
			printf("  MaxData                             %04X  ..H\n",inp_unit->ADR_MAX_DATA_HV);
			printf("  Trigger HV Sequencer (single)             ..I\n");
			printf("  Trigger HV Sequencer (cont.)              ..J\n");
			printf("  Trigger & Read HV Sequencer (single)      ..K\n");
			printf("  Trigger & Read HV Sequencer (cont.)       ..M\n");
			printf("  Show Counter                              ..N\n");
			printf("  Reset all Counter                         ..O\n");
			printf("\n");
			printf("LV & HV Sequencer                              \n");
			printf("  Trigger LV & HV Sequencer (single)        ..Q\n");
			printf("  Trigger LV & HV Sequencer (cont.)         ..T\n");
			printf("  Trigger & Read LV & HV Sequencer (single) ..U\n");
			printf("  Trigger & Read LV & HV Sequencer (cont.)  ..V\n");
			printf("  Show Counter                              ..W\n");
			printf("  Reset all Counter                         ..X\n");
			printf("\n");
			printf("                                              %c ", UCLastKey[MEN_SEQ]);

			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=UCLastKey[MEN_SEQ];
		}

		use_default = (key == TAB);
		if (use_default || (key == CR)) key=UCLastKey[MEN_SEQ];
		if (key > ' ') printf("%c\n", key);

		if (!inp_unit) continue;


//		ux=key-'0';

		switch (key) {


			case 'S':	break;

			case 'L':
							inp_unit->ADR_CTRL = MODE_SLOW;
							printf("\nClearing LV Memory...\n\n");
							inp_unit->ADR_SRAM_ADR=0;
							for (l=0; l<0x80000l; l++) {
								inp_unit->ADR_SRAM_DATA=0;
							}
							printf("\nloading LV RAM...\n");
							LoadFileToMem(LV,"C:\\LVDATA.TXT");
							printf("\nLV RAM loaded...\n");

							inp_unit->ADR_CTRL = 0x0000;
							printf("\nClearing HV Memory...\n");
							inp_unit->ADR_SRAM_ADR=0x80000l;
							for (l=0; l<0x80000l; l++) {
								inp_unit->ADR_SRAM_DATA=0;
							}
							printf("\nloading HV RAM...\n");
							LoadFileToMem(HV,"C:\\HVDATA.TXT");
							printf("\nHV RAM loaded...\n");

							while (!kbhit());	getch();
							break;

			case 'R':
							inp_unit->ADR_ACTION = R_RESETSEQ;
							printf("\nSequencer resetted...\n");
							delay(1000);
//							inp_unit->ADR_RAM_ADR_LV=0;
							break;









			/************ HV ************/

			case '0':	printf("Output Polarity (LV) <000/7FF> -> ");
							inp_unit->ADR_SEQPOL_LV = (u_short)Read_Hexa(inp_unit->ADR_SEQPOL_LV,0x7FF);
							break;
			case '1':	printf("Chip Select Polarity (LV) <000/3FF> -> ");
							inp_unit->ADR_CSPOL_LV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_LV,0x3FF);
							break;

			case '2':	printf("ADC Offset Clocks (LV) <0/F> -> ");
							inp_unit->ADR_ADCOFFSET_LV = (u_short)Read_Hexa(inp_unit->ADR_ADCOFFSET_LV,0xF);
							break;

			case '3':	printf("Chip Mask (LV) <000/3FF> -> ");
							inp_unit->ADR_CSPOL_LV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_LV,0x3FF);
							break;

			case '4':	printf("Max Data (LV) <0000/FFFF> -> ");
							inp_unit->ADR_CSPOL_LV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_LV,0xFFFF);
							break;

			case '5':   // LV single
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_LV_ON;
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_LV_ON | CTRL_LV_TRIGGER;
							delay(1);
							inp_unit->ADR_CTRL = MODE_SLOW;
//							delay(1);
//							inp_unit->ADR_CTRL = 0x002C;
							break;

			case '6': 	// LV cont
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON;
							l=0;
							printf("\n\ntriggering LV Sequencer");

							while(1) {
								if (l%1000) printf(".");
								l++;
								delay(10);
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_LV_TRIGGER;
								if (kbhit()) {
									 getch();
									 break;
								}
							}

							printf("\nSeq. %ld mal getriggert...",l);
							while(1) {
								if (kbhit()) {
									 getch();
									 break;
								}
							}
							inp_unit->ADR_CTRL = MODE_SLOW;
							break;

			case '7':	// LV read single
							{
								unsigned 	l;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								inp_mode(IM_VERTEX);

								// LV freischalten
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON;
								// LV Trigger
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_LV_TRIGGER;

								if (read_event(EVF_NODAC, RO_RAW)==0)
//								if (read_event()==0)
									printf("[%05d] Daten gelesen...\n",rout_len);
								else {
									printf("[KEINE] Daten gelesen...\n");
									continue;
								}

								for (l=0; l<(unsigned)(rout_len/2)+2; l++) {
									printf("%04X,",d[l]);
								}

								inp_unit->ADR_CTRL = MODE_SLOW;
							}
							break;

			case '8':   // LV read cont
							{
								unsigned 	cw,m=0,l=0;
								char 			c;
								u_long		*data;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								data = (u_long*)d;
								inp_mode(IM_VERTEX);

								inp_unit->ADR_CTRL = MODE_FAST_SER;
								clrscr();

								// ADC Dummy Data, LV freischalten , Fast Mode (in line)
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON;

								cw = 0x00AC;

								while(1) {
									m++;
									// LV Trigger
									inp_unit->ADR_CTRL = cw | CTRL_LV_TRIGGER;

									delay(10);
									gotoxy(1,2);

									cw = inp_unit->ADR_CTRL;
									printf("\n");
									printf("ESC  Stop\n");
									printf("A    toggle ADC / Dummy ADC (Akt: ");
									if (cw&CTRL_ADCTESTDATA)
											printf("Dummy ADC )\n");
									else  printf("ADC       )\n");
									printf("[%d] Loops",m);

									gotoxy(1,10);
									if (read_event(EVF_NODAC, RO_RAW)==0)
//									if (read_event()==0)
										printf("[%05d] Daten gelesen...\n",rout_len);
									else {
										printf("[KEINE] Daten gelesen...\n");
										clrscr();
										break;
									}

									for (l=0; l<(unsigned)(rout_len/4)+1; l++) {
										if ((l%8)==0) printf("\n"); else printf(",");
										printf("%08lX",data[l]);
									}
									printf("                 ");


									if (kbhit()) {
//										 while(kbhit())
										 c=toupper(getch());
										 if ((c == CTL_C) || (c == ESC)) break;

										 if(c=='A') {
											 if ((cw&CTRL_ADCTESTDATA)>0)
													cw = cw & ~CTRL_ADCTESTDATA;
											 else cw = cw | CTRL_ADCTESTDATA;

//											 inp_unit->ADR_CTRL = cw;
										 }
									}
								}
								printf("\n%d getriggert...\n",m);
								inp_unit->ADR_CTRL = MODE_SLOW;
							}
							break;

			case '9':
							printf("\nLV Sequencer Counter\n\n");

							printf("COUNTER 0 : %d\n",inp_unit->ADR_COUNTER_LV[0]);
							printf("COUNTER 1 : %d\n",inp_unit->ADR_COUNTER_LV[1]);
							printf("COUNTER 2 : %d\n",inp_unit->ADR_COUNTER_LV[2]);
							printf("COUNTER 3 : %d\n",inp_unit->ADR_COUNTER_LV[3]);
							printf("\n");
							break;

			case 'A':	inp_unit->ADR_ACTION = R_CNTRSTLV;
							printf("\nCounter zurueck gesetzt...\n\n");
							break;





			/************ HV ************/

			case 'D':	printf("Output Polarity (HV) <000/7FF> -> ");
							inp_unit->ADR_SEQPOL_HV = (u_short)Read_Hexa(inp_unit->ADR_SEQPOL_HV,0x7FF);
							break;
			case 'E':   printf("Chip Select Polarity (HV) <000/3FF> -> ");
							inp_unit->ADR_CSPOL_HV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_HV,0x3FF);
							break;

			case 'F':	printf("ADC Offset Clocks (HV) <0/F> -> ");
							inp_unit->ADR_ADCOFFSET_HV = (u_short)Read_Hexa(inp_unit->ADR_ADCOFFSET_HV,0xF);
							break;

			case 'G':	printf("Chip Mask (HV) <000/3FF> -> ");
							inp_unit->ADR_CSPOL_HV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_HV,0x3FF);
							break;

			case 'H':	printf("Max Data (HV) <0000/FFFF> -> ");
							inp_unit->ADR_CSPOL_HV = (u_short)Read_Hexa(inp_unit->ADR_CSPOL_HV,0xFFFF);
							break;


			case 'I':	// HV single
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_HV_ON;
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_HV_ON | CTRL_LV_TRIGGER;
							break;

			case 'J': 	// HV cont
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_HV_ON;
							l=0;
							printf("\n\ntriggering HV Sequencer");

							while(1) {
								if (l%1000) printf(".");
								l++;
								delay(10);
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_HV_ON | CTRL_LV_TRIGGER;
//								inp_unit->ADR_CTRL = 0x00AC;
								if (kbhit()) {
									 getch();
									 break;
								}
							}

							printf("\nSeq. %ld mal getriggert...",l);
							while(1) {
								if (kbhit()) {
									 getch();
									 break;
								}
							}
							break;

			case 'K':   // HV read single
							{
								unsigned 	l;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								inp_mode(IM_VERTEX);

								// HV freischalten
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_HV_ON;
								// HV Trigger
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_HV_ON | CTRL_LV_TRIGGER;


								if (read_event(EVF_NODAC, RO_RAW)==0)
//								if (read_event()==0)
									printf("[%05d] Daten gelesen...\n",rout_len);
								else {
									printf("[KEINE] Daten gelesen...\n");
									continue;
								}

								for (l=0; l<(unsigned)(rout_len/2)+2; l++) {
									printf("%04X,",d[l]);
								}

							}
							break;

			case 'M':   // HV read cont
							{
								unsigned 	cw,m=0,l=0;
								char 			c;
								u_long		*data;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								data = (u_long *)d;
								inp_mode(IM_VERTEX);

								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_HV_ON;
								clrscr();
								cw = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_HV_ON | CTRL_LV_TRIGGER;

								while(1) {
									m++;
									// Trigger
									inp_unit->ADR_CTRL = cw | CTRL_HV_TRIGGER;

									delay(10);
									gotoxy(1,2);

									cw = inp_unit->ADR_CTRL;
									printf("\n");
									printf("ESC  Stop\n");
									printf("A    toggle ADC / Dummy ADC (Akt: ");
									if (cw&CTRL_ADCTESTDATA)
											printf("Dummy ADC )\n");
									else  printf("ADC       )\n");
									printf("[%d] Loops",m);

									gotoxy(1,10);
									if (read_event(EVF_NODAC, RO_RAW)==0)
//									if (read_event()==0)
										printf("[%05d] Daten gelesen...\n",rout_len);
									else {
										printf("[KEINE] Daten gelesen...\n");
										clrscr();
										break;
									}

									for (l=0; l<(unsigned)(rout_len/4)+1; l++) {
										if ((l%8)==0) printf("\n"); else printf(",");
										printf("%08lX",data[l]);
									}
									printf("                 ");

									if (kbhit()) {
										 c=toupper(getch());
										 if ((c == CTL_C) || (c == ESC)) break;

										 if(c=='A') {
											 if ((cw&CTRL_ADCTESTDATA)>0)
													cw = cw & 0xff7f;
											 else cw = cw | 0x0080;

										 }
									}
								}
								printf("\n%d getriggert...\n",m);
							}
							break;



			case 'N':	printf("\nHV Sequencer Counter\n\n");
							printf("COUNTER 0 : %d\n",inp_unit->ADR_COUNTER_HV[0]);
							printf("COUNTER 1 : %d\n",inp_unit->ADR_COUNTER_HV[1]);
							printf("COUNTER 2 : %d\n",inp_unit->ADR_COUNTER_HV[2]);
							printf("COUNTER 3 : %d\n",inp_unit->ADR_COUNTER_HV[3]);
							printf("\n");
							break;

			case 'O':	inp_unit->ADR_ACTION = R_CNTRSTHV;
							printf("\nCounter zurueck gesetzt...\n\n");
							break;



			/************ LV & HV ************/


			case 'Q': 	// LV & HV single
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON;
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON | CTRL_LV_TRIGGER | CTRL_HV_TRIGGER;
							break;

			case 'T':   // LV & HV cont
							inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON;
							l=0;
							printf("\n\ntriggering LV & HV Sequencer");

							while(1) {
								if (l%1000) printf(".");
								l++;
								delay(10);
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON | CTRL_LV_TRIGGER | CTRL_HV_TRIGGER;
								if (kbhit()) {
									 getch();
									 break;
								}
							}

							printf("\nSeq. %ld mal getriggert...",l);
							while(1) {
								if (kbhit()) {
									 getch();
									 break;
								}
							}
							break;

			case 'U':   // LV & HV read single
							{
								unsigned 	l;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								inp_mode(IM_VERTEX);

								// HV freischalten
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON;
								// HV Trigger
								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON | CTRL_LV_TRIGGER | CTRL_HV_TRIGGER;


								if (read_event(EVF_NODAC, RO_RAW)==0)
//								if (read_event()==0)
									printf("[%05d] Daten gelesen...\n",rout_len);
								else {
									printf("[KEINE] Daten gelesen...\n");
									continue;
								}

								for (l=0; l<(unsigned)(rout_len/2)+2; l++) {
									printf("%04X,",d[l]);
								}

							}
							break;

			case 'V':   // LV & HV read cont
							{
								unsigned 	cw,l=0,s=0,last,first;
								u_long		m=0;
								char 			c;
								u_long		*data,n;
								u_short		*d;
								d = (u_short*)dmabuf[0];
								inp_mode(IM_VERTEX);

								inp_unit->ADR_CTRL = MODE_FAST_SER | CTRL_ADCTESTDATA | CTRL_LV_ON | CTRL_HV_ON;
								clrscr();
								cw = MODE_FAST_SER | CTRL_LV_ON | CTRL_HV_ON;
								inp_unit->ADR_CTRL = cw;

								first = 1;

								m=0;
								while(1) {
									m++;
									// Trigger
									inp_unit->ADR_CTRL = cw | CTRL_HV_TRIGGER | CTRL_LV_TRIGGER;

									delay(10);
									gotoxy(1,2);

									cw = inp_unit->ADR_CTRL;
									printf("\n");
									printf("ESC  Stop\n");
									printf("A    toggle ADC / Dummy ADC (Akt: ");
									if (cw&CTRL_ADCTESTDATA)
											printf("Dummy ADC )\n");
									else  printf("ADC       )\n");
									printf("S    Sort Data Block");
									printf("\n[%ld] Loops ",m);

									gotoxy(1,10);
									if (read_event(EVF_NODAC, RO_RAW)==0)
//									if (read_event()==0)
										printf("[%05d] Daten gelesen...\n",rout_len);
									else {
										printf("[KEINE] Daten gelesen...\n");
										clrscr();
										break;
									}
									data = (u_long *)d;


									if (!s) {
										printf("LV & HV Data\n");
										for (l=0; l<(unsigned)(rout_len/4)+1; l++) {
											if ((l%8)==0) printf("\n"); else printf(",");
											printf("%08lX",data[l]);
										}
										printf("                 ");
									} else {
										printf("LV Data");
										n = 0;
										for (l=0; l<(unsigned)(rout_len/4); l++) {
											if ((data[l]&0x08000000l)==0) {
												if ((n++%8)==0) printf("\n"); else printf(",");
												printf("%08lX",data[l]);
											}
										}
										printf("                 ");
										printf("\n\nHV Data");
										n = 0;

										for (l=0; l<(unsigned)(rout_len/4); l++) {
											if ((data[l]&0x08000000l)>0) {
												if ((n++%8)==0) printf("\n"); else printf(",");
												printf("%08lX",data[l]);
											}
										}
										printf("                 ");

									}

									if (first)	{ first = 0; last = rout_len; }
									if (rout_len!=last) {
										c=toupper(getch());
										if ((c == CTL_C) || (c == ESC)) break;
										first = 1;
									}
									last = rout_len;


									if (kbhit()) {
										 c=toupper(getch());
										 if ((c == CTL_C) || (c == ESC)) break;

										 if(c=='A') {
											 if ((cw&CTRL_ADCTESTDATA)>0)
													cw = cw & 0xff7f;
											 else cw = cw | 0x0080;

										 }
										 if (c=='S') {
											s=~s;
											clrscr();
										 }
									}
								}
								printf("\n%d getriggert...\n",m);
								inp_unit->ADR_CTRL = MODE_SLOW;
							}
							break;

			case 'W':	printf("\nLV Sequencer Counter\n\n");
							printf("COUNTER 0 : %d\n",inp_unit->ADR_COUNTER_LV[0]);
							printf("COUNTER 1 : %d\n",inp_unit->ADR_COUNTER_LV[1]);
							printf("COUNTER 2 : %d\n",inp_unit->ADR_COUNTER_LV[2]);
							printf("COUNTER 3 : %d\n",inp_unit->ADR_COUNTER_LV[3]);
							printf("\n");
							printf("\nHV Sequencer Counter\n\n");
							printf("COUNTER 0 : %d\n",inp_unit->ADR_COUNTER_HV[0]);
							printf("COUNTER 1 : %d\n",inp_unit->ADR_COUNTER_HV[1]);
							printf("COUNTER 2 : %d\n",inp_unit->ADR_COUNTER_HV[2]);
							printf("COUNTER 3 : %d\n",inp_unit->ADR_COUNTER_HV[3]);
							printf("\n");
							break;

			case 'X':	inp_unit->ADR_ACTION = R_CNTRSTLV | R_CNTRSTHV;
							printf("\nCounter zurueck gesetzt...\n\n");
							break;



		}

//		printf("ux=%d\n",ux);
		UCLastKey[MEN_SEQ]=key;

	}
}




static int slowctrl_menu(void)
{
	char		key;
	char		hd;
	int		men=MEN_SLOWCTRL;
	unsigned char lpot = 0;
	unsigned char hpot = 0;

	inp_unit->ADR_CTRL = MODE_SLOW;
	hd=1;
	while (1) {
		clrscr();
		while (kbhit()) getch();

		errno=0;
		if (hd) {
			Writeln();
//			inp_assign(sc_conf, 1);
			printf("ADC Test\n");
			printf("\n");
			printf("LV Kanal\n");
			printf("  Read ADC                             ..1\n");
			printf("  Offset Poti Rampe               %03d  ..2\n",lpot);
			printf("  Offset Poti Rampe und ADC Read       ..3\n");
			printf("  Write File to Front                  ..4\n");

			printf("\n");
			printf("HV Kanal\n");
			printf("  Read ADC                             ..A\n");
			printf("  Offset Poti Rampe               %03d  ..B\n",hpot);
			printf("  Offset Poti Rampe und ADC Read       ..C\n");
			printf("  Write File to Front                  ..D\n");

			printf("\n");
			printf("                                         %c ", UCLastKey[men]);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=UCLastKey[men];
		}

		use_default = (key == TAB);
		if (use_default || (key == CR)) key=UCLastKey[men];
		if (key > ' ') printf("%c\n", key);

		if (!inp_unit) continue;


		switch (key) {
			case '1':	{	// ADC Read
								unsigned int v = 0;

								inp_unit->ADR_CTRL   = MODE_SLOW;  // Slow Control on

								while(1) {
									delay(50);

									// Read ADC LV
									inp_unit->ADR_SCTRL = SCTRL_ADC_LV;

									// Daten lesen
									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&STAT_BUSY)==0) break;
									}

									v = inp_unit->ADR_SCTRLOUT;
									printf("ADC : %04X\r",v);

									if (kbhit()) {
										 getch();
										 break;
									}
								}
							}
							break;


			// WR Digi Poti LV
			case '2': 	{
								u_short  v;
								u_long	l;

								lpot = 0;
								while (1) {
									printf("Poti : %04d\r",lpot);

									//Poti beschreiben
									l = lpot;
									inp_unit->ADR_SCTRL = SCTRL_DATA07 | SCTRL_CS_LAST | SCTRL_DIGIPOTI_LV | (u_long)(l<<24);

									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&STAT_BUSY)==0) break;
									}

									delay(10);

									if (kbhit()) break;
									lpot=lpot++;
								}
							}
							break;


			case '3':   {  // Offset Rampe ADC Read
								unsigned int v = 0;
								u_long l = 0;

								inp_unit->ADR_CTRL   = MODE_SLOW;  // Slow Control on

								while(1) {
									delay(50);

									//Poti beschreiben
									l = lpot;
									inp_unit->ADR_SCTRL = SCTRL_DATA08 | SCTRL_CS_LAST | SCTRL_DIGIPOTI_LV | (u_long)(l<<24);

									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&STAT_BUSY)==0) break;
									}

									printf("Poti : %04d -> ",lpot);
									lpot++;


									// Read ADC HV
									inp_unit->ADR_SCTRL = SCTRL_ADC_LV;

									// Daten lesen
									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&STAT_BUSY)==0) break;
									}

									v = inp_unit->ADR_SCTRLOUT;
									printf("ADC : %04X\r",v);

									if (kbhit()) {
										 getch();
										 break;
									}
								}
							}
							break;

			case '4':	// File to Front LV
							{
								unsigned int v = 0;
								u_short  data[500];
								int		n=0,a;
								u_long	ul,l;

								LoadBitFileToBuffer("c:\\lvbit.txt",&n,data);
								inp_unit->ADR_CTRL   = 0x0000;

								l = 0;
								while(1) {
									gotoxy(1,20);
									printf("Loop [%ld]\n\n",l++);
									for (a=0; a<n; a++) {
										//DAC beschreiben
										if (a==n-1)
												ul = 0x0000F804 |((u_long)data[a]<<16);
										else	ul = 0x0000F004 |((u_long)data[a]<<16);
										inp_unit->ADR_SCTRL = ul;
										printf("%08X\n",ul);

										while (1) {
											v = inp_unit->ADR_STAT;
											if ((v&0x0001)==0) break;
										}
									}
									if (kbhit()) break;
//									while (!kbhit());
								}
							}
							break;





			case 'A':	{	// ADC Read
								unsigned int v = 0;

								inp_unit->ADR_CTRL   = 0x0000;  // Slow Control on

								while(1) {
									delay(50);

									// Read ADC HV
									inp_unit->ADR_SCTRL = 0x00000009;

									// Daten lesen
									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&0x0001)==0) break;
									}

									v = inp_unit->ADR_SCTRLOUT;
									printf("ADC : %04X\r",v);

									if (kbhit()) {
										 getch();
										 break;
									}
								}
							}
							break;


			// WR Digi Poti LV
			case 'B': 	{
								u_short  v;
								u_long	l;
								hpot = 0;
								while (1) {
									printf("Poti : %04d\r",hpot);

									//Poti beschreiben
									l = hpot;
									inp_unit->ADR_SCTRL = 0x00007802 | (u_long)(l<<24);

									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&0x0001)==0) break;
									}

									delay(10);

									if (kbhit()) break;
									hpot=hpot++;
								}
							}
							break;

			case 'C':   {  // Offset Rampe ADC Read
								unsigned int v = 0;
								u_long l = 0;

								inp_unit->ADR_CTRL   = 0x0000;  // Slow Control on

								while(1) {
									delay(50);

									//Poti beschreiben
									l = hpot;
									inp_unit->ADR_SCTRL = 0x00007802 | (l<<24);

									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&0x0001)==0) break;
									}

									printf("Poti : %04d -> ",hpot);
									hpot++;


									// Read ADC HV
									inp_unit->ADR_SCTRL = 0x00000009;

									// Daten lesen
									while (1) {
										v = inp_unit->ADR_STAT;
										if ((v&0x0001)==0) break;
									}

									v = inp_unit->ADR_SCTRLOUT;
									printf("ADC : %04X\r",v);

									if (kbhit()) {
										 getch();
										 break;
									}
								}
							}
							break;

			case 'D':	// File to Front LV
							{
								unsigned int v = 0;
								u_short  data[500];
								int		n=0,a;
								u_long	ul,l=0;

								LoadBitFileToBuffer("c:\\lvbit.txt",&n,data);
								inp_unit->ADR_CTRL   = 0x0000;

								while(1) {
									gotoxy(1,20);
									printf("Loop [%ld]\n\n",l++);
									for (a=0; a<n; a++) {
										//DAC beschreiben
										if (a==n-1)
												ul = 0x0000F805 |((u_long)data[a]<<16);
										else	ul = 0x0000F005 |((u_long)data[a]<<16);
										inp_unit->ADR_SCTRL = ul;
										printf("%08X\n",ul);

										while (1) {
											v = inp_unit->ADR_STAT;
											if ((v&0x0001)==0) break;
										}
									}
									if (kbhit()) break;
//									while (!kbhit());
								}
							}
							break;


		}

		UCLastKey[men]=key;

	}
}


//
int vertex_menu(void)
{
	u_int		ux;
	char		key;
	char		hd;

//xdmabuf=(UINT_DBUF*)&dmabuf[0];

	hd=1;
	while (1) {
		while (kbhit()) getch();

		errno=0;
		if (hd) {
			Writeln();
			inp_assign(sc_conf, 1);
			if (inp_bc->ADR_CARD_OFFL)
			printf("not all Card online        %04X\n", inp_bc->ADR_CARD_OFFL);
			printf("online Input Cards         %04X\n", inp_bc->ADR_CARD_ONL);
			printf("FIFO Data                  %04X\n", inp_bc->ADR_FIFO);
			printf("ERROR Bit                  %04X ..E\n",inp_bc->ADR_ERROR);
			printf("BUSY Bit                   %04X\n",*((u_short *)&inp_bc->ADR_TRIGGER));
			printf("Master Reset, assign input Bus  ..M/B\n");
			printf("\n");
			if (inp_conf) {
			printf("Karten Adresse (addr %2u)   %4u ..U\n", inp_conf->slv_addr, sc_conf->slv_sel);
			printf("\n");
			printf("Version                    %04X\n", inp_unit->ADR_IDENT);
				if (inp_conf->f1tdc) {
					printf("das ist eine F1 TDC Karte!!!\n");
				}
			}
			if (inp_conf && !inp_conf->f1tdc) {
			printf("Status                     %04X ..1\n", inp_unit->ADR_STAT);
			printf("Control                    %04X ..2\n", inp_unit->ADR_CTRL);
			printf("\n");
			printf("All High                   %04X ..3\n", inp_unit->ADR_ALLHIGH);
			printf("All LOW                    %04X ..4\n", inp_unit->ADR_ALLLOW);
			printf("\n");
			printf("Memory Test                     ..5\n");
			printf("Sequencer Test                  ..6\n");
			printf("Slow Control                    ..7\n");
			printf("\n");
			printf("JTAG utilities                  ..J\n");
			printf("MASTER Reset                    ..M\n");
			printf("\n");
			}
			printf("                                  %c ",	UCLastKey[MEN_MAIN]);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=UCLastKey[MEN_MAIN];
		}

		use_default = (key == TAB);
		if (use_default || (key == CR)) key=UCLastKey[MEN_MAIN];
		if (key > ' ') printf("%c\n", key);

		if (key=='B') { inp_assign(sc_conf, 0); UCLastKey[MEN_MAIN]=key; continue; }
		if (key=='M') { inp_bc->ADR_ACTION=MRESET; 	 UCLastKey[MEN_MAIN]=key; continue; }


		if (!inp_unit) continue;


		ux=key-'0';

		switch (key) {


			case 'E':	inp_unit->ADR_ACTION = CLEARERROR;
							break;

			case 'J': // JTAG utilities
						  {	JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata,&config.jtag_vertex, 1); continue; }



			case '1': 	ux = 1;
							printf("2Value 0x%04X ", inp_conf->slv_data[ux]);
							inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
							if (errno) continue;
							UCLastKey[MEN_MAIN]=key;
							inp_unit->ADR_STAT=inp_conf->slv_data[ux];
							break;
			case '2': 	ux = 2;
							printf("2Value 0x%04X ", inp_conf->slv_data[ux]);
							inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
							if (errno) continue;
							UCLastKey[MEN_MAIN]=key;
							inp_unit->ADR_CTRL=inp_conf->slv_data[ux];
							break;
			case '3': 	while(1) {
								printf("All High  %04X\n", inp_unit->ADR_ALLHIGH);
								if (kbhit()) {
									 getch();
									 break;
								}
							}
							ux = 3; UCLastKey[MEN_MAIN]=key;
							break;
			case '4': 	while(1) {
								printf("All Low  %04X\n", inp_unit->ADR_ALLLOW);
								if (kbhit()) {
									 getch();
									 break;
								}
							}
							ux = 4; UCLastKey[MEN_MAIN]=key;
							break;

							// Memory Test
			case '5':   ux = 5; UCLastKey[MEN_MAIN]=key;
							mem_menu();
							break;

							// Sequencer Test
			case '6':   ux = 5; UCLastKey[MEN_MAIN]=key;
							sequencer_menu();
							break;

							// SLOW Ctrl
			case '7': 	slowctrl_menu();
							break;

			case 'U': 	if (inp_assign(sc_conf, 2)) continue; UCLastKey[MEN_MAIN]=key; continue;

			default :
				printf("3Value  %5u ", inp_conf->slv_data[ux]);
				inp_conf->slv_data[ux]=(u_int)Read_Deci(inp_conf->slv_data[ux], -1);
				if (errno) continue;
				break;
		}

		UCLastKey[MEN_MAIN]=key;
	}
}


