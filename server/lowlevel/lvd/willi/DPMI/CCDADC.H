// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	ccdadc.h -- PCI base board with CMC ADC

#define CCDADC_DEVICE_ID	0x0009

#define ADDRSPACE0			0x80000000L

#define CCDADC		0x08		// 3. Byte im version Register

//--------- Device Register Region 2
//
typedef struct {
	u_long	version;				//00
	u_long	sr;					//04 Status Register
	u_long	cr;					//08 Control Register
	u_long	intsr;				//0C Status Register
	u_long	delay;				//10
	u_long	dac_value;			//14
	u_long	adc_value;			//18
	u_long	pixel_counter;		//1C
	u_long	frame_counter;		//20
	u_long	frame_balance;		//24
	u_long	frame_size;			//28
	u_long	frame_block;		//2C
	u_long	temp;					//30 32 bit temporary register RW for tests
	u_long	jtag_csr;			//34 JTAG control/status
	u_long	jtag_data;			//38 JTAG data
	u_long	pixel_size;			//3C
	u_long	balance_max;		//40
} ADC_REGS;
//
// -------------------------- status register --------------------------------
//
#define SR_LB_TOUT			0x00001000L

#define SR_MRESET				0x01000000L
#define SR_FORCE_EOT			0x02000000L
//
// -------------------------- control register -------------------------------
//
#define CR_ADCINPUT			0x00001L	// Read ADC Data for DMA transfer
#define CR_VERBOSE			0x00002L	// Read ADC Data for DMA transfer

#define CSR_INTMASK			0x01FF0L	// interrupt enable bits
//
