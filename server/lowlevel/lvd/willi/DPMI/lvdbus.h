// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	lvdbus.h -- LVD BUS Master und Slave(Test) PCI Interface

#define LVDBUS_DEVICE_ID	0x0007

#define ADDRSPACE0			0x80000000L
#define ADDRSPACE1			0xC0000000L

#define MASTER		0x07		// 3. Byte im version Register
#define SLAVE		0x17
//
//--------- Device Register Region 2
//
typedef struct {
	u_long	version;				//00
	u_long	sr;					//04 Status Register
#define SR_BLKTX				0x00000001L		// Blocktransfer
#define SR_BLKEND				0x00000010L		// Blocktransfer End
#define SR_ATTENT				0x00000020L		// SCAN Attention
#define SR_LB_TOUT			0x00001000L		// local bus timeout
#define SR_MRESET				0x00010000L		// Master Reset
#define SR_DMA0_ABORT		0x00020000L		// abort DMA channel 0

	u_long	cr;					//08 Control Register
#define CR_INTMASK			0xFFF0

	u_long	intsr;				//0C Status Register
	u_long	lvd_block;			//10
	u_long	res0[3];				//14
	u_long	lvd_scan_win;		//20
	u_long	lvd_scan;			//24
#define LVD_SCAN			0x100
#define LVD_SCAN_BLOCK	0x200

	u_long	lvd_blk_cnt;		//28 21 bit
	u_long	lvd_blk_max;		//2C 21 bit
	u_long	temp;					//30 32 bit temporary register RW for tests
	u_long	jtag_csr;			//34 JTAG control/status
	u_long	jtag_data;			//38 JTAG data
	u_long	res2;					//3C
	u_long	lvd_scan_cnt[16];	//40 je 17 bit
} LVD_REGS;
//
