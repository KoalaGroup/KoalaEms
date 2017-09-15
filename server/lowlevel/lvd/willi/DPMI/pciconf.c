// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	pciconf.c -- PCI Configuration Library */

#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include "pci.h"

extern PCI_CONFIG	config;

typedef struct {
	u_short	vendor,				//00
				device,
				command,
				status;
	u_char	revision,			//08
				class_reg,
				class_sub,
				class_base;
	u_char	cache_line,			//0C
				latency,
				header,
				bist;
	u_long	base_addr[6],		//10
				res_0[2],
				rom_base,			//30
				res_1[2];
	u_char	int_line,			//3C
				int_pin,
				min_gnt,
				max_lat;
} DEV_CONFIG;

static DEV_CONFIG		dev_config;

#ifdef AMCC
#define _VENDOR_ID	AMCC_VENDOR_ID
#define _DEVICE_ID	AMCC_DEVICE_ID
static u_char			buffer[0x1000];
#else
 #ifdef PLX
 #define _VENDOR_ID	PLX_VENDOR_ID
 #define _DEVICE_ID	PLX_DEVICE_ID
 #else
 #define _VENDOR_ID	0
 #define _DEVICE_ID	0
 #endif
#endif

static u_long	breg[6];		// base register, length info
static u_long	rreg;

//============================================================================
//                            Setup/Configuration menu                       =
//============================================================================
//
//--------------------------- err_logging ------------------------------------
//
static int err_logging(void)
{
	printf("Error Logging %4d ", config.errlog);
	config.errlog=(u_short)Read_Deci(config.errlog, -1);
	return 0;
}


//--------------------------- set_parameter ----------------------------------
//
static int set_parameter(
	u_int	venid,
	u_int	devid)
{
	int		ret;
	u_short	v_id;
	u_short	d_id;
	u_short	indx;

	v_id=config.vendor_id;
	d_id=config.device_id;
	indx=config.c_index;
	gotoxy(1, 4);
	printf("vendor ID 0x%04X      (%04X/%04X)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
			 v_id, venid, _VENDOR_ID);
	v_id=(u_short)Read_Hexa(v_id, -1);
	if (errno) return -1;

	printf("device ID 0x%04X      (%04X/%04X)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
			 d_id, devid, _DEVICE_ID);
	d_id=(u_short)Read_Hexa(d_id, -1);
	if (errno) return -1;

	printf("Index       %4d ", indx);
	indx=(u_short)Read_Deci(indx, -1);
	if (errno) return -1;

	pdev.device_id=d_id;
	pdev.vendor_id=v_id;
	pdev.index=indx;
	ret=pci_bios(FIND_PCI_DEVICE, &pdev);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return 1;
	}

	Writeln();
	printf("Bus Number      =%d\n", pdev.bus_no);
	printf("Dev Number      =%d,  function number =%d\n",
			 pdev.device_no >>3, pdev.device_no &0x07);

	cnf_acc.bus_no   =pdev.bus_no;
	cnf_acc.device_no=pdev.device_no;
	cnf_acc._register=0x3C;
	ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return 1;
	}

	printf("Interrupt Line  =%d\n", (u_char)cnf_acc.value);
	config.device_id=pdev.device_id;
	config.vendor_id=pdev.vendor_id;
	config.c_index  =pdev.index;

	config.region.bus_no   =pdev.bus_no;
	config.region.device_no=pdev.device_no;
	return ((pci_ok=(char)MapRegions(&config.region)) == -1);
}
//
//
//--------------------------- read_conf_space --------------------------------
//
static int read_conf_space(int other)
{

	u_int		ux;
	int		ret;

	if (other) {
		printf("PCI bus %2d ", config.cnfs_bus_no);
		config.cnfs_bus_no=(u_short)Read_Deci(config.cnfs_bus_no, -1);
		if (errno) return -1;

		printf("device  %2d ", config.cnfs_device_no);
		config.cnfs_device_no=(u_short)Read_Deci(config.cnfs_device_no, 0x1F);
		if (errno) return -1;

		cnf_acc.bus_no   =config.cnfs_bus_no;
		cnf_acc.device_no=config.cnfs_device_no <<3;
	} else {
		cnf_acc.bus_no   =config.region.bus_no;
		cnf_acc.device_no=config.region.device_no;
	}

	printf("bus %u, device %u\n", cnf_acc.bus_no, cnf_acc.device_no >>3);
	ux=0;
	do {
		cnf_acc._register=ux <<2;
		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		((u_long*)&dev_config)[ux]=cnf_acc.value;
	} while (++ux < (sizeof(DEV_CONFIG)/4));

	ux=0;
	do {
		cnf_acc._register=0x10+(ux <<2);
		cnf_acc.value=0xFFFFFFFFL;
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		breg[ux]=cnf_acc.value;
		cnf_acc.value=dev_config.base_addr[ux];
		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		ux++;
	} while (ux < 6);

	cnf_acc._register=0x30;
	cnf_acc.value=0xFFFFFFFFL;
	ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	rreg=cnf_acc.value;
	cnf_acc.value=dev_config.rom_base;
	ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	Writeln();
	printf("Vendor   ID     %04X    Command  %04X    Class base  %02X\n",
			 dev_config.vendor, dev_config.command, dev_config.class_base);
	printf("Device   ID     %04X    Status   %04X    Class sub   %02X\n",
			 dev_config.device, dev_config.status, dev_config.class_sub);
	printf("Revision ID       %02X                     Class reg   %02X\n",
			 dev_config.revision, dev_config.class_reg);
	printf("Base Reg  0 %08lX -> %08lX         Cache Line  %02X    Inter Line %02X\n",
			 breg[0], dev_config.base_addr[0], dev_config.cache_line, dev_config.int_line);
	printf("Base Reg  1 %08lX -> %08lX         Latency     %02X    Inter Pin  %02X\n",
			 breg[1], dev_config.base_addr[1], dev_config.latency, dev_config.int_pin);
	printf("Base Reg  2 %08lX -> %08lX         Header Type %02X    Min Gnt    %02X\n",
			 breg[2], dev_config.base_addr[2], dev_config.header, dev_config.min_gnt);
	printf("Base Reg  3 %08lX -> %08lX         BIST        %02X    Max Lat    %02X\n",
			 breg[3], dev_config.base_addr[3], dev_config.bist, dev_config.max_lat);
	printf("Base Reg  4 %08lX -> %08lX\n",
			 breg[4], dev_config.base_addr[4]);
	printf("Base Reg  5 %08lX -> %08lX         ROM Base   %08lX -> %08lX\n",
			 breg[5], dev_config.base_addr[5], rreg, dev_config.rom_base);
	Writeln();
	return 0;
}

