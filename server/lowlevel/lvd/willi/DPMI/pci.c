// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	pci.c -- Library for PCI Devices */

#pragma inline
#include <windows.h>
#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include "pci.h"

extern PCI_CONFIG	config;

char		pci_ok;		//-1: Device not found
							// 0: Region 0 not mapped
							// 1: Region 0 is IO
							// 2: Region 0 is MEM
char		debug;
char		last_key;
char		use_default;

PCI_PRESENT		bios_psnt;
PCI_DEVICE		pdev;
PCI_CONF_IO		cnf_acc;

static struct time	timerec;
static char				strbuf[80];

//---------------------------- mem_size --------------------------------------
//
u_long mem_size(void)
{
	u_int		ux;
	u_short	buf[4];
	u_long	iterator;
	u_long	lbuf[5];

u_int get_extmem2(u_short *buf);
u_int get_mementry(u_long *iter, u_long *lbuf);

	for (ux=0; ux < 4; buf[ux++]=0);
	if (!get_extmem2(buf)) {
		if (buf[3]) return (u_long)(buf[3]+0x100)<<16;
	}

	iterator=0;
	while (1) {
		for (ux=0; ux < 5; lbuf[ux++]=0);
		if (get_mementry(&iterator, lbuf)) return 0;

		if ((lbuf[4] == 1) && (lbuf[0] == 0x100000L)) return lbuf[2];

		if (!iterator) return 0;
	}
}
//
//---------------------------- swap_l ----------------------------------------
//
// swap 32bit
//
u_long swap_l(u_long val)
{
	union {
		u_long	l;
		u_char	b[4];
	} sw;

	sw.b[0]=(u_char)(val >>24);
	sw.b[1]=(u_char)(val >>16);
	sw.b[2]=(u_char)(val >>8);
	sw.b[3]=(u_char)val;
	return sw.l;
}


//---------------------------- swap_w ----------------------------------------
//
// swap 16bit
//
u_short swap_w(u_short val)
{
	return (val <<8) |(val >>8);
}


//---------------------------- MapPhysicalToLinear ---------------------------
//
// Attention: return long => dx|ax
//
static u_long MapPhysicalToLinear(u_long PhysicalAddr, u_long Length)
{
#if 0
	asm {
		mov   bx,PhysicalAddr+2
		mov   cx,PhysicalAddr
		mov   si,Length+2
		mov   di,Length
		mov   ax,0x0800				// DPMI call function code }
		int   0x31						// call DPMI function }
		jc    @error
		mov   dx,bx
		mov   ax,bx						// 32 Bit Mode
		shl	eax,16
		mov   ax,cx
		jmp   @exit
@error:
		xor   eax,eax
		mov   dx,ax
@exit:
	}

#endif

	DPMIREGS	regs;

	regs.ax=0x0800;
	regs.bx=(u_short)(PhysicalAddr >>16);
	regs.cx=(u_short)PhysicalAddr;
	regs.si=(u_short)(Length >>16);
	regs.di=(u_short)Length;
	if (DPMI_call(&regs)) return 0;

	return ((u_long)regs.bx <<16) |regs.cx;
}
//
#if (0)
//---------------------------- MapLinearToPhysical ---------------------------
//
static u_long MapLinearToPhysical(u_long PhysicalAddr, u_long Length)
{
	asm {
		mov   bx,PhysicalAddr+2
		mov   cx,PhysicalAddr
		mov   si,Length+2
		mov   di,Length
		mov   ax,0x0801				// DPMI call function code }
		int   0x31						// call DPMI function }
		jc    @error1
		mov   dx,bx
		mov   ax,bx						// 32 Bit Mode
		shl	eax,16
		mov   ax,cx
		jmp   @exit1
@error1:
		xor   eax,eax
		dec	eax
		mov   dx,ax
@exit1:
	}
}
#endif

//---------------------------- MakeSelector ----------------------------------
//
static long	tmp;

