#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>

#define nKEY

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef unsigned int		u_int;

#define JS_RESET			0
#define JS_IDLE			8
#define JS_DR_SCAN		1
#define JS_DR_CAPTURE	2
#define JS_DR_SHIFT		3
#define JS_DR_EXIT1		4
#define JS_DR_PAUSE		5
#define JS_DR_EXIT2		6
#define JS_DR_UPDATE		7
#define JS_IR_SCAN		9
#define JS_IR_CAPTURE	10
#define JS_IR_SHIFT		11
#define JS_IR_EXIT1		12
#define JS_IR_PAUSE		13
#define JS_IR_EXIT2		14
#define JS_IR_UPDATE		15

#define SERASE		0x0A	// Globally refines the programmed values in the array
#define FVFY3		0xE2
#define ISPEN		0xE8
#define FPGM		0xEA	// Programs specific bit values at specified addresses
#define FADDR		0xEB	// Sets the PROM array address register
#define FERASE		0xEC	// Erases a specified program memory block
#define FDATA0		0xED	// Accesses the array word-line register ?
#define CONFIG		0xEE
#define NORMRST	0xF0	// CONLD, Exits ISP Mode ?
#define FDATA3		0xF3	// 6
#define FVFY1		0xF8	// Reads the fuse values at specified addresses
#define IDCODE		0xFE

#define XSC_DATA_RDPT		0x0004
#define XSC_DATA_UC			0x0006
#define XSC_DATA_DONE		0x0009
#define XSC_DATA_CCB			0x000C
#define XSC_BLANK_CHECK		0x000D
#define XSC_DATA_SUCR		0x000E
#define XSC_OP_STATUS		0x00E3
#define ISC_PROGRAM			0x00EA	// FPGM
#define ISC_ADDRESS_SHIFT	0x00EB	// FADDR
#define ISC_ERASE				0x00EC	// FERASE
#define ISC_DATA_SHIFT		0x00ED	// FDATA0
#define XSC_READ				0x00EF
#define CONLD					0x00F0	// NORMST
#define XSC_DATA_BTC			0x00F2
#define XSC_CLEAR_STATUS	0x00F4
#define XSC_DATA_WRPT		0x00F7
#define XSC_UNLOCK			0xAA55

#define BYPASS		0xFFFF

