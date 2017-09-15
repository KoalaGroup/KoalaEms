// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	gigalink.h -- register types and protocol constants

#define GIGALINK_DEVICE_ID	0x0001

#define ADDRSPACE0		0x80000000L
#define RREG_OFFS			0x0800			// start of remote Regs in space 0
#define ADDRSPACE1		0xC0000000L
#define GRANSPACE1		0x00400000L		// space 1 granularity (4 MB)
#define UPMSKSPACE1		(~(GRANSPACE1-1))
#define LOMSKSPACE1		(GRANSPACE1-1)

#define C_MAILEXT		192
#define C_SP1_SEL		64		// # of SPACE1 selectors
#define CT_SP1_SEL	4		// for this test program

typedef struct {
	u_long	hdr;
	u_long	am;
	u_long	ad;
	u_long	ad_h;
} BUS_DESCR;

typedef struct {
	u_long	ident;		//000
	u_long	sr;			// 	status register
	u_long	cr;			// 	control register (interrupt enable bits)
	u_long	semaphore;
	u_long	doorbell;	//010 only remote, access to L2PDBELL for local
	u_long	res0[3];
	u_long	mailbox[8];	//020 only remote, access to MBOX[0-7] for local
	u_long	res1[16];

	u_long	t_hdr;		//080
	u_long	t_am;
	u_long	t_ad;
	u_long	t_ad_h;
	u_long	t_da;			//090
	u_long	t_da_h;
	u_long	prot_err_nw;

	u_long	tc_hdr;
	u_long	tc_da;		//0A0
	u_long	res3;			//		tc_da_h

	u_long	balance;		// 	counter for request(up)/confirmation(down)
	u_long	prot_err;	// 	first protocol error (read and clear)

	u_long	d0_bc;		//0B0 dma0 demand byte count (for DAQ)
	u_long	d0_bc_adr;	//		pci address to buffer for byte counts
	u_long	d0_bc_len;	//		buffer length

	u_long	d_hdr;		//		block transfer or pipelined read
	u_long	d_am;			//0C0
	u_long	d_ad;
	u_long	d_ad_h;
	u_long	d_bc;
	u_long	res4[2];		//0D0

	u_long	rd_pipe_adr;//		pci address to buffer for pipelined read
	u_long	rd_pipe_len;//		length of the buffer
	u_long	res5[2];		//0E0

	u_long	tp_special;	//		transparent mode, write special word
	u_long	tp_data;		//		transparent mode, write data word
	u_long	opt_csr;		//0F0	control/status for optical interface, pigy back
	u_long	jtag_csr;	//		JTAG control/status
	u_long	jtag_data;	//		JTAG data
	u_long	res6;

	u_long	mailext[C_MAILEXT];//100
//	0x400
	BUS_DESCR	bus_descr[C_SP1_SEL];	// descriptor for direct bus access, space1
// 0x800
} GIGAL_REGS;
//
// -------------------------- status register --------------------------------
//
#define SR_RX_SYNC			0x00000001L	// optical receiver synchronized
#define SR_TX_SYNC			0x00000002L	// optical remote receiver synchronized
#define SR_INHIBIT			0x00000004L
#define SR_CONFIGURED		0x00000008L
#define SR_TX_SYNCH_CHG		0x00000010L
#define SR_INH_CHG			0x00000020L
#define SR_SEMA_CHG			0x00000040L
#define SR_REC_VIOLATION	0x00000080L	// receive violation
#define SR_RESET_REQ			0x00000100L
#define SR_DMD_EOT			0x00000200L
#define SR_MBX0				0x00000400L
#define SR_S_XOFF				0x00000800L
#define SR_LEMO_IN_0_CHG	0x00001000L
#define SR_LEMO_IN_1_CHG	0x00002000L
#define SR_PROT_END			0x00004000L	// last confirmation or error received
#define SR_PROT_ERROR		0x00008000L	// error received

#define SR_DMA0_BLOCKED		0x00010000L
#define SR_NO_PREAD_BUF		0x00020000L
#define SR_PROT_ERR			0x00040000L
#define SR_BUS_TOUT			0x00080000L	// PLX local bus timeout
#define SR_TP_SPECIAL		0x00100000L	// transparent special word available
#define SR_TP_DATA			0x00200000L	// transparent data available

#define SR_SEND_EOT			0x80000000L	// terminate DMA0 with EOT signal
//
// -------------------------- control register ------------------------------
//
#define CR_RESET				0x00001L	// general reset
#define CR_TRANPARENT		0x00002L	// transparent test mode
#define CR_READY				0x00004L
#define CR_BIGENDIAN			0x00008L

#define CSR_INTMASK			0x07FF0L	// interrupt enable bits
#define CR_REC_FLUSH			0x08000L	// flush opt receiver if transparent mode
#define CR_REM_RESET			0x10000L	// send reset request to remote

#define SET_CR_IM(im)	(((~(im) &0xFFFEL) <<16) |((im) &0xFFFF))
//
// -------------------------- optical control/status register ---------------
//
#define OCS_FIFO_PFF			0x40000000L		// FIFO partial full
#define OCS_FIFO_FF			0x80000000L		// FIFO full
//
// -------------------------- protocol definitions --------------------------
//
#define SC_PROT	0x1C				// special character K28.0

#define THW_SO_HD	0x00000001L		// temp prot, start on writing header
#define THW_SO_AD	0x00000002L		// temp prot, start on address
#define THW_SO_DA	0x00000004L		// temp prot, start on writing data
#define HW_WR		0x00000400L		// header word, write request
#define HW_AM		0x00000800L		// VME address modifier
#define HW_A64		0x00001000L		// 64 Bit address
#define HW_BT		0x00002000L		// Blocktransfer
#define HW_FIFO	0x00004000L		// FIFO mode (no address increment)
#define HW_EOT		0x00008000L		// for Blocktransfers
#define HW_R_BUS	0x00010000L		// remote bus space
#define HW_R_DMD	0x00020000L		// demand remote space
#define HW_L_PIPE	0x00400000L		// read pipe local space
#define HW_L_DMD	0x00800000L		// demand local space
#define HW_BE		0x0F000000L		// byte enable
#define HW_BE0011	0x03000000L

#define P_END		0x000100L	// protocoll end delimiter
#define P_CON		0x000200L	// protocoll reply/confirmation
#define P_ECON		0x000300L	// protocoll error reply/confirmation
//
// -------------------------- protocol errors --------------------------------
//
#define PE_SYNCH		0x001
#define PE_NRDY		0x002
#define PE_XOFF		0x003
#define PE_RESOURCE	0x004
#define PE_DLOCK		0x005
#define PE_PROT		0x006
#define PE_TO			0x007
#define PE_BERR		0x008

#define LPE_RESOURCE	0x104
#define LPE_DLOCK		0x105
#define LPE_TO			0x107
