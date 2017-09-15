// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	pcibios.c -- Library for PCI Overview */

#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include "pci.h"

extern PCI_CONFIG	config;

static PCI_CLASS	class;

//============================================================================
//                            PCI BIOS menu                                  =
//============================================================================
//
typedef struct {
	u_char	class[4];		// class[3] is not used
	char		*txt;
} CLASS_TXT;

static CLASS_TXT const class_txt[] = {
	{{ 0x00, 0x00, 0x00, 0x00 }, "Device other then VGA" },
	{{ 0x00, 0x01, 0x00, 0x00 }, "VGA compatible device" },

	{{ 0x01, 0x00, 0x00, 0x00 }, "SCSI controller" },
	{{ 0x01, 0x01, 0xFF, 0x00 }, "IDE controller" },
	{{ 0x01, 0x02, 0x00, 0x00 }, "Floppy disk controller" },
	{{ 0x01, 0x03, 0x00, 0x00 }, "IPI controller" },
	{{ 0x01, 0x04, 0x00, 0x00 }, "RAID controller" },
	{{ 0x01, 0x80, 0x00, 0x00 }, "unknown mass storage controller" },

	{{ 0x02, 0x00, 0x00, 0x00 }, "Ethernet controller" },
	{{ 0x02, 0x01, 0x00, 0x00 }, "Token ring controller" },
	{{ 0x02, 0x02, 0x00, 0x00 }, "FDDI controller" },
	{{ 0x02, 0x03, 0x00, 0x00 }, "ATM controller" },
	{{ 0x02, 0x80, 0x00, 0x00 }, "unknown network controller" },

	{{ 0x03, 0x00, 0x00, 0x00 }, "VGA compatible controller" },
	{{ 0x03, 0x00, 0x01, 0x00 }, "8514 compatible controller" },
	{{ 0x03, 0x01, 0x00, 0x00 }, "XGA controller" },
	{{ 0x03, 0x80, 0x00, 0x00 }, "unknown display controller" },

	{{ 0x04, 0x00, 0x00, 0x00 }, "Video device" },
	{{ 0x04, 0x01, 0x00, 0x00 }, "Audio device" },
	{{ 0x04, 0x80, 0x00, 0x00 }, "unknown multimedia device" },

	{{ 0x05, 0x00, 0x00, 0x00 }, "RAM memory controller" },
	{{ 0x05, 0x01, 0x00, 0x00 }, "Flash memory controller" },
	{{ 0x05, 0x80, 0x00, 0x00 }, "unknown memory controller" },

	{{ 0x06, 0x00, 0x00, 0x00 }, "Host/PCI bridge" },
	{{ 0x06, 0x01, 0x00, 0x00 }, "PCI/ISA bridge" },
	{{ 0x06, 0x02, 0x00, 0x00 }, "PCI/EISA bridge" },
	{{ 0x06, 0x03, 0x00, 0x00 }, "PCI/Micro Channel bridge" },
	{{ 0x06, 0x04, 0x00, 0x00 }, "PCI/PCI bridge" },
	{{ 0x06, 0x05, 0x00, 0x00 }, "PCI/PCMCIA bridge" },
	{{ 0x06, 0x06, 0x00, 0x00 }, "NuBus bridge" },
	{{ 0x06, 0x07, 0x00, 0x00 }, "CardBus bridge" },
	{{ 0x06, 0x80, 0x00, 0x00 }, "unknown bridge type" },

	{{ 0x07, 0x00, 0x00, 0x00 }, "Generic XT compatible serial controller" },
	{{ 0x07, 0x00, 0x01, 0x00 }, "16450 compatible serial controller" },
	{{ 0x07, 0x00, 0x02, 0x00 }, "16550 compatible serial controller" },
	{{ 0x07, 0x01, 0x00, 0x00 }, "Parallel port" },
	{{ 0x07, 0x01, 0x01, 0x00 }, "Bidirectional parallel port" },
	{{ 0x07, 0x01, 0x02, 0x00 }, "ECP 1.X compliant parallel port" },
	{{ 0x07, 0x80, 0x00, 0x00 }, "unknown communication device" },

	{{ 0x08, 0x00, 0x00, 0x00 }, "Generic 8259 PIC" },
	{{ 0x08, 0x00, 0x01, 0x00 }, "ISA PIC" },
	{{ 0x08, 0x00, 0x02, 0x00 }, "EISA PIC" },
	{{ 0x08, 0x01, 0x00, 0x00 }, "Generic 8237 DMA controller" },
	{{ 0x08, 0x01, 0x01, 0x00 }, "ISA DMA controller" },
	{{ 0x08, 0x01, 0x02, 0x00 }, "EISA DMA controller" },
	{{ 0x08, 0x02, 0x00, 0x00 }, "Generic 8254 system timer" },
	{{ 0x08, 0x02, 0x01, 0x00 }, "ISA system timer" },
	{{ 0x08, 0x02, 0x02, 0x00 }, "EISA system timers (2 timers)" },
	{{ 0x08, 0x03, 0x00, 0x00 }, "Generic RTC controller" },
	{{ 0x08, 0x03, 0x01, 0x00 }, "ISA RTC controller" },
	{{ 0x08, 0x80, 0x00, 0x00 }, "unknown system peripheral" },

	{{ 0x09, 0x00, 0x00, 0x00 }, "Keyboard controller" },
	{{ 0x09, 0x01, 0x00, 0x00 }, "Digitizer (PEN)" },
	{{ 0x09, 0x02, 0x00, 0x00 }, "Mouse controller" },
	{{ 0x09, 0x80, 0x00, 0x00 }, "unknown input controller" },

	{{ 0x0A, 0x00, 0x00, 0x00 }, "Generic docking station" },
	{{ 0x0A, 0x80, 0x00, 0x00 }, "unknown type of docking station" },

	{{ 0x0B, 0x00, 0x00, 0x00 }, "386" },
	{{ 0x0B, 0x01, 0x00, 0x00 }, "486" },
	{{ 0x0B, 0x02, 0x00, 0x00 }, "Pentium" },
	{{ 0x0B, 0x10, 0x00, 0x00 }, "Alpha" },
	{{ 0x0B, 0x40, 0x00, 0x00 }, "Co-processor" },

	{{ 0x0C, 0x00, 0x00, 0x00 }, "Firewire (IEEE 1394)" },
	{{ 0x0C, 0x01, 0x00, 0x00 }, "ACCESS.bus" },
	{{ 0x0C, 0x02, 0x00, 0x00 }, "SSA" },

	{{ 0x00, 0x00, 0x00, 0x00 }, 0 }};

