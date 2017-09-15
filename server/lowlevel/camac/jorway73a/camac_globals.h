/* ************************************************************** */
/*                                                                */
/*   camac_globals.h						  */
/* -------------------------------------------------------------- */

/*
	16aug95	jms	stuff needed for sjy driver
	13sep95 das	added min/max defines
	22jul97 das     modified for linux
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

typedef unsigned char UCHAR;

#ifndef XDEFLG
#define xdef extern
#else
#define xdef
#endif

#define SJY_VERSION "v1.1"      /* Version of sjy Linux port */

/* SCSI command definitions */
#define INQUIRY_CMD         0x12
#define INQUIRY_CMDLEN         6
#define INQUIRY_REPLY_LEN     36
#define INQUIRY_VENDOR         8     /* Offset in reply data to vendor name */

#define TESTUNITREADY_CMD    0x0
#define TESTUNITREADY_CMDLEN   6

#define SENSEKEY               2     /* Offset in sense buffer to sense key */
#define ADD_SENSECODE         12     /* Offset to additional sense code */

#define SENSE_CMD           0x03
#define SENSE_CMDLEN           6


 
/* SCSI return statuses */
#define SJY_GOOD            0x00
#define SJY_CHECK_CONDITION 0x02
#define SJY_CONDITION_MET   0x04

/* SCSI sense buffer length. The Jorway 411S is 16 bytes, the 73A is 18 bytes */
#define SJY_SENSE_LENGTH     18


/* sense keys */
#define SJY_NO_SENSE        0x0
#define SJY_NOT_READY       0x2
#define SJY_HARDWARE_ERROR  0x4
#define SJY_ILLEGAL_REQUEST 0x5
#define SJY_UNIT_ATTENTION  0x6
#define SJY_SHORT_TRANSFER  0x9
#define SJY_ABORTED_COMMAND 0xB

/* additional sense keys */
#define SJY_NO_X            0x44
#define SJY_NO_Q            0x80



/* branch specific information */
typedef struct{
	int	fd;		/* file descriptor for branch */
	int	scsi_id;	/* SCSI id of JY411S */
	int	scsi_bus;	/* SCSI bus number, or controller number */
	int	serial;		/* serial=1, parallel=0 */
        int     errn;           /* SCSI/CAMAC error */
        int     result;         /* SCSI command result */
        unsigned char sense_buffer[SJY_SENSE_LENGTH]; /* SCSI sense buffer */ 
	} SJY_BRANCH;


/* note: SJY411S or SJY73A is defined in the build */


/* Function Modifier 	- D16/D24 */
#define	SJY_D24_BIT			0x20


/*
** Jorway 411S definitions
*/

#ifdef SJY411S

#define	SJY_MAX_SJY411S	32	/* maximum number of Jorways on host computer */

xdef SJY_BRANCH sjy_branch[SJY_MAX_SJY411S];

/* Valid ranges for sjy branch, crate, slot, subadd */
#define SJY_BRANCH_MAX  SJY_MAX_SJY411S
#define SJY_BRANCH_MIN  0
#define SJY_CRATE_SER_MAX   62
#define SJY_CRATE_PAR_MAX   7
#define SJY_CRATE_MIN   1
#define SJY_SLOT_MAX    30
#define SJY_SLOT_MIN    1
#define SJY_SUBADD_MAX  15
#define SJY_SUBADD_MIN  0


/* structures and definitions for Request Sense */
typedef struct _SJB_SENSE {
	UCHAR	errorCode;
	UCHAR	segNumber;
        UCHAR	senseKey;
	UCHAR	reserved1;
	UCHAR	wcda[3];
	UCHAR	asl;
	UCHAR	mainReg;
	UCHAR	serialReg[2];
	UCHAR	reserved2;
	UCHAR	addSense;
	UCHAR	reserved3;
	UCHAR	cna[2];
} SJB_SENSE, *PSJB_SENSE;

#define CRATE(cna)		((cna[0] & 0xFE) >> 1)
#define SLOT(cna)		(((cna[0] & 0x01) >> 1) | ((cna[1] & 0xF0) >> 4))
#define ADD(cna)		(cna[1] & 0x0F)
#define WCD(wcda)		((wcda[0] << 16) | (wcda[1] << 8) | (wcda[2]))


/* CAMAC command codes for the SRB */
#define SJY_NONDATA_CMD		0xC1
#define SJY_DATA_CMD		0xE1
#define SJY_SHORTXFER_DATA_CMD	SJY_NONDATA_CMD


/*  Jorway 411S SRB layout
 *
 *	Non-Data Transfers
 *	b0	0xC1
 *	b1	F
 *	b2	N
 *	b3	A
 *	b4	C	| 0x80 for serial
 *	b5	0
 *
 *	Data Transfers
 *	b0	0x1E
 *	b1	0
 *	b2	F	| 0x20 for 24 bit functions
 *	b3	N
 *	b4	A 	| (mode<<5)
 *	b5	C	| 0x80 for serial
 *	b6	xfrlen >>16			bytes
 *	b7	xfrlen >> 8			bytes
 *	b8	xfrlen				bytes
 *	b9	0
 *
 */