static u_short MakeSelector(u_long phys, u_long len)
{
	u_long	lin;
	u_short	sel;

	if (!len) return 0;

	if (!(lin=MapPhysicalToLinear(phys, len))) return 0;

//	printf("%08lX %08lX\n", lin, MapLinearToPhysical(0, lin));
//	WaitKey(0);
	if (!(sel=AllocSelector(SELECTOROF(&tmp)))) return 0;

	if (debug) printf("MakeSelector(0x%08lX, 0x%08lX) lin=0x%08lX, sel=0x%04X\n",
							phys, len, lin, sel);

	SetSelectorLimit(sel, len-1);
	return SetSelectorBase(sel, lin);
}

//------------------------------ str2up --------------------------------------
//
void str2up(char *str)
{
	while (*str) {
		if ((*str >= 'a') && (*str <= 'z')) *str -=0x20;
		str++;
	}
}

//---------------------------- Writeln --------------------------------------
//
void Writeln(void) { printf("\n"); }
//
//---------------------------- token ----------------------------------------
//
// Holt das naechste Wort aus str[], ab Position *ix.
//		Fuehrende Leerzeichen und TABs werden uebersprungen.
//		Das Wort geht bis zum naechsten ' ', TAB, '+', '-', '@' oder '*'
//		*ix wird auf die neue Position gesetzt.
//
char *token(char str[], int *ix)
{
	int	lx,tx;
	int	ux;

	lx=strlen(str);
	tx=0; ux=*ix;
	if (ux >= lx) { strbuf[0]=0; return strbuf; }

	while ((str[ux] == ' ') || (str[ux] == TAB)) ux++;

	if (ux >= lx) { strbuf[0]=0; *ix=ux; return strbuf; }

	strbuf[tx++]=toupper(str[ux++]);
	while (   (ux < lx)
			 && (str[ux] != ' ') && (str[ux] != TAB)
			 && (str[ux] != '+') && (str[ux] != '-')
			 && (str[ux] != '@') && (str[ux] != '*'))
		strbuf[tx++]=toupper(str[ux++]);

	strbuf[tx]=0; *ix=ux;
	return strbuf;
}


//--------------------------- value ------------------------------------------
//
// Input:  inx   steht auf erstes Zeichen,
// Outpt:  inx   auf naechstes Zeichen,
//
// dezimale Eingabe braucht '.' am Ende.
// hexa Eingabe ein 'H' am Ende.
//
long value(char *line, int *inx, int d_base)
{
	int	ix;
	long	tval,opr;
	char	ch0,ch1;
	int	base;
	char	base_;

	tval=0; base_=0;
	if ((d_base == 10) || (d_base == 16)) base=d_base; else base=10;

	ch0=line[*inx];
	if ((ch0 == '+') || (ch0 == '-')) (*inx)++; else ch0='+';

	while (1) {
		if (*inx >= strlen(line)) return tval;

		ch1=ch0;

		if (line[*inx] == '(') {
			(*inx)++; opr=value(line, inx, d_base);
			if (line[*inx] == ')') (*inx)++;
		} else {
			ix=*inx; opr=0;
			while ((ix < strlen(line)) &&
					 (line[ix] >= '0') && (line[ix] <= '9')) ix++;

			if ((ix < strlen(line)) && (line[ix] == '.')) {
				base=10; base_=0;
			}

			while ((ix < strlen(line)) &&
					 (((line[ix] >= '0') && (line[ix] <= '9')) ||
					  ((toupper(line[ix]) >= 'A') && (toupper(line[ix]) <= 'F'))))
				ix++;

			if ((ix <= strlen(line)) && (toupper(line[ix]) == 'H')) {
				base=16; base_=1;
			}

			while (*inx < ix) {
				ch0=toupper(line[(*inx)++]);
				if (ch0 >= 'A') ch0 -=7;
				opr=(opr *base) +(ch0 -'0');
			}

			if (base_) (*inx)++;

			ch0=line[*inx];
		}

		switch (ch1) {
	case '+':	tval=tval+opr; break;
	case '-':	tval=tval-opr; break;
	case '*':	tval=tval*opr; break;
	case '/':
			if (!opr) return 0x7FFFFFFFL;

			tval=tval /opr;
		}

		ch0=line[*inx];
		if ((*inx >= strlen(line)) ||
			 ((ch0 != '+') && (ch0 != '-') && (ch0 != '*') && (ch0 != '/')))
			return tval;

		(*inx)++;
	}
}


