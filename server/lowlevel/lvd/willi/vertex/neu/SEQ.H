// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	seq.h -- PCI Sequencer

/*===========================================================================

opcode
0000000		RTS
0000001		WHALT
000001.
00001nn		COUNT
0001ttt		GAP
001crrr		SWITCH
01xxxxx		WAIT
10iiidd		DATA
110nndd		DTRIGGER
1110aaa		JMP
1111aaa		JSR

===========================================================================*/

#define SQ_SRAM_LEN	0x080000L	// SRAM word

#define OPC_RTS		0x00
#define OPC_WHALT		0x01
#define OPC_COUNT		0x04
#define OPC_DGAP		0x08
#define OPC_SW			0x10
#define OPC_SWC		0x18
#define OPC_WTRG		0x20
#define OPC_WTIME		0x30
#define OPC_OUT		0x40
#define OPC_TRG		0x60
#define OPC_JMP		0x70
#define OPC_JSR		0x78

#define B_PAGE			11				// SRAM address bit per page
#define C_PAGE			(1<<B_PAGE)	// SRAM word per page
#define M_PAGE			(C_PAGE-1)
#define B_JMP			7
#define M_JMP			((1<<B_JMP)-1)
#define B_DATA			9						// general count of data bits
#define M_DATA			((1<<B_DATA)-1)
#define B_XDATA		9						// extended may be 11
#define M_XDATA		((1<<B_XDATA)-1)
#define B_DREPT		(14-B_XDATA)		// data repeat field
#define M_DREPT		((1<<B_DREPT)-1)
#define M_GAP			0x0FFF
//
//--------------------------- seq.c -----------------------------------------
//
int seq_edit_menu(
	VTEX_RG	*inp_unit,
	int 		hv);