/* Crate Modifier 	- Parallel/Serial */
#define SJY_SERIAL_BIT		0x80

/* Slot Modifiers  	- DMA Mode */
#define SJY_Q_STOP		0x00
#define SJY_ECA			0x20
#define SJY_Q_REPEAT		0x40
#define SJY_ADD_SCAN		0x60
#define SJY_ZEN_MODE		0x80
#define SJY_EXT_SUB_SCAN	0xA0
#define SJY_STOP_ON_WORD	0xE0
#define SJY_MODE_MASK		0xE0
#define SJY_D24_MODE		(SJY_D24_BIT<<24)

#define SJY_CRATE(c)	(((c)&SJY_SERIAL_BIT)?(c):(1<<((c)-1)))

#endif	/* 411S */


/*
** Jorway 73A defintions
*/

#ifdef SJY73A	

// "Branch" is used here synonymously with Controller SCSI ID
// for code reuse purposes
#define	SJY_MAX_SJY73A	7	/* maximum number of Jorways on host computer */

xdef SJY_BRANCH sjy_branch[SJY_MAX_SJY73A];

/* Valid ranges for sjy branch, crate, slot, subadd */
#define SJY_BRANCH_MAX  SJY_MAX_SJY73A
#define SJY_BRANCH_MIN  0
#define SJY_SLOT_MAX    30
#define SJY_SLOT_MIN    1
#define SJY_SUBADD_MAX  15
#define SJY_SUBADD_MIN  0


/* structures and definitions for Request Sense */
typedef struct _SJB_SENSE {
	UCHAR	errorCode;
	UCHAR	segNumber;
	UCHAR	senseKey;
	UCHAR	fifoStatus;
	UCHAR	dmabc[3];
	UCHAR	asl;
	UCHAR	reserved8;
	UCHAR	reserved9;
	UCHAR	reserved10;
	UCHAR	reserved11;
	UCHAR	addSense;
	UCHAR	reserved13;
	UCHAR	reserved14;
	UCHAR	reserved15;
	UCHAR	reserved16;
	UCHAR	reserved17;
} SJB_SENSE, *PSJB_SENSE;

#define DMABC(dmabc)		((dmabc[0] << 16) | (dmabc[1] << 8) | (dmabc[2]))


/* CAMAC command codes for the SRB */
#define	SJY_NONDATA_CMD			0x01
#define SJY_SHORTXFER_DATA_CMD		0x01
#define SJY_LONGXFER_DATA_CMD		0x21


/*  Jorway 73A SRB layout
 *
 *	Non-Data Transfers
 *	b0	0x01
 *	b1	F
 *	b2	N
 *	b3	A
 *	b4	0
 *	b5	0
 *
 *	Short Data Transfers (<256 bytes)
 *	b0	0x01
 *	b1	F
 *	b2	N	| (mode<<6) | 0x20 for 24 bit functions
 *	b3	A
 *	b4	xfrlen
 *	b5	0
 *
 *	Long Data Transfers
 *	b0	0x21
 *	b1	0
 *	b2	F
 *	b3	N	| (mode<<6) | 0x20 for 24 bit functions
 *	b4	A
 *	b5	0
 *	b6	xfrlen >>16			bytes
 *	b7	xfrlen >> 8			bytes
 *	b8	xfrlen				bytes
 *	b9	0
 */

/* Crate Modifier 	- Parallel/Serial. The 73A is a type A (parallel) controller only */
#define SJY_SERIAL_BIT		0x00

/* Slot Modifiers  	- DMA Mode */
#define SJY_STOP_ON_WORD	0x00
#define SJY_ADD_SCAN		0x40
#define SJY_Q_STOP		0x80
#define SJY_Q_REPEAT		0xC0
#define SJY_MODE_MASK		0xC0
#define SJY_D24_MODE		(SJY_D24_BIT<<16)

#define SJY_CRATE(c)		0

#endif	/* 73A */


/* Clear DMA mode in SRB */
#define SJY_CLEAR_MODE(cmd)		(cmd & ~(SJY_MODE_MASK<<16))

/* Encode function, station, address, crate for SRB */
#define SJY_CAMAC_EXT(f,n,a,c)   ( (SJY_CRATE(c)) | ((a)<<8) | ((n)<<16) | ((f)<<24) )



/* define maximum configuration */
#define SCSI_MAXCTLR	4				// typical # of PCI slots/PC
#define SCSI_MAXTARG	7				// for an AHA-2940 
#define SCSI_SENSE_LEN	32


/* sjy interface (to generic scsi driver) functions */
int sjy_get_qx(int branch);
int sjy_read(int branch, int cmd, void *dataBuffer, int xfrlen);
int sjy_write(int branch, int cmd, void *dataBuffer, int xfrlen);
int sjy_nondata(int branch, int cmd);
unsigned char *sjy_inquiry(int branch);
int sjy_tur(int branch);
unsigned char *sjy_reqSense(int branch);

int sjy_notify_camac_data(int branch, int xfrlen, int status);
int sjy_notify_camac_nondata(int branch, int xfrlen, int status);

void sjy_printerror(unsigned long ErrorCode);