//--------------------------- free_configure ---------------------------------
//
static int free_configure(void)
{
	int		ret;
	u_int		ux;
	u_int		ix;
	u_int		int_line;
	u_long	base;
	u_char	rst;
	u_long	stcmd;
	u_char	ok;

	if ((ret=read_conf_space(pci_ok == 0)) != 0) return ret;

	rst=0; ok=pci_ok; pci_ok=0;
	if (ok &&
		 (dev_config.revision == config.region.revision)) {
		ix=0;
		do {
			cnf_acc._register=0x10+(ix <<2);
			ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
			if (ret) break;

			base=cnf_acc.value;
			if (base == 0) {
				if (config.region.regs[ix].base &0x1) break;
			} else {
				if ((base != 1) ||
					 !(config.region.regs[ix].base &0x1)) break;
			}

			cnf_acc.value=0xFFFFFFFFL;
			ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
			if (ret) break;

			ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
			if (ret) break;

			if (config.region.regs[ix].len != (~(cnf_acc.value &~0x0F))+1) break;
			cnf_acc.value=config.region.regs[ix].base;
			ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
			if (ret) break;

		} while (++ix < C_REGION);

		if (ix == C_REGION) rst=1;
	}

	int_line=config.region.int_line; stcmd=0xF0000007L;
	if (!rst) {
		int_line=0xFF;
		ux=0;
		do {
			if (!breg[ux]) { config.region.regs[ux].base=0; continue; }

			if (breg[ux] &0x1) {
				printf("Base Addr IO  %u: 0x%08lX ",
						 ux, config.region.regs[ux].base);
				config.region.regs[ux].base=
											Read_Hexa(config.region.regs[ux].base, -1);
				if (errno) return -1;

				continue;
			}

			printf("Base Addr MEM %u: 0x%08lX ",
					 ux, config.region.regs[ux].base);
			config.region.regs[ux].base=
											Read_Hexa(config.region.regs[ux].base, -1);
			if (errno) return -1;

		} while (++ux < C_REGION);

		if ((rreg &~0x7FF) && (rreg &0x1)) {
			printf("EROM Addr Reg  : 0x%08lX ", config.region.xrom_base);
			config.region.xrom_base=Read_Hexa(config.region.xrom_base, -1);
			if (errno) return -1;
		} else config.region.xrom_base=0;

		printf("Status/Command   0x%08lX ", stcmd);
		stcmd=Read_Hexa(stcmd, -1);
		if (errno) return -1;

		ux=0;
		do {
			if (breg[ux]) {
				cnf_acc._register=0x10+(ux <<2);
				cnf_acc.value    =config.region.regs[ux].base;
				ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
				if (ret) { printf("%s\n", pci_error(ret)); return 1; }
			}
		} while (++ux < 6);

		cnf_acc._register=0x30;
		if (config.region.xrom_base)
			cnf_acc.value =config.region.xrom_base |0x1;
		else cnf_acc.value    =0x00000000;

		ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }
	}

	cnf_acc._register=0x0C;				// master latency, cache line
	cnf_acc.value    =0x4008;
	ret=pci_bios(WRITE_CONFIG_WORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	cnf_acc._register=0x3C;				// Interrupt Line
	cnf_acc.value    =int_line;
	ret=pci_bios(WRITE_CONFIG_BYTE, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	cnf_acc._register=0x04;				// PCI Status / Command
	cnf_acc.value    =stcmd;
	ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	if (rst) return (pci_ok=ok);

	config.device_id=dev_config.device;
	config.vendor_id=dev_config.vendor;
	config.c_index  =0;

	config.region.bus_no   =cnf_acc.bus_no;
	config.region.device_no=cnf_acc.device_no;
	return ((pci_ok=(char)MapRegions(&config.region)) == -1);
}
//
//
//--------------------------- dsp_configuration ------------------------------
//
static int dsp_configuration(int other)
{

	u_int		ux,uy;
	int		ret;

	if ((ret=read_conf_space(other)) != 0) return ret;

	if (WaitKey(0)) return 0;

	Writeln();
	printf("           0x00      0x40      0x80      0xC0\n");
	ux=0;
	do {
		printf("0x%02X:", ux); uy=0;
		do {
			cnf_acc._register=ux+uy;
			ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
			if (ret) { printf("%s\n", pci_error(ret)); return 1; }

			printf("  %08lX", cnf_acc.value);
			uy +=0x40;
		} while (uy < 0x100);

		Writeln();
		ux +=4;
	} while (ux < 0x40);

	return 1;
}

//--------------------------- temp_configure ---------------------------------
//
static int temp_configure(void)
{
	int		ret;
	u_short	lat;

	cnf_acc.bus_no   =config.region.bus_no;
	cnf_acc.device_no=config.region.device_no;
	cnf_acc._register=0x0C;				// master latency, cache line
	ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return 1;
	}

	lat=(u_short)cnf_acc.value;
	printf("latency/cache line 0x%04X ", lat);
	lat=(u_short)Read_Hexa(lat, -1);
	if (errno) return -1;

	cnf_acc._register=0x0C;				// master latency, cache line
	cnf_acc.value    =lat;
	ret=pci_bios(WRITE_CONFIG_WORD, &cnf_acc);
	if (ret) { printf("%s\n", pci_error(ret)); return 1; }

	return 1;
}

