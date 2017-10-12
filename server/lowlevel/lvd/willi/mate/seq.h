/*!
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * ---
 * Copyright (C) 2008, Willi Erven <w.erven@fz-juelich.de>
 */


/*===========================================================================

opcode
0000000		RTS
0000001		WHALT
000001n		CNT
000010n		WTRG
000011n		DGAP
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

#define TB_SRAM_LEN	0x00200000		// SRAM word, total => 2 bank LV/HV

#define SQ_SRAM_LEN	0x00080000		// SRAM word, single bank

#define OPC_RTS		0x00
#define OPC_WHALT		0x01
#define OPC_COUNT		0x02
#define OPC_WTRG		0x04
#define OPC_DGAP		0x06
#define OPC_GAP		0x08
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

#define OPC(x)	((u_int)((x)<<B_DATA))
