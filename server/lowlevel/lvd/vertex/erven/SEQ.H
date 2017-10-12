// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	seq.h -- PCI Sequencer

/*===========================================================================

opcode
0000000		RTS
0000001		WHALT
000001n		CNT
000010n		WTRG
000011.		res
0001ttt		GAP
001crrr		SWITCH
010000n		OTRG
010001n		LPCNT
01001aa		JMPS
011nnaa		TDJNZ
100aaaa		JMP
101aaaa		JSR
11iiiii		DATA

===========================================================================*/

#define TB_SRAM_LEN	0x00200000L		// SRAM word, total => 2 bank LV/HV

#define SQ_SRAM_LEN	0x00080000L		// SRAM word, single bank

#define OPC_RTS		0x00
#define OPC_WHALT		0x01
#define OPC_COUNT		0x02
#define OPC_WTRG		0x04
#define OPC_RES		0x06
#define OPC_DGAP		0x08
#define OPC_SW			0x10
#define OPC_SWC		0x18
#define OPC_TRG		0x20
#define OPC_LPCNT		0x22
#define OPC_JMPS		0x24
#define OPC_TDJNZ		0x30
#define OPC_JMP		0x40
#define OPC_JSR		0x50
#define OPC_OUT		0x60

#define B_PAGE			11						// SRAM address bit per jump short
#define C_PAGE			(1<<B_PAGE)			// SRAM word per jump short
#define M_PAGE			(C_PAGE-1)
#define B_EJMP			6						// jump address extention bit
#define M_EJMP			((1<<B_EJMP)-1)
#define B_DATA			9						// general count of data bits
#define M_DATA			((1<<B_DATA)-1)
#define B_DREPT		5						// length of data repeat field
#define M_DREPT		((1<<B_DREPT)-1)
#define M_GAP			0x0FFF
#define M_JMPS			0x07FF
#define M_JMP			0x1FFF
//
//--------------------------- seq.c -----------------------------------------
//
int seq_load(VTEX_RG	*_inp_unit, char *fname, char hv);

int seq_edit_menu(
	VTEX_RG	*inp_unit,
	int 		hv);