#ifdef AMCC
//--------------------------- read_eeprom ------------------------------------
//
static int read_eeprom(u_int loc)
{
	u_long	tmo;
	long		ret;

	WRTamcc(MCSR, NV_LAB+(((u_long)loc &0xFF) <<16));
	WRTamcc(MCSR, NV_HAB+(((u_long)loc &0xFF00) <<8));
	WRTamcc(MCSR, NV_RD);

	tmo=4*config.ms_count;
	do {
	  ret=RDamcc(MCSR);
	  tmo--;
	} while (tmo && (ret < 0));

	if (ret < 0) {
	  Writeln();
	  printf("not ready\n");
	  return -1;
	}

	return (int)(ret >>16) &0xFF;
}


//--------------------------- write_eeprom -----------------------------------
//
static int write_eeprom(u_int loc, u_int value)
{
	u_long	tmo;
	u_int		tmp;
	long		ret;

	value=value &0xFF;
	WRTamcc(MCSR, NV_LAB+(((u_long)loc &0xFF) <<16));
	WRTamcc(MCSR, NV_HAB+(((u_long)loc &0xFF00) <<8));
	WRTamcc(MCSR, NV_WR +((u_long)value <<16));
	tmo=20*config.ms_count;
	do {
		ret=RDamcc(MCSR);
		tmo--;
	} while (tmo && (ret < 0));

	if (ret < 0) {
		Writeln();
		printf("not ready\n");
		return 1;
	}

	if (value == 0xFF) delay(50);  // minimum 20ms

	tmo=20*config.ms_count;
	do {
		tmp=read_eeprom(loc);
		if (tmp == value) return 0;

		tmo--;
	} while (tmo);

	Writeln();
	printf("write failed\n");
	return 1;
}
//
//--------------------------- dsp_eeprom -------------------------------------
//
static int dsp_eeprom(void)
{
	int	ret;
	u_int	ux, uy;
	char	sum;

	Writeln();
	ux=0; sum=0;
	do {
		uy=0;
		do {
			ret=read_eeprom(ux+uy);
			if (ret == -1) return 1;

			buffer[uy]=ret;
			uy++;
			sum=sum+ret;
		} while (uy < 0x100);

		dump(buffer, 0x100, ux, 0x0C);  // reverse mit header
		ux +=0x100;
		if (ux == 0x200) {
			Writeln();
			if (!sum) {
				printf("checksum ok\n");
			} else {
				printf("wrong checksum, exp checkbyte[0x1ff] =0x%02X\n",
						  buffer[0xFF]-sum);
			}
		}

		if (WaitKey(0)) return 0;

		Writeln();
	} while (ux < 0x400);

	return 0;
}
//
//--------------------------- verify_eeprom ----------------------------------
//
static int verify_eeprom(void)
{
	char	wrk[32];
	char	tk0[32];
	char	tk1[32];
	int	ix;
	u_int	loc;
	int	tmp;
	u_int	new;

	Writeln();
	printf("@123   = new location is 0x123\n");
	printf("+3     = increment location by 3\n");
	printf("-9     = decrement locatiin by 9\n");
	Writeln();
	loc=0;
	while (1) {
		if ((tmp=read_eeprom(loc)) == -1) return 1;

		printf("0x%03X: %02X ", loc, tmp);
		strcpy(wrk, Read_String("", sizeof(wrk)));
		if (errno) return 0;

		str2up(wrk); ix=0;
		strcpy(tk0, token(wrk, &ix));
		strcpy(tk1, token(wrk, &ix));
		if (tk0[0] && (tk0[0] != '@') && (tk0[0] != '+') && (tk0[0] != '-')) {
			ix=0;
			new=(u_int)value(tk0, &ix, 16);
			if (ix < strlen(tk0)) printf("Fehlerhafte Eingabe!\n");
			else {
				if (new != tmp) {
					if (write_eeprom(loc, new)) return 1;
				}
			}

			strcpy(tk0, tk1);
		}

		if (!tk0[0] || ((tk0[0] != '@') && (tk0[0] != '+') && (tk0[0] != '-')))
			loc++;
		else {
			ix=1; new=(u_int)value(tk0, &ix, 16);
			switch (tk0[0]) {

		case '@':
				loc=new; break;

		case '+':
				loc +=new; break;

		case '-':
				loc -=new;

			}
		}

		if (loc >= 0x400) loc=0;
	}
}
//
//--------------------------- save_eeprom ------------------------------------
//
static int save_eeprom(void)
{
	int	ret;
	u_int	loc;
	int	hx;

	printf("\nsave EEPROM image to a file\n\n");
	printf("file name ");
	strcpy(config.save_file,
			 Read_String(config.save_file, sizeof(config.save_file)));
	if (errno) return 0;

	if ((hx=_rtl_creat(config.save_file, 0)) == -1) {
		perror("error creating file");
		return 1;
	}

	loc=0;
	do {
		ret=read_eeprom(loc);
		if (ret == -1) {
			close(hx);
			return 1;
		}

		buffer[loc++]=ret;
	} while (loc < 0x400);

	if (write(hx, buffer, 0x400) != 0x400) {
		perror("error writing file");
		close(hx);
		return 1;
	}

	if (close(hx) == -1) {
		perror("error closing file");
		close(hx);
		return 1;
	}

	return 0;
}
//
//--------------------------- load_eeprom ------------------------------------
//
static int load_eeprom(void)
{
	int	ret;
	u_int	loc;
	int	hx;
	u_int	lx;

	printf("\nload EEPROM image from a file\n\n");
	printf("file name ");
	strcpy(config.load_file,
			 Read_String(config.load_file, sizeof(config.load_file)));
	if (errno) return 0;

	if ((hx=_rtl_open(config.load_file, O_RDONLY)) == -1) {
		perror("error opening file");
		return 1;
	}

	lx=read(hx, buffer, 0x410);
	close(hx);
	if (lx <= 0) {
		printf("file empty\n");
		return 1;
	}

	if (lx > 0x400) {
		printf("file to large (maximum is 0x400/1024)\n");
		return 1;
	}

	printf("read EEPROM\n");
	loc=0;
	do {
		ret=read_eeprom(loc);
		if (ret == -1) return 1;

		buffer[0x400+loc]=ret;
		loc++;
	} while (loc < 0x400);

	printf("write EEPROM\n");
	loc=0;
	do {
		if (buffer[loc] != buffer[0x400+loc]) {
			if (write_eeprom(loc, buffer[loc])) return 1;
		}

		loc++;
		if (!(loc &0xF)) printf("\r%4d ", loc);

	} while (loc < lx);

	printf("\r%4d\n", loc);
	printf("verify EEPROM\n");
	loc=0;
	do {
		ret=read_eeprom(loc);
		if (ret == -1) return 1;

		if (ret != buffer[loc]) {
			printf("data error, exp:0x%02X, is:0x%02\n", buffer[loc], ret);
			return 1;
		}

		loc++;
	} while (loc < lx);

	return 1;
}
#endif