#define C_CLASS_TXT		(sizeof(class_txt)/sizeof(CLASS_TXT))

//-------------------------- _show_pci_device -------------------------------
//
int _show_pci_device(void)
{
	u_int		ux,uy;
	u_int		vnd,dev;
	int		ret;
	u_char	bcls,scls,
				ifcls;		// Prog I/F

	printf("PCI bus %4d ", config.bmen_bus_no);
	config.bmen_bus_no=(u_short)Read_Deci(config.bmen_bus_no, -1);
	if (errno) return 1;

	printf("\nno vendor device base sub if\n");
	printf(  "       ID     ID  ---class--\n");
	printf(  "----------------------------------------------\n");

	cnf_acc.bus_no   =config.bmen_bus_no;
	ux=0;
	do {
		cnf_acc.device_no=ux <<3;
		cnf_acc._register=0;
		ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		if (cnf_acc.value != 0xFFFFFFFFL) {
			vnd=(u_int)cnf_acc.value; dev=(u_int)(cnf_acc.value >>16);

			cnf_acc._register=8;
			ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
			if (ret) { printf("%s\n", pci_error(ret)); return 1; }

			ifcls=(u_int)(cnf_acc.value >>8);
			scls =(u_int)(cnf_acc.value >>16);
			bcls =(u_int)(cnf_acc.value >>24);
			uy=0;
			while ((uy < C_CLASS_TXT) &&
					 ((class_txt[uy].class[0] != bcls) ||
					  (class_txt[uy].class[1] != scls) ||
						((class_txt[uy].class[2] != 0xFF) &&
						(class_txt[uy].class[2] != ifcls)))) uy++;

			printf("%2d   %04X   %04X   %02X  %02X %02X",
					 ux, vnd, dev, bcls, scls, ifcls);
			if (uy < C_CLASS_TXT) printf("  %s", class_txt[uy].txt);

			Writeln();
		}

		ux++;
	} while (ux < 32);

	return 1;
}