//------------------------------ timemark ------------------------------------
//
void timemark(void)
{
	gettime(&timerec);
//	printf("%d:%d:%d.%02d\n", timerec.ti_hour, timerec.ti_min, timerec.ti_sec, timerec.ti_hund);
}


//------------------------------ ZeitBin -------------------------------------
//
// Zeit in 1/100 Sekunden seit letzter Zeitmarke als Binaerwert
// Beim Start muá
//    timemark; oder GetTime(hour, minute, second, sec100);
//
// aufgerufen werden.
//
u_long ZeitBin(void)
{
	struct time	trec;
	long			tim;

	gettime(&trec);
	if ((tim=(100L*(60L*(60L*trec.ti_hour +trec.ti_min) +trec.ti_sec) +trec.ti_hund)
		 -(100L*(60L*(60L*timerec.ti_hour +timerec.ti_min) +timerec.ti_sec) +timerec.ti_hund)) < 0)
		tim +=(100*60*60*24L);
	return tim;
}


//------------------------------ dsp_rate ------------------------------------
//
//
void dsp_rate(u_long lp)
{
	u_long 	tm;
	double	ftm;
	double	rate;

	tm=ZeitBin();
	if (lp < 10) return;

	ftm=tm /100.0;
	printf("Zeit = %4.2f sec\n", ftm);
	if (!tm) return;

	rate=lp /ftm;
	printf("Rate = %1.3fk Zyklen pro Sekunde = %1.2f æs\n",
			 rate /1000.0, 1000000.0 /rate);
}
//
//------------------------------ WaitKey -------------------------------------
//
int WaitKey(
	int	key)	// >' ': display this key
					// !=0:  clear default
					// CR,ESC: return only when this key pressed
{
	int	ret=0;

	while (kbhit()) {
		last_key=getch();
		if (!key && ((last_key == CTL_C) || (last_key == ESC))) {
			errno=-1; return 1;
		}
	}

	if (key) use_default=0;

	if (use_default) return 0;

	if (key > ' ') printf("%c: ", key);
	switch (key) {
case CR:	printf("press CR "); break;
case ESC:printf("press ESC "); break;
default:	printf("press any key "); key=0;
			break;
	}

	while (1) {
		last_key=getch();
		if (last_key == 0) {
			getch(); continue;
		}

		if (!key && (last_key == TAB)) {
			use_default =1;
			break;
		}

		if ((key != ESC) && ((last_key == CTL_C) || (last_key == ESC))) {
			errno=-1; ret=1; break;
		}

		if (!key || (last_key == key)) break;
	}

	printf("\r"); clreol();
	return ret;
}


//------------------------------ Readln --------------------------------------
//
int Readln(char *ln, u_int len)
{
	int	ix;
	char	key;

	ix=0;
	while (1) {
		key=getch();
		if (key >= ' ') {
			if (ix < len) {
				ln[ix++]=key;
				printf("%c", key);
			}
		} else {
			last_key=key;
			if (key == CR) {
				ln[ix]=0; Writeln();
				return 0;
			}

			if ((key == ESC) || (key == CTL_C)) {
				ln[ix]=0; Writeln();
				return (errno=-1);
			}

			if (((key == BS) || (key == DEL)) && ix) { ix--; printf("\b \b"); }
		}
	}
}