//--------------------------- Konfigurierung ---------------------------------
//
int Konfigurierung(
	u_int	venid,
	u_int devid)
{
#define SP_		20
#define ZL_		5

	int	setpar=0;
	u_int	ux,uy;
	char	key;
	int	ret;

	while (1) {
		clrscr();
		ux=0;
		gotoxy(SP_, ux+++ZL_);
						printf("Error Logging ...................0");
		gotoxy(SP_, ux+++ZL_);
						printf("PCI Parameter ...................1");
		gotoxy(SP_, ux+++ZL_);
						printf("show Configuration Space Header .2");
		gotoxy(SP_, ux+++ZL_);
						printf("show other Config Space Header ..3");
		gotoxy(SP_, ux+++ZL_);
						printf("restore / configure PCI device ..4");
		gotoxy(SP_, ux+++ZL_);
						printf("PCI configuration ...............5");
#ifdef AMCC
		gotoxy(SP_, ux+++ZL_);
						printf("zeige EEPROM ....................6");
		gotoxy(SP_, ux+++ZL_);
						printf("verify EEPROM ...................7");
		gotoxy(SP_, ux+++ZL_);
						printf("save EEPROM to file .............8");
		gotoxy(SP_, ux+++ZL_);
						printf("load EEPROM from file ...........9");
#endif
		gotoxy(SP_, ux+++ZL_);
						printf("Ende ..........................ESC");
		gotoxy(SP_, ux+++ZL_);
						printf("                                 %c ",
								 config.cnf_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			Writeln();
			return setpar;
		}

		use_default=(key == TAB);
		if ((key == TAB) || (key == CR)) key=config.cnf_menu;

		uy=key-'0';
		if (uy > 9) uy=uy-7;

		clrscr(); gotoxy(1, 10);
		config.cnf_menu=key; errno=0; ret=0;
		while (1) {
			switch (uy) {

		case 0:
				ret=err_logging(); break;

		case 1:
				ret=set_parameter(venid, devid); setpar=1; break;

		case 2:
				if (pci_ok) ret=dsp_configuration(0);
				break;

		case 3:
				ret=dsp_configuration(1); break;

		case 4:
				ret=free_configure(); break;

		case 5:
				ret=temp_configure(); break;

#ifdef AMCC
		case 6:
				if (pci_ok) ret=dsp_eeprom();
				break;

		case 7:
				if (pci_ok) ret=verify_eeprom();
				break;

		case 8:
				if (pci_ok) ret=save_eeprom();
				break;

		case 9:
				if (pci_ok) ret=load_eeprom();
				break;
#endif
			}

			if (ret <= 0) break;

			WaitKey(1);
			if (last_key != TAB) break;

			Writeln();
		}
	}
}

