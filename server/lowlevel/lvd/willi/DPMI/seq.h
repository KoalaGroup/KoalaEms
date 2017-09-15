// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	seq4.h -- PCI Sequencer Release 4

#define SEQ_DEVICE_ID	0x0008

#define ADDRSPACE0		0x80000000L
#define ADDRSPACE1		0xC0000000L

//--------- Device Register Region 2
//
typedef struct {
	u_long	version;				//00
	u_long	sr;					//04 Status Register
#define SR_LB_TOUT		0x00001000L
#define SR_MRESET			0x01000000L
#define SR_CCOUNT			0x02000000L

	u_long	cr;					//08 Control Register
#define CSR_INTMASK		0x00003FF0L	// interrupt enable bits
#define CR_RUN				0x00100000L
#define CR_HALT			0x00200000L

	u_long	intsr;				//0C Status Register
	u_long	counter[4];			//10
	u_long	lvdinv[2];			//20
	u_long	swregs;				//28
	u_long	pb_modifier;		//2C
	u_long	temp;					//30 32 bit temporary register RW for tests
	u_long	jtag_csr;			//34 JTAG control/status
	u_long	jtag_data;			//38 JTAG data
	u_long	res2;					//3C
} SEQ_REGS;

typedef struct {
	u_long	lw,hw;
	u_char	opc;
	u_char	res[7];
} SRAM;

/*===========================================================================

opcode
000...00		JMP
000...01		JSR
000...10 	RTS
000...11 	WHALT
0010iiii		DRTS
0011iiii		DWHALT
010xxxxx		WAIT
0110nn..		COUNT
0111....
10iiiiii		DATA
110.....
1110nnii		DCOUNT
1111nnii		DTRIGGER

===========================================================================*/

#define SRAM_LEN		0x080000L

#define OPC_JMP		0x00
#define OPC_JSR		0x01
#define OPC_RTS		0x02
#define OPC_WHALT		0x03
#define OPC_SW			0x10
#define OPC_SWC		0x18
#define OPC_WTRG		0x40
#define OPC_WTIME		0x50
#define OPC_COUNT		0x60
#define OPC_OUT		0x80
#define OPC_DRTS		0xC0
#define OPC_DGAP		0xD0
#define OPC_DCOUNT	0xE0
#define OPC_TRG		0xF0
//