//------------------------------ Read_String ---------------------------------
//
char *Read_String(	// neuer String
	char	*def,			// kann NIL sein
	u_int	len)
{
	char	key;
	u_int	ux;

	strbuf[0]=0; ux=0;
	if (def) {
		ux=0;
		while (1) {
			if (!(strbuf[ux]=def[ux])) break;

			ux++;
			if ((ux >= len) || (ux >= sizeof(strbuf))) {
				strbuf[--ux]=0; break;
			}
		}
	}

	printf("%s", strbuf);
	if (use_default) {
		Writeln();
		return strbuf;
	}

	while (1) {
		while (1) {
			key=getch();
			if (   (key >= ' ') || (key == CR) || (key == TAB)
				 || (key == CTL_C) || (key == ESC))
				break;

			if (ux && ((key == BS) || (key == DEL))) break;
		}

		last_key=key;
		if ((key == BS) || (key == DEL)) {
			ux--; printf("\b \b");
		} else {
			if (key >= ' ') {
				if (ux < sizeof(strbuf)-1) {
					strbuf[ux++]=key; printf("%c", key);
				}
			} else {
				Writeln(); strbuf[ux]=0;
				if ((key == CTL_C) || (key == ESC)) {
					if (def)	strcpy(strbuf, def); else strbuf[0]=0;

					errno=-1;
				}

				if (key == TAB) use_default=1;

				return strbuf;
			}
		}
	}
}


/*------------------------------ Read_Num ------------------------------------
--
-- Wert einlesen in hexa (anhaengendes H) odr dezimaler (anhaengender .) Basis einlesen.
-- def  = default Wert
-- max  = maximaler Wert
-- base = default Basis, 0 =dezi, 1 = hexa
--
-- last_key = Abschlusszeichen (cr, ^C, ^Z, ESC), muss global definiert sein.
-- bei ^C und ESC wird errno=-1 gesetzt.
*/
long Read_Num(u_long def, u_long max, u_int base)
{
	u_int		ix, ux, tmp;
	u_long	ret;
	char		buf[16];
	char		cc,cl;

	if (use_default) { Writeln(); return def; }

	if (((long)def != -1) && (def > max)) def=max;

	
	while (1) {
		last_key=0; ix=0;
		do {
			cl=0;
			if (ix) cl=buf[ix-1];

			do {
				cc=toupper(getch());
				if ((cc == CR)  || (cc == CTL_Z) || (cc == CTL_C) ||
					 (cc == ESC) || (cc == TAB)   || (cc == CTL_A) || (cc == CTL_R)) {
					last_key=cc; break;
				}

			} while (
				   (!ix || ((cc != BS) && (cc != DEL))) &&
				   (ix  || (cc != '-')) &&
				   ((ix >= (sizeof(buf)-2)) || (cl == 'H') || (cl == '.') ||
					((cc != '.') && (cc != 'H') && ((cc < '0') || (cc > '9')) &&
					 ((cc < 'A') || (cc > 'F')) )) );

			if ((cc == BS) || (cc == DEL)) { ix--; printf("\b \b"); continue; }

			if (cc >= ' ') { buf[ix++]=cc; printf("%c", cc); }

		} while (!last_key);

		ux=0; cl=0; ret=def;
		if (ix) {
			ret=0; cl=buf[ix-1];
			if (cl == '.') { base=0; cl=1; }
			else {
				if (cl == 'H') { base=1; cl=1; }
				else cl=0;
			}

			if (buf[0] == '-') ux=1;

			if (base) {
				while ((ux+cl) < ix) {
					tmp=buf[ux++]-'0';
					if (tmp >= 10) tmp -=7;
					ret=(ret <<4)+tmp;
				}
			} else {
				while ((ux < ix) && (buf[ux] >= '0') && (buf[ux] <= '9')) {
					ret=(10*ret)+buf[ux++]-'0';
				}
			}

			if (buf[0] == '-') ret=-ret;
		}

		if (((ux+cl) == ix) && (((int)ret == -1) || (ret <= max))) {
			Writeln();
			if (cc == TAB) use_default =1;

			if ((cc == CTL_C) || (cc == ESC)) errno=-1;

			return ret;
		}

		while (ix) { ix--; printf("\b \b"); }
	}
}

/**************************** read_hexa **************************************
--
-- Zahl einlesen mit default Basis hexa.
-- def = default Wert, max = maximaler Wert.
*/
long Read_Hexa(long def, long max)
{
	return Read_Num(def, max, 1);
}

/**************************** read_deci **************************************
--
-- Zahl einlesen mit default Basis deci.
-- def = default Wert, max = maximaler Wert.
*/
long Read_Deci(long def, long max)
{
	return Read_Num(def, max, 0);
}

