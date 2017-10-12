#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <ctype.h>
#include "pci.h"
#include "regs.h"
#include "seq.h"

//
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef unsigned int		u_int;

PCI_CONFIG		config;		// will be imported at pci.c, pciconf.c and pcibios.c

static u_short	*buf;
static char		ProgName[20];
static char		*pinp;
static FILE		*logfile=0;
static char		logname[80];

static int usage(void)
{
	printf("usage: %s {datafile} [/<options>]\n", ProgName);
	printf("       /v      verbose\n");
	return 1;
}

static int start(int argc, char *argv[])
{
	int	ix,iy,iz;

	ix=strlen(argv[0]);
	while (ix) {
		ix--;
		if ((argv[0][ix] == '/') || (argv[0][ix] == '\\') ||
			 (argv[0][ix] == ':')) { ix++; break; }
	}

	iy=0;
	while (1) {
		ProgName[iy]=argv[0][ix++];
		if ((ProgName[iy] == '.') || !ProgName[iy]) { ProgName[iy]=0; break; }

		iy++;
	}

	ix=1; iy=0;
	while (ix < argc) {
		if (argv[ix][0] == '/') {
			iz=1;
			while (argv[ix][iz]) {
				switch (argv[ix][iz]) {
	case '/':	break;

	case '?':	return usage();

//	case 'v':
//	case 'V':	verbose=1; break;

	default:
					printf("Option %c ist nicht erlaubt\n\n", argv[ix][iz]);
					return usage();
				}

				iz++;
			}
		} else {
			iy++;
			pinp=argv[ix];
		}

		ix++;
	}

	if (iy != 1) return usage();

	return 0;
}
//
//--------------------------- close_logfile ----------------------------------
//
static int close_logfile(void)
{
	int	ret;

	if (!logfile) return 0;

	fflush(logfile); ret=0;
	if (fclose(logfile)) {
		perror("can't close logfile");
		printf("name =%s", logname);
		ret=1;
	}

	logfile=0;
	return ret;
}
//
//--------------------------- open_logfile -----------------------------------
//
static int open_logfile(void)
{
	char	*str;
	char	ln[4];

	printf("logfile: ");
	str=Read_String(logname, sizeof(logname));
	if (errno) {
		close_logfile();
		return 1;
	}

	if (logfile && !strcmp(str, logname)) return 0;

	if (close_logfile()) return 1;

	if (!*str) return 0;

	logfile=fopen(str, "r");
	if (logfile) {
		fclose(logfile); logfile=0;
		printf("overwride %s (Y) ", str);
		if (   Readln(ln, sizeof(ln))
			 || (ln[0] && (toupper(ln[0]) != 'Y') && (toupper(ln[0]) != 'J'))) {
			return 1;
		}
	}

	logfile=fopen(str, "w");
	if (!logfile) {
		perror("can't open logfile");
		printf("name =%s\n", str);
		return 1;
	}

	strcpy(logname, str);
	return 0;
}
//
//--------------------------- seq_inst --------------------------------------
//
// sequencer instruction
//
static char		ibuf[80];
//
static int seq_inst(u_long pc, u_short *sram)
{
	u_int		l;
	u_char	opc,mul;
	u_char	err;

	opc=*sram >>B_DATA; err=0;
	l=sprintf(ibuf, "%05lX: ", pc);
	if (   ((opc &~0x01) == OPC_TRG)
		 || ((opc &~0x01) == OPC_WTRG)
		 || ((opc &~0x1F) == OPC_OUT)) {	// data

		mul=0;
		if ((opc &~0x1F) == OPC_OUT) mul=(*sram >>B_DATA) &M_DREPT;

		if (mul) l +=sprintf(ibuf+l, "%2u*", mul+1);
		else l +=sprintf(ibuf+l, "   ");

		l +=sprintf(ibuf+l, "%03X", *sram &M_DATA);

	} else l +=sprintf(ibuf+l, "           ");

	switch (opc &0x78) {
case 0x00:
		if ((opc &~0x1) == OPC_WTRG) {
			l +=sprintf(ibuf+l, " WTG%u", opc&1);
			break;
		}

		if (*sram &M_DATA) err=13;

		if (opc == OPC_RTS) {
			l +=sprintf(ibuf+l, " RTS");
			break;
		}

		if (opc == OPC_WHALT) {
			l +=sprintf(ibuf+l, " WH");
			break;
		}

		if ((opc &~0x1) == OPC_RES) {
			err=11;
			break;
		}

		if ((opc &~0x1) == OPC_COUNT) {
			l +=sprintf(ibuf+l, " C%u", opc &1);
			break;
		}

		err=12;
		break;

case OPC_DGAP:
		l +=sprintf(ibuf+l, " DGAP %4u", (*sram &M_GAP)+1);
		break;

case OPC_SW:
case OPC_SW+0x08:
		l +=sprintf(ibuf+l, (opc &0x08) ? " SWC %u" : " SW %u", opc &0x07);
		break;

case 0x20:
		if ((opc &~0x1) == OPC_TRG) {
			l +=sprintf(ibuf+l, " T%u", opc &1);
			break;
		}

		if ((opc &~0x1) == OPC_LPCNT) {
			l +=sprintf(ibuf+l, " LC%u %3u", (opc &1)+2, *sram &0xFF);
			break;
		}

		if ((opc &~0x3) == OPC_JMPS) {
			l +=sprintf(ibuf+l, " JMPS %05lX", (pc&~M_JMPS)|(*sram &M_JMPS));
			break;
		}

		err=14;
		break;

case OPC_TDJNZ:
case OPC_TDJNZ+0x8:
		l +=sprintf(ibuf+l, " TDJNZ %u %05lX", (opc>>2) &3, (pc&~M_JMPS)|(*sram &M_JMPS));
		break;

case OPC_JMP:
case OPC_JMP+0x8:
		l +=sprintf(ibuf+l, " JMP %05lX", (u_long)(*sram &M_GAP) <<B_EJMP);
		break;

case OPC_JSR:
case OPC_JSR+0x8:
		l +=sprintf(ibuf+l, " JSR %05lX", (u_long)(*sram &M_GAP) <<B_EJMP);
		break;

case OPC_OUT:
case OPC_OUT+0x08:
case OPC_OUT+0x10:
case OPC_OUT+0x18:
		break;

default:
		err=15;
		break;
	}

	if (err) l +=sprintf(ibuf+l, " ;ill opc %02X/%d", opc, err);

	return l;
}
//
//----------------------------------------------------------------------------
//										main
//----------------------------------------------------------------------------
//
static u_short	zero2[2];