//-------------------------- _find_pci_device --------------------------------
//
int _find_pci_device(void)
{
	int	ret;

	printf("device ID $%04X ", config.bmen_device_id);
	config.bmen_device_id=(u_short)Read_Hexa(config.bmen_device_id, -1);
	if (errno) return 0;

	printf("vendor ID $%04X ", config.bmen_vendor_id);
	config.bmen_vendor_id=(u_short)Read_Hexa(config.bmen_vendor_id, -1);
	if (errno) return 0;

	printf("Index      %4d ", config.bmen_index);
	config.bmen_index=(u_short)Read_Deci(config.bmen_index, -1);
	if (errno) return 0;

	pdev.device_id=config.bmen_device_id;
	pdev.vendor_id=config.bmen_vendor_id;
	pdev.index    =config.bmen_index;
	ret=pci_bios(FIND_PCI_DEVICE, &pdev);

	Writeln();
	printf("Bus Number =%d\n", pdev.bus_no);
	printf("Dev Number =%d,  function number =%d\n",
			 pdev.device_no >>3, pdev.device_no &0x07);
	if (ret) {
		printf("%s\n", pci_error(ret));
		return 1;
	}

	return 1;
}

//--------------------------- _find_class ------------------------------------
//
int _find_class(void)
{
	int	ret;

	printf("base class $%02X ", config.bmen_bcls);
	config.bmen_bcls=(u_short)Read_Hexa(config.bmen_bcls, 0xFF);
	if (errno) return 0;

	printf("sub class  $%02X ", config.bmen_scls);
	config.bmen_scls=(u_short)Read_Hexa(config.bmen_scls, 0xFF);
	if (errno) return 0;

	printf("interface  $%02X ", config.bmen_ifcls);
	config.bmen_ifcls=(u_short)Read_Hexa(config.bmen_ifcls, 0xFF);
	if (errno) return 0;

	printf("Index       %2d ", class.index);
	class.index=(u_short)Read_Deci(class.index, -1);
	if (errno) return 0;

	class.base_class=config.bmen_bcls;
	class.sub_class =config.bmen_scls;
	class.prog_if   =config.bmen_ifcls;
	if ((ret=pci_bios(FIND_PCI_CLASS_CODE, &class)) != 0) {
	  printf("%s\n", pci_error(ret));
	  return 1;
	}

	Writeln();
	printf("Bus Number =%d\n", class.bus_no);
	printf("Dev Number =%d,  function number =%d\n",
			 class.device_no >>3, class.device_no &0x07);
	return 1;
}

//--------------------------- cyc_cnf_rd ------------------------------------
//
int cyc_cnf_rd(void)
{
	int		ret;
	int		dsp=0;
	u_long	dsp_val=0;
	u_long	lp=0;

	printf("PCI bus     %2d ", config.bmem_crd_bus);
	config.bmem_crd_bus=(u_char)Read_Deci(config.bmem_crd_bus, -1);
	if (errno) return -1;

	printf("device      %2d ", config.bmem_crd_dev);
	config.bmem_crd_dev=(u_char)Read_Deci(config.bmem_crd_dev, 0x1F);
	if (errno) return -1;

	printf("address 0x%04X ", config.bmem_crd_addr);
	config.bmem_crd_addr=(u_short)Read_Hexa(config.bmem_crd_addr, 0xFFFF);
	if (errno) return -1;

	printf("b/w/l 0/1/2 %2d ", config.bmem_crd_sz);
	config.bmem_crd_sz=(u_char)Read_Deci(config.bmem_crd_sz, 2);
	if (errno) return -1;

	cnf_acc.bus_no   =config.bmem_crd_bus;
	cnf_acc.device_no=config.bmem_crd_dev <<3;
	cnf_acc._register=config.bmem_crd_addr;
	while (1) {
		switch (config.bmem_crd_sz) {
	case 0:
			ret=pci_bios(READ_CONFIG_BYTE, &cnf_acc);
			break;

	case 1:
			ret=pci_bios(READ_CONFIG_WORD, &cnf_acc);
			break;


	default:
			ret=pci_bios(READ_CONFIG_DWORD, &cnf_acc);
			break;

		}

		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		if (!dsp && (cnf_acc.value != dsp_val)) { dsp_val=cnf_acc.value; dsp=1; }

		if (!(++lp &0xFFFFL)) {
			printf("\r%10lu %08lX", lp, dsp_val); dsp=0;
			if (kbhit()) {
				Writeln();
				return 1;
			}
		}
	}
}