/*------------------------------ Read_Float ----------------------------------
--
-- last_key = Abschlusszeichen (cr, ^C, ^Z, ESC), muss global definiert sein.
-- bei ^C und ESC wird errno=-1 gesetzt.
*/
float Read_Float(float def)
{
	u_int		ix;
	char		buf[16];
	float		val;
	char		key,
				dpt;		// dezimal point

	if (use_default) { Writeln(); return def; }

	while (1) {
		last_key=0; ix=0; dpt=0;
		while (1) {
			key=toupper(getch());
			if ((key == CR)  || (key == CTL_Z) || (key == CTL_C) ||
				 (key == ESC) || (key == TAB)   || (key == CTL_A)) {
				last_key=key; break;
			}

			if (ix && ((key == BS) || (key == DEL))) {
				ix--; printf("\b \b");
				if (buf[ix] == '.') dpt=0;
				continue;
			}

			if (ix >= (sizeof(buf)-2)) continue;

			if ((!ix  && (key == '-')) ||
				 (!dpt && (key == '.')) ||
				 ((key >= '0') && (key <= '9'))) {
				buf[ix++]=key; printf("%c", key);
				if (key == '.') dpt=1;
			}
		}

		buf[ix]=0; val=def;
		if (ix) sscanf(buf, "%f", &val);

		Writeln();
		if (key == TAB) use_default =1;

		if ((key == CTL_C) || (key == ESC)) errno=-1;

		return val;

//		while (ix) { ix--; printf("\b \b"); }
	}

}

//------------------------------ dump ----------------------------------------
// in:
//  flg   [1..0] => Byte/Word/Long Format
//           [2] => reverse
//           [3] => header
//        [6..4] => Number of address digits
//
void dump(void *buf, u_int lng, u_long offs, u_int flg)
{
	u_int		adg;
	u_long	madg;
	u_int		ix,iy;
	u_char	cx;

	adg=flg >>4;
	if (!adg) adg=4;
	if (adg > 8) adg=8;
	madg=(1 <<(4*adg))-1;

	if (flg &0x08) {
		printf("%*s", adg+2, " ");
		if (flg &0x4) {     // reverse
			iy=16;
			switch (flg &0x3) {

		case 0:
				do {
					iy--;
					printf("  %X", (offs+iy) &0x0F);
					if (!(iy &3)) printf(" ");
				} while (iy);
				break;

		case 1:
				do {
					iy -=2;
					printf("    %X", (offs+iy) &0x0F);
					if (!(iy &7)) printf(" ");
				} while (iy);
				break;

		case 2:
		case 3:
				do {
					iy -=4;
					printf("        %X", (offs+iy) &0x0F);
					if (!(iy &7)) printf(" ");
				} while (iy);
				break;

			}
		} else {
			iy=0;
			switch (flg &0x3) {

		case 0:
				do {
					printf("  %X", (offs+iy) &0x0F);
					iy++;
					if (!(iy &3)) printf(" ");
				} while (iy < 16);
				break;

		case 1:
				do {
					printf("    %X", (offs+iy) &0x0F);
					iy +=2;
					if (!(iy &7)) printf(" ");
				} while (iy < 16);
				break;

		case 2:
		case 3:
				do {
					printf("        %X", (offs+iy) &0x0F);
					iy +=4;
					if (!(iy &7)) printf(" ");
				} while (iy < 16);
				break;

			}
		}

		printf("  0123456789ABCDEF\n");
	}

	ix=0;
	while (ix < lng) {
		if (flg &0x4) {     // reverse
			iy=16;
			switch (flg &0x3) {

		case 0:
				printf("%0*lX: ", adg, (offs+ix+0x0F) &madg);
				while (ix+iy > lng) {
					printf("   "); iy--;
					if (!(iy &3)) printf(" ");
				}

				do {
					iy--; printf(" %02X", ((u_char*)buf)[ix +iy]);
					if (!(iy &3)) printf(" ");
				} while (iy);
				break;

		case 1:
				printf("%0*lX: ", adg, (offs+ix+0x0E) &madg);
				while (ix+iy > lng) {
					printf("     "); iy -=2;
					if (!(iy &7)) printf(" ");
				}

				do {
					iy -=2; printf(" %04X", ((u_short*)buf)[(ix+iy) >>1]);
					if (!(iy &7)) printf(" ");
				} while (iy);
				break;

		case 2:
		case 3:
				printf("%0*lX: ", adg, (offs+ix+0x0C) &madg);
				while (ix+iy > lng) {
					printf("         "); iy -=4;
					if (!(iy &7)) printf(" ");
				}

				do {
					iy -=4; printf(" %08lX", ((u_long*)buf)[(ix+iy) >>2]);
					if (!(iy &7)) printf(" ");
				} while (iy);
				break;

			}
		} else {
			printf("%0*lX: ", adg, (offs+ix) &madg);
			iy=0;
			switch (flg &0x3) {

		case 0:
				do {
					printf(" %02X", ((u_char*)buf)[ix +iy]); iy++;
					if (!(iy &3)) printf(" ");
				} while ((iy != 16) && (ix+iy != lng));

				while (iy != 16) {
					printf("   "); iy++;
					if (!(iy &3)) printf(" ");
				}
				break;

		case 1:
				do {
					printf(" %04X", ((u_short*)buf)[(ix+iy) >>1]); iy +=2;
					if (!(iy &7)) printf(" ");
				} while ((iy != 16) && (ix+iy != lng));

				while (iy != 16) {
					printf("     "); iy +=2;
					if (!(iy &7)) printf(" ");
				}
				break;

		case 2:
		case 3:
				do {
					printf(" %08lX", ((u_long*)buf)[(ix+iy) >>2]); iy +=4;
					if (!(iy &7)) printf(" ");
				} while ((iy != 16) && (ix+iy != lng));

				while (iy != 16) {
					printf("         "); iy +=4;
					if (!(iy &7)) printf(" ");
				}
				break;

			}
		}

		printf("  "); iy=0;
		do {
			cx=((u_char*)buf)[ix]; ix++; iy++;
			if ((cx < 0x20) || (cx >= 0x7F)) cx='.';
			printf("%c", cx);
		} while ((iy != 16) && (ix != lng));

		Writeln();
	}
}


