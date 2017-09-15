// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	n110.h -- PCI base board with CMC N110 TDC

#define N110TDC_DEVICE_ID	0x000A

#define ADDRSPACE0			0x80000000L

#define N110TDC	0x0A		// 3. Byte im version Register

//--------- Device Register Region 2
//
typedef struct {
	u_long	version;				//00
	u_long	sr;					//04 Status Register
#define SR_DATA_AVAIL		0x00000001L
#define ISR_DATA_AVAIL		0x00000010L
#define ISR_FIFO_HFULL		0x00000020L
#define ISR_FIFO_FULL		0x00000040L
#define ISR_V24_RDY			0x00000080L
#define ISR_LB_TOUT			0x00000100L
#define ISR_GRAN_SIG			0x00000200L
#define ISR_GRAN_OVFL		0x00000400L

#define SR_MRESET				0x00010000L
#define SR_DAQ_RESET			0x00020000L
#define SR_COUNTER_RST		0x00040000L
#define SR_TIME_STAMP_RST	0x00080000L
#define SR_DMD_EOT			0x00100000L
#define SR_AB_DMA0			0x00200000L

	u_long	cr;					//08 Control Register
#define CR_DAQ_ENABLE		0x00001L	// general reset
#define CSR_INTMASK			0x01FF0L	// interrupt enable bits

	u_long	intsr;				//0C Status Register
	u_long	daq_data;			//10
	union {
			u_char tx_byte;
			u_long tx_prot;
			}  v24_tx;				//14
	u_long	byte_count;			//18
	u_long	counter[5];			//1C
	u_long	stime;				//30
	u_long	temp;					//34 32 bit temporary register RW for tests
	u_long	jtag_csr;			//38 JTAG control/status
	u_long	jtag_data;			//3C JTAG data
	u_long	rx_gran;				//40
	u_long	gran_count;			//44
	
} N110_REGS;
//
