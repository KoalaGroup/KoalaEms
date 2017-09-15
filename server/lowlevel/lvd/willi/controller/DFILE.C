#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef unsigned int		u_int;

#define OFFSET(strct, field)	((u_int)&((strct*)0)->field)

#define T_REF				25.0		// 40 MHz
#define REFCLKDIV			7
#define HSDIV				(T_REF*(1<<REFCLKDIV)/0.152)

#define THRH_VIN		0.5
#define THRH_MIN		-THRH_VIN
#define THRH_MAX		((255./128.-1.)*THRH_VIN)
#define THRH_SLOPE	((THRH_MAX-THRH_MIN)/255.0)

typedef struct {
	u_long	hlen;
	u_short	sec;
	u_short	min;
	u_short	hour;
	u_short	day;
	u_short	mon;
	u_short	year;
	u_short	edge;
	u_short	trig;
	u_short	hsdiv;
	short		lat;
	u_short	win;
	short		tdac;
	u_char	dac[100];		// u32 align
} DAT_INFO;

#define C_CHAN	0x100
u_long	*buf;
u_long	hits[4][C_CHAN];
u_long	hofl[4][C_CHAN];
u_long	totev[4];
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
	u_long	prec=0;
	u_long	el;
	u_int		i;
	u_long	x;
	DAT_INFO	*setup;
	float		res;
	u_int		con,c;
	int		sc;
	u_short	hofl_id=0xFFFF;
	char		hav=0;
	u_int		ln=0;
	u_int		ev_max=0;
	char		key;

	if (argc != 2) {
		printf("%u\n", argc);
		printf("usage: %s {datafile}\n", argv[0]);
		return 0;
	}

	if ((buf=calloc(0x8000, 2)) == 0) {
		printf("can't allocate buffer\n");
		return 0;
	}

	for (sc=0; sc < 3; sc++) {
		totev[sc]=0;
		for (c=0; c < C_CHAN; c++) { hits[sc][c]=0; hofl[sc][c]=0; }
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

//printf("len=%08lX\n", len);
		i=0;
		do {
			x=buf[i];
#if 1
if ((x >>16) == 0) {
	printf("format 2 error at %06lX =%08lX, lrec at %06lX, len =%04X\n",
			 org+(i<<2), x, prec, (u_int)(org+(i<<2)-prec));
if (i < 3 ) printf("i =%u\n", i);
else
	printf(" %08lX %08lX %08lX\n", buf[i-3], buf[i-2], buf[i-1]);
	ln=0; printf("press any key "); key=getch();
	printf("\n");
	i++;
	x=buf[i];
	if ((key == 0x1B) || (key == 3)) {
		close(hdl);
		return 0;
	}
}
#endif
			el=(u_short)x;
			if ((u_int)el > ev_max) ev_max=(u_int)el;

//if (el >= 0x180) {
//printf("%06lX: %08lX %08lX %08lX\n", org, buf[i], buf[i+1], buf[i+2]);
//if (i < 3 ) printf("i =%u\n", i);
//else
//	printf(" %08lX %08lX %08lX\n", buf[i-3], buf[i-2], buf[i-1]);
//}
			if (((u_long)i<<2)+el > len) {
				if (el > 0x10000L) {
					printf("format 1 error at %06lX =%08lX\n", org+(i<<2), x);
					close(hdl);
					return 0;
				}

				org=lseek(hdl, org+(i<<2), SEEK_SET);
				org -=len;
				break;
			}

//printf("%06lX: %08lX %08lX %08lX\n", org+(i<<2), buf[i], buf[i+1], buf[i+2]);
			if ((x >>16) == 0x8001) {
				if (hav) {
					printf("count of hits\n"); ln++;
					for (sc=0; sc < 3; sc++)
						for (c=0; c < C_CHAN; c++)
							if (hits[sc][c]) {
								if (++ln >= 40) {
									ln=0; printf("press any key "); getch();
									printf("\n");
								}

								printf("%2d/%02X: %10u\n", sc-1, c, hits[sc][c]);
							}

					printf("\ncount of hit overflows\n"); ln++;
					for (sc=0; sc < 3; sc++)
						for (c=0; c < C_CHAN; c++)
							if (hofl[sc][c]) {
								if (++ln >= 40) {
									ln=0; printf("press any key "); getch();
									printf("\n");
								}

								printf("%2d/%02X: %10u\n", sc-1, c, hofl[sc][c]);
							}

					printf("\ntotal events\n"); ln++;
					for (sc=0; sc < 3; sc++)
						if (totev[sc]) { printf("%2d: %10u\n", sc-1, totev[sc]); ln++; }
				}

				for (sc=0; sc < 3; sc++) {
					totev[sc]=0;
					for (c=0; c < C_CHAN; c++) { hits[sc][c]=0; hofl[sc][c]=0; }
				}

				hav=0;
				setup=(DAT_INFO*)(buf+i);
				printf("%u.%02u.%04u %2u:%02u:%02u",
						 setup->day, setup->mon, setup->year,
						 setup->hour, setup->min, setup->sec);
				switch ( setup->edge) {

			case 0:
					printf(" falling edge");
					break;

			case 1:
					printf(" rising edge");
					break;

			case 2:
					printf(" falling and rising edge");
					break;
				}

				if (setup->trig || (setup->tdac >= 0)) printf("; raw hits");
				else { printf("; triggered hits"); hofl_id=0x7FFF; }

				res=HSDIV/setup->hsdiv;
				printf("; resolution %1.1fps\n", res); ln++;
				if (setup->tdac >= 0) {
					printf("           test pulse with %1.3fV threshold\n",
							 THRH_MIN+THRH_SLOPE*setup->tdac); ln++;
				} else {
					if (setup->trig == 0) {
						printf("           latancy %1.0fns window %1.0fns\n",
								 setup->lat*res/1000., setup->win*res/1000.); ln++;
					}

					con=(u_int)(el-OFFSET(DAT_INFO, dac));
					for (c=0; c < con;) {
						printf(" %6.3f", THRH_MIN+THRH_SLOPE*setup->dac[c]);
						c++;
						if ((c %4) == 0) printf("\n"); ln++;
					}
				}
			} else {
				if ((x >>16) == 0x8002) {
					printf("%s\n", (char*)(buf+i+1)); ln++;
				} else {
					if ((x >>16) == 0x8000) {
						hav=1;
						sc=((u_int)(buf[i+1]>>16)+1)&3;
						totev[sc]++;
						for (c=3; c < (u_int)(el>>2); c++) {
							x=buf[i+c];
							if ((u_short)x == hofl_id) hofl[sc][(u_char)(x >>16)]++;
							else hits[sc][(u_char)(x >>16)]++;
						}
					} else {
						printf("format 2 error at %06lX =%08lX, lrec at %06lX, len =%04X\n",
								 org+(i<<2), x, prec, (u_int)(org+(i<<2)-prec));
//ln=0; printf("press any key "); key=getch();
//printf("\n");
//						if ((key == 0x1B) || (key == 3)) {
						close(hdl);
						return 0;
//						}
//						el=4;
					}
				}
			}

			prec=org+(i<<2);
		} while ((i+=(u_int)(el>>2)) < (u_int)(len>>2));

		org +=len;
//printf("%06lX\n", org);
	} while (len == 0x10000L);

	close(hdl);
	if (!hav) return 0;

	printf("count of hits\n"); ln++;
	for (sc=0; sc < 3; sc++)
		for (c=0; c < C_CHAN; c++)
			if (hits[sc][c]) {
				if (++ln >= 40) {
					ln=0; printf("press any key "); getch();
					printf("\n");
				}

				printf("%2d/%02X: %10u\n", sc-1, c, hits[sc][c]);
			}

	printf("\ncount of hit overflows\n"); ln++;
	for (sc=0; sc < 3; sc++)
		for (c=0; c < C_CHAN; c++)
			if (hofl[sc][c]) {
				if (++ln >= 40) {
					ln=0; printf("press any key "); getch();
					printf("\n");
				}

				printf("%2d/%02X: %10u\n", sc-1, c, hofl[sc][c]);
			}

	printf("\ntotal events\n"); ln++;
	for (sc=0; sc < 3; sc++)
		if (totev[sc]) { printf("%2d: %10u\n", sc-1, totev[sc]); ln++; }

	printf("\nmax event length: %04X\n", ev_max);
	return 0;
}