//--------------------------- pci_error --------------------------------------
//
char *pci_error(int err)
{
	switch (err) {

case -1: return("kein PCI BIOS vorhanden");

case -2: return("unbekannte Funktion");

case -3: return("unbekannte PCI BIOS Identifikation");

case FUNCTION_NOT_SUPPORTED:
		return ("PCI function not supported");

case BAD_VENDOR_ID:
		return ("PCI bad vendor ID");

case DEVICE_NOT_FOUND:
		return ("PCI device not found");

case BAD_REGISTER_NUMBER:
		return ("PCI bad register number");

default:
		return ("unknown error");
	}
}
//
//---------------------------- MapRegions ------------------------------------
//
// return -> setting pci_ok
//		-1:	System Error
//		 0:	device found but no regions
//		 1:	IO Region
//		 2:	Memory Region
//
int MapRegions(REGION *reg)
{
	int		ix;
	int		ret;
	u_long	base;
	u_long	len;
	REGION	region;

	if (debug)
		printf("bus no =%d, device no =$%02X\n", reg->bus_no, reg->device_no);

	setmem(&region, sizeof(REGION), 0);
	region.bus_no    =reg->bus_no;
	region.device_no =reg->device_no;

	cnf_acc.bus_no   =reg->bus_no;
	cnf_acc.device_no=reg->device_no;
	cnf_acc._register=0x3C;
	ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return -1;
	}

	region.int_line =(u_char)cnf_acc.value;

	cnf_acc._register=0x08;
	ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return -1;
	}

	region.revision =(u_char)cnf_acc.value;