//--------------------------- cyc_cnf_wr ------------------------------------
//
int cyc_cnf_wr(void)
{
	int		ret;
	u_long	lp=0;

	printf("PCI bus     %2d ", config.bmem_cwr_bus);
	config.bmem_cwr_bus=(u_char)Read_Deci(config.bmem_cwr_bus, -1);
	if (errno) return -1;

	printf("device      %2d ", config.bmem_cwr_dev);
	config.bmem_cwr_dev=(u_char)Read_Deci(config.bmem_cwr_dev, 0x1F);
	if (errno) return -1;

	printf("address 0x%04X ", config.bmem_cwr_addr);
	config.bmem_cwr_addr=(u_short)Read_Hexa(config.bmem_cwr_addr, 0xFFFF);
	if (errno) return -1;

	printf("b/w/l 0/1/2 %2d ", config.bmem_cwr_sz);
	config.bmem_cwr_sz=(u_char)Read_Deci(config.bmem_cwr_sz, 2);
	if (errno) return -1;

	printf("val 0x%08lX ", config.bmem_cwr_val);
	config.bmem_cwr_val=Read_Hexa(config.bmem_cwr_val, -1);
	if (errno) return -1;

	cnf_acc.bus_no   =config.bmem_cwr_bus;
	cnf_acc.device_no=config.bmem_cwr_dev <<3;
	cnf_acc._register=config.bmem_cwr_addr;
	cnf_acc.value    =config.bmem_cwr_val;
	while (1) {
		switch (config.bmem_cwr_sz) {
	case 0:
			ret=pci_bios(WRITE_CONFIG_BYTE, &cnf_acc);
			break;

	case 1:
			ret=pci_bios(WRITE_CONFIG_WORD, &cnf_acc);
			break;


	default:
			ret=pci_bios(WRITE_CONFIG_DWORD, &cnf_acc);
			break;

		}

		if (ret) { printf("%s\n", pci_error(ret)); return 1; }

		if (!(++lp &0xFFFFL)) {
			printf("\r%10lu", lp);
			if (kbhit()) {
				Writeln();
				return 1;
			}
		}
	}
}

//--------------------------- pci_bios_dial ---------------------------------
//
int pci_bios_dial(void)
{
#define SP_		20
#define ZL_		6

	u_int	ux,uy;
	char	key;
	int	ret;

	while (1) {
		clrscr();
		gotoxy(20, 2);
		printf("PCI Version %d.%02X", bios_psnt.major_version, bios_psnt.minor_version);
		gotoxy(20, 3);
		printf("hardware mechanism %02X, last PCI bus %d",
				 bios_psnt.mechanism, bios_psnt.last_PCI_bus);

		ux=0;
		gotoxy(SP_, ux+++ZL_);
						  printf("show all devices ................0");
		gotoxy(SP_, ux+++ZL_);
						  printf("find device .....................1");
		gotoxy(SP_, ux+++ZL_);
						  printf("find class code .................2");
		gotoxy(SP_, ux+++ZL_);
						  printf("cyclic config read ..............3");
		gotoxy(SP_, ux+++ZL_);
						  printf("cyclic config write .............4");
		gotoxy(SP_, ux+++ZL_);
						  printf("Ende ..........................ESC");
		gotoxy(SP_, ux+ZL_);
						  printf("                                 %c ", config.pci_menu);

		do {
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return 0;

			use_default=(key == TAB);
			if (use_default || (key == CR)) key=config.pci_menu;

			uy=key-'0';
			if (uy > 9) uy=uy-7;

		} while (uy > 4);

		clrscr(); gotoxy(1, 10);
		config.pci_menu=key; errno=0; ret=0;
		while (1) {
			switch (uy) {

		case 0:
				ret=_show_pci_device(); break;

		case 1:
				ret=_find_pci_device(); break;

		case 2:
				ret=_find_class(); break;

		case 3:
				ret=cyc_cnf_rd(); break;

		case 4:
				ret=cyc_cnf_wr(); break;

			}

			if (ret == 0) break;

			WaitKey(1);
			if (last_key != TAB) break;

			Writeln();
		}

	}
}