int main(int argc, char *argv[])
{
	int   	hdl;
	u_long	org=0;
	u_long	len;
	u_int		i;
	u_long	iz;

	if (start(argc, argv)) return 0;

	if ((buf=calloc(0x8000, 2)) == 0) {
		printf("can't allocate buffer\n");
		return 0;
	}

	if ((hdl=_rtl_open(pinp, O_RDONLY)) == -1) {
		perror("error opening data file");
		printf("name =%s", pinp);
		return 0;
	}

	if (open_logfile()) {
		close(hdl);
		return 0;
	}

	iz=0;
	do {
		if ((i=read(hdl, buf, 0x8000)) == 0xFFFF) {
			perror("error reading data file");
			break;
		}

		len=i;
		if (i == 0x8000) {
			if ((i=read(hdl, buf+0x2000, 0x8000)) == 0xFFFF) {
				perror("error reading data file");
				break;
			}

			len +=i;
		}

printf("len=%08lX\n", len);
		i=0;
		do {
			if (buf[i] == 0) { iz++; continue; }

			if (iz) { 
				ibuf[seq_inst(org/2 +(i-iz), zero2)]=0;
				fprintf(logfile, "%04X  %-30s\n", 0, ibuf);
				iz--;
			}

			while (iz) {
				fprintf(logfile, "%04X", 0);
				if (!((i-(u_int)iz)&0x3F)) fprintf(logfile, "  %05lX:\n", org/2 +(i-iz));
				else fprintf(logfile, "\n");

				iz--;
			}

			ibuf[seq_inst(org/2 +i, buf +i)]=0;
			fprintf(logfile, "%04X  %-30s\n", buf[i], ibuf);
		} while (++i < (u_int)(len>>1));

		org +=len;
	} while (len == 0x10000L);

	for (iz=0; iz < 8; iz++) fprintf(logfile, "%04X\n", 0);

	close_logfile();
	close(hdl);
	return 0;
}