//
//--- map all regions
//
	for (ix=0; ix < C_REGION; ix++) {
		cnf_acc._register=0x10+(ix <<2);
		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);

		base=cnf_acc.value;
		cnf_acc.value=0xFFFFFFFFL;
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		len=cnf_acc.value;
		region.regs[ix].len=(~(cnf_acc.value &~0x0F))+1;
		cnf_acc.value=base;
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		if (debug) printf("%d: base=0x%08lX, len=0x%08lX\n", ix, base, len);

		if (!base || (base == len) || (base &1)) {
			if ((base &1) && !(base &0xFFFF0000L)) {
				region.regs[ix].base=base;
				region.regs[ix].selio=(u_short)base &~0x0F;
			}
		} else {
			len=region.regs[ix].len;
			if (len > 0x10000L) len =0x10000L;

//			if (debug) printf("%d: base=0x%08lX, len=0x%05lX\n", ix, base, len);

			region.regs[ix].selio=MakeSelector(base &~0x0F, len);
			if (!region.regs[ix].selio) {
				printf("can't make Selector for 0x%08lX\n", base);
				ret=-100; break;
			}

			region.regs[ix].base=base;
		}

		if (ret) break;
	}

	ix =0;
	do region.desc[ix].rg_offs =0; while (++ix < C_DESC);
//
//--- map expanded rom region
//
	while (!ret) {
		cnf_acc._register=0x30;
		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		base=cnf_acc.value;
		region.xrom_base=base;
		cnf_acc.value=0xFFFFFFFFL;
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		len=cnf_acc.value;
		region.xrom_len=(~(len &~0x0F))+1;
		cnf_acc.value=base;
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) break;

		if (debug) printf("x: base=0x%08lX, len=0x%08lX\n", base, len);

		if ((base == len) || !(base &~0x0F) || !(base &0x1)) {
			region.xrom_sel=0; break;
		}

		len=region.xrom_len;
		if (len > 0x10000L) len=0x10000L;

		if (debug) printf("x: base=0x%08lX, len=0x%05lX\n", base, len);
		region.xrom_sel=MakeSelector(base &~0x0F, len);
		if (!region.xrom_sel) {
			printf("can't make Selector for 0x%08lX\n", base);
			ret=-100;
		}

		break;
	}


	if (ret) {
		if (ret != -100) printf("%s\n", pci_error(ret));

		return -1;
	}

	if (debug) {
		for (ix=0; ix < C_REGION; ix++) {
			printf("region %d, 0x%04X/0x%08lX\n",
					 ix, region.regs[ix].selio, region.regs[ix].len);
		}

		WaitKey(0);
	}

	if (!region.regs[0].selio) {
		printf("there is no main region, the found regions are:\n");
		for (ix=0; ix < C_REGION; ix++)
			printf("region %d, 0x%04X/0x%08lX\n",
					 ix, region.regs[ix].selio, region.regs[ix].len);
		WaitKey(0);
		errno=-1;
		return 0;
	}

	ret=0;
	if ((region.r0_regs=pci_ptr(&region, 0, 0)) != 0) ret=2;
	else {
		if ((region.r0_ioport=region.regs[0].selio) != 0) ret=1;
	}

	if (ret) *reg=region;

	return ret;
}