u_long	*buf;
//
//----------------------------------------------------------------------------
//										main
//----------------------------------------------------------------------------
//
int main(int argc, char *argv[])
{
	int   	hdl;
	u_long	len;
	u_long	org=0;
	u_long	el;
	u_int		i,j;
	u_int		hdr=1;
	u_int		sw=0;
	u_long	dat,tdat,idat0,idat1,odat0;
	u_int		ln=0;
	u_int		sdat,ldat=16;
	u_long	us=0;
	u_long	cnt=0,lcnt;
	u_int		ir0,ir1;
#ifdef KEY
	char		key;
#endif
	u_int		err=0xFFFF;

	if (argc != 2) {
		printf("%u\n", argc);
		printf("usage: %s {datafile}\n", argv[0]);
		return 0;
	}

	if ((buf=calloc(0x8000, 2)) == 0) {
		printf("can't allocate buffer\n");
		return 0;
	}

	if ((hdl=_rtl_open(argv[1], O_RDONLY)) == -1) {
		perror("error opening data file");
		printf("name =%s", argv[1]);
		return 0;
	}

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

printf("len=%08lX\n", len); ln++;
		i=0; j=0;
		if (!hdr) el=len;
		do {
			dat=buf[i];
			if (hdr) {
				el=(u_short)dat;
				if (((u_long)i<<2)+el > len) {
					if (el > 0x10000L) {
						printf("format 1 error at %06lX =%08lX\n", org+(i<<2), dat);
						close(hdl);
						return 0;
					}

					org=lseek(hdl, org+(i<<2), SEEK_SET);
					org -=len;
					break;
				}

				j=3;
			}

//printf("%06lX: %08lX %08lX %08lX %05lX\n", org+(i<<2), buf[i], buf[i+1], buf[i+2], el); ln++;
			while (j < (u_int)(el>>2)) {
				dat=buf[i+j++];
				switch (sw) {
			case 0:
					if (!(dat &0x80000000L)) {
						printf("\n sequence error\n"); ln++; break;
					}

					tdat=dat; sw=1;
					break;

			case 1:
					sw=0;
					if (dat &0x80000000L) {
						printf("\n sequence error\n"); ln++; break;
					}

					sdat=(u_int)(dat>>27);
					switch (ldat) {
				case JS_RESET:
						if (sdat != JS_IDLE) err=ldat;
						break;

				case JS_IDLE:
						if (sdat != JS_DR_SCAN) err=ldat;
						break;

				case JS_DR_SCAN:
						if ((sdat != JS_DR_CAPTURE) && (sdat != JS_IR_SCAN)) err=ldat;
						break;

				case JS_DR_CAPTURE:
						if ((sdat != JS_DR_SHIFT) && (sdat != JS_DR_EXIT1)) err=ldat;
						break;

				case JS_DR_SHIFT:
						if (sdat != JS_DR_EXIT1) err=ldat;
						break;

				case JS_DR_EXIT1:
						if ((sdat != JS_DR_PAUSE) && (sdat != JS_DR_UPDATE)) err=ldat;
						break;

				case JS_DR_PAUSE:
						if (sdat != JS_DR_EXIT2) err=ldat;
						break;

				case JS_DR_EXIT2:
						if ((sdat != JS_DR_UPDATE) && (sdat != JS_DR_SHIFT)) err=ldat;
						break;

				case JS_DR_UPDATE:
						if ((sdat != JS_DR_SCAN) && (sdat != JS_IDLE)) err=ldat;
						break;

				case JS_IR_SCAN:
						if ((sdat != JS_IR_CAPTURE) && (sdat != JS_RESET)) err=ldat;
						break;

				case JS_IR_CAPTURE:
						if ((sdat != JS_IR_SHIFT) && (sdat != JS_IR_EXIT1)) err=ldat;
						break;

				case JS_IR_SHIFT:
						if (sdat != JS_IR_EXIT1) err=ldat;
						break;

				case JS_IR_EXIT1:
						if ((sdat != JS_IR_PAUSE) && (sdat != JS_IR_UPDATE)) err=ldat;
						break;

				case JS_IR_PAUSE:
						if (sdat != JS_IR_EXIT2) err=ldat;
						break;

				case JS_IR_EXIT2:
						if ((sdat != JS_IR_UPDATE) && (sdat != JS_IR_SHIFT)) err=ldat;
						break;

				case JS_IR_UPDATE:
						if ((sdat != JS_DR_SCAN) && (sdat != JS_IDLE)) err=ldat;
						break;
					}

					ldat=sdat;
					if (err != 0xFFFF) {
						printf("error in state %u => %u\n", err, sdat);
						close(hdl);
						return 0;
					}

					if ((sdat == JS_DR_PAUSE) || (sdat == JS_IR_PAUSE))
						printf("====== PAUSE =======\n");

					lcnt=(dat-cnt)&0x07FFFFFFL; cnt=dat;
					if (sdat == JS_RESET) {
						printf("RESET\n"); ln++; us=tdat; break;
					}

					if (sdat == JS_IDLE) {
						printf("IDLE    %6lu %7lu us\n",
								 lcnt, (tdat-us)&0x7FFFFFFFL);
						ln++;
						us=tdat; break;
					}

					if ((sdat == JS_DR_EXIT1) || (sdat == JS_IR_EXIT1)) sw=2;
					break;

			case 2:
					idat0=dat;
					sw=3;
					break;

			case 3:
					idat1=dat;
					sw=4;
					break;

			case 4:
					odat0=dat;
					sw=5;
					break;

			case 5:
					sw=0;
					if (sdat == JS_IR_EXIT1) {
						if (lcnt == 16+6) {
							ir0=(u_int)(idat0 &0x3F);
							ir1=(u_int)(idat0 >>6);
							printf("IR      %6lu %04X %02X => %04X %02X ==> ",
									 lcnt, ir1, ir0, (u_int)(odat0 >>6), (u_int)(idat0 &0x3F));
							switch (ir1) {
						case ISPEN:					printf("ISPEN             "); break;
						case IDCODE:				printf("IDCODE            "); break;
						case CONFIG:				printf("CONFIG            "); break;
						case XSC_DATA_RDPT:		printf("XSC_DATA_RDPT     "); break;
						case XSC_DATA_UC:			printf("XSC_DATA_UC       "); break;
						case XSC_DATA_DONE:		printf("XSC_DATA_DONE     "); break;
						case XSC_DATA_CCB:		printf("XSC_DATA_CCB      "); break;
						case XSC_BLANK_CHECK:	printf("XSC_BLANK_CHECK   "); break;
						case XSC_OP_STATUS:		printf("XSC_OP_STATUS     "); break;
						case XSC_DATA_SUCR:		printf("XSC_DATA_SUCR     "); break;
						case ISC_PROGRAM:			printf("ISC_PROGRAM       "); break;
						case ISC_ADDRESS_SHIFT:	printf("ISC_ADDRESS_SHIFT "); break;
						case ISC_ERASE:			printf("ISC_ERASE         "); break;
						case ISC_DATA_SHIFT:		printf("ISC_DATA_SHIFT    "); break;
						case XSC_READ:				printf("XSC_READ          "); break;
						case CONLD:					printf("CONLD             "); break;
						case XSC_DATA_BTC:		printf("XSC_DATA_BTC      "); break;
						case XSC_CLEAR_STATUS:	printf("XSC_CLEAR_STATUS  "); break;
						case XSC_DATA_WRPT:		printf("XSC_DATA_WRPT     "); break;
						case XSC_UNLOCK:			printf("XSC_UNLOCK        "); break;
						case BYPASS:				printf("BYPASS            "); break;
						default:                printf("??????????        "); break;
							}

							if (ir0 == 0x3F) printf(" BYPASS");
							printf("\n");
							ln++;
							break;
						}
					}

					if ((sdat == JS_DR_EXIT1) && (ir0 == 0x3F)) {
						printf("DR      %6lu %08lX:%08lX %08lX:%08lX\n",
								lcnt, idat1>>1, (idat1<<31)|(idat0>>1),
								dat>>1, (dat<<31)|(odat0>>1));
						ln++;
						break;
					}

					if (sdat == JS_DR_EXIT1) printf("DR      ");
					else printf("IR      ");

					printf("%6lu %08lX:%08lX %08lX:%08lX\n",
							 lcnt, idat1, idat0, dat, odat0);
					ln++;
					break;

				}

				if (ln >= 40) {
					ln=0;
#ifdef KEY
					printf("press any key "); key=getch();
					printf("\n");
					if ((key == 0x1B) || (key == 3)) {
						close(hdl);
						return 0;
					}
#endif
				}
			}

		} while ((i+=(u_int)(el>>2)) < (u_int)(len>>2));

		org +=len;
//printf("%06lX\n", org);
	} while (len == 0x10000L);

	close(hdl);
	return 0;
}
