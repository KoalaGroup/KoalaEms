// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	HOTLink.h -- register types and protocol constants

#define HOTLINK_DEVICE_ID	0x0002

#define ADDRSPACE0		0x80000000L

//--------- Device Register Region 2
//
typedef union {			// multi size
	u_char	byte;			//		send 1 Byte
	u_short	word;			//		send 2 Byte
	u_long	dword;		//		send 4 Byte
} DREG;

typedef struct {
	u_long	version;				//00 must be 1
	u_long	sr;					//04 Status Register
	u_long	cr;					//08 Control Register
	u_long	intsr;				//0C Status Register
	DREG		txrx_data;			//10 transmit/receive data register
	DREG		tx_special;			//14 transmit special char register
	u_long	eod_delay;			//18 delay for End Of Data status bit
	u_long	txrx_tout;			//1C Wert fuer transmit/receive Timeout
	u_long	dmd_granularity;	//20 size for block interrupt (minus 4)
	u_long	dmd_gran_counter;	//24 counter form <dmd_granularity> down to 0
	u_long	byte_counter;		//28 HOTLink receive byte counter
	u_long	spec_counter;		//2C special character counter
	u_long	temp;					//30 32 bit temporary register RW for tests
	u_long	jtag_csr;			//34 JTAG control/status
	u_long	jtag_data;			//38 JTAG data
} HOTL_REGS;
//
// -------------------------- status register --------------------------------
//
#define SR_CARRIER			0x00000001L
#define SR_DATA_AVAIL		0x00000002L
#define SR_FIFO_FULL			0x00000004L
#define SR_REC_STOPED		0x00000008L
#define SR_CARRIER_CHG		0x00000010L
#define SR_SPEC_CAHR			0x00000020L
#define SR_START_DATA		0x00000040L
#define SR_END_OF_DATA		0x00000080L
#define SR_TXRX_TOUT       0x00000100L
#define SR_DMD_GRAN_SIG		0x00000200L
#define SR_FIFO_FULL_ERR	0x00000400L
#define SR_REC_ERR			0x00000800L
#define SR_LB_TOUT			0x00001000L

#define SR_MRESET				0x00010000L
#define SR_REC_FLUSH			0x00020000L
#define SR_REC_RESET			0x00040000L
#define SR_DMA0_ABORT		0x00080000L
//
// -------------------------- control register -------------------------------
//
#define CR_REC_ENABLE		0x00001L	// general reset
#define CR_LOOP_BACK			0x00002L	// transparent test mode
#define CR_AUTO_FLUSH		0x00004L

#define CSR_INTMASK			0x00FF0L	// interrupt enable bits
//
// -------------------------- granularity counter ----------------------------
//
#define CW_GRAN_CNT		24		// width
//