//---------------------------- pci_ptr ---------------------------------------
//
void *pci_ptr(
	REGION	*reg,		// region descriptor
	u_short	regnr,	// region number, C_REGION => expanded ROM selector
	u_long	offs)		// offset in region
{
	u_int	ix;
	u_int	sel;
	void	*ptr;

	if (!reg) reg=&config.region;

	if (debug) printf("pci_ptr(%d, 0x%08lX);\n", regnr, offs);

	if (regnr == C_REGION) {	// expanded ROM selector
		if (!reg->xrom_sel || (offs+3 >= reg->xrom_len)) ptr=0;
		else ptr=MAKELP(reg->xrom_sel, (u_short)offs);

		if (debug) printf("=%08lX\n", ptr);
		return ptr;
	}

	if (   (regnr >= C_REGION) || !(reg->regs[regnr].selio &7)
		 || (offs+3 >= reg->regs[regnr].len)) return 0;

	sel =(u_int)(offs >>16);
	if (!sel) {
		ptr=MAKELP(reg->regs[regnr].selio, (u_short)offs);
		if (debug) printf("=%08lX\n", ptr);
		return ptr;
	}

	sel |=(regnr <<13); ix=0;
	while (reg->desc[ix].rg_offs) {
		if (reg->desc[ix].rg_offs == sel) {
			ptr=MAKELP(reg->desc[ix].sel, (u_short)offs);
			if (debug) printf("=%08lX\n", ptr);
			return ptr;
		}

		if (++ix >= C_DESC) {
			printf("selector table is full\n");
			return 0;
		}
	}

	reg->desc[ix].sel=MakeSelector((reg->regs[regnr].base &~0x0F) |(offs &~0xFFFFL), 0x10000L);
		
	if (!reg->desc[ix].sel) return 0;

	reg->desc[ix].rg_offs=sel;
	ptr=MAKELP(reg->desc[ix].sel, (u_short)offs);
	if (debug) printf("=%08lX\n", ptr);
	return ptr;
}
//
//---------------------------- pci_attach -----------------------------------
//
int pci_attach(		// 0: ok
							// 1: Fehler gemeldet
							// 0x8. PCI bios error
	REGION	*reg,
	int		devid,	// device id
	int		onl)		// T: keine Alternative zu config.*_id
{
	int	ret;

	if (!reg) reg=&config.region;

	pci_ok=0;
	if ((ret=pci_bios(PCI_BIOS_PRESENT, &bios_psnt)) != 0) {
		printf("kein PCI BIOS vorhanden ret=0x%04X\n", ret);
		return 1;
	}

	if (bios_psnt.present || (bios_psnt.signature != LIT2LONG('PC','I '))) {
		printf("invallid response from PCI_BIOS_PRESENT call\n");
		return 1;
	}

	pdev.device_id=config.device_id;
	pdev.vendor_id=config.vendor_id;
	pdev.index    =config.c_index;
	if (((ret=pci_bios(FIND_PCI_DEVICE, &pdev)) != 0) && !onl) {
		if (pdev.index) {
			pdev.index     =0;
			if (!(ret=pci_bios(FIND_PCI_DEVICE, &pdev))) {
				printf("Device found with Index 0!\n");
			}
		}

		if (ret) {
			pdev.vendor_id =AMCC_VENDOR_ID;
			if (config.device_id == AMCC_DEVICE_ID)
				pdev.device_id=devid;
			else
				pdev.device_id=AMCC_DEVICE_ID;

			if (!(ret=pci_bios(FIND_PCI_DEVICE, &pdev))) {
				if (pdev.device_id == AMCC_DEVICE_ID)
					printf("Vendor and Device ID from AMCC found!\n");
				else
					printf("correct Device ID with Index 0 found\n");
			}
		}

		if (!ret) {
			WaitKey(ESC);
			config.device_id     =pdev.device_id;
			config.vendor_id     =pdev.vendor_id;
			config.c_index       =pdev.index;
		}
	}

	if (ret) {
		if (onl) return ret;

		printf("%s\n", pci_error(ret));
		WaitKey(ESC);
		return (ret != DEVICE_NOT_FOUND);
	}

	config.cnfs_bus_no    =pdev.bus_no;
	config.cnfs_device_no =pdev.device_no >>3;
	reg->bus_no   =pdev.bus_no;
	reg->device_no=pdev.device_no;
	return ((pci_ok=(char)MapRegions(reg)) == -1);
}

//---------------------------- WRTamcc ---------------------------------------
//
void WRTamcc(u_int offs, u_long val)
{
	if (pci_ok == 2) {
		((u_long*)config.region.r0_regs)[offs >>2]=val;
		return;
	}

	if (pci_ok != 1) return;

	WPortL(config.region.r0_ioport +offs, val);
}


//---------------------------- RDamcc ----------------------------------------
//
u_long RDamcc(u_int offs)
{
	if (pci_ok == 2) {
		return ((u_long*)config.region.r0_regs)[offs >>2];
	}

	if (pci_ok != 1) return 0;

	return RPortL(config.region.r0_ioport +offs);
}


