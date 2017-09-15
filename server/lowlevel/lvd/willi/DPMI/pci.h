// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

/*	pci.h -- Library for PCI Devices */

#ifndef _PCI_H
#define _PCI_H

#define FZJ_VENDOR_ID	0x1796

#define AMCC_VENDOR_ID	0x10E8
#define AMCC_DEVICE_ID	0x4750

#define C_REGION		6
#define C_DESC			32		// size of selector table

#define LIT2LONG(a, b)	((((u_long)b) <<16) | a)
#define OFFSET(strct, field)	((u_int)&((strct*)0)->field)

#define CTL_A	0x01
#define ETX		0x03
#define CTL_C	ETX
#define EOT		0x04
#define ENQ		0x05
#define ACK		0x06
#define BEL		0x07
#define BS		0x08
#define TAB		0x09
#define LF		0x0A
#define FF		0x0C
#define CR		0x0D
#define DLE		0x10
#define XON		0x11
#define CTL_R	0x12
#define XOFF	0x13
#define NAK		0x15
#define ETB		0x17
#define CTL_Z	0x1A
#define ESC		0x1B
#define DEL		0x7F
//
// code for function keys
//
#define F1			59
#define F2			60
#define F3			61
#define F4			62
#define F5			63
#define F6			64
#define F7			65
#define F8			66
#define F9			67
#define F10			68

#define SHIF1		84
#define SHIF2		85
#define SHIF3		86
#define SHIF4		87
#define SHIF5		88
#define SHIF6		89
#define SHIF7		90
#define SHIF8		91
#define SHIF9		92
#define SHIF10		93

#define CTLF1		94
#define CTLF2		95
#define CTLF3		96
#define CTLF4		97
#define CTLF5		98
#define CTLF6		99
#define CTLF7		100
#define CTLF8		101
#define CTLF9		102
#define CTLF10		103

#define ALTF1		104
#define ALTF2		105
#define ALTF3		106
#define ALTF4		107
#define ALTF5		108
#define ALTF6		109
#define ALTF7		110
#define ALTF8		111
#define ALTF9		112
#define ALTF10		113

#define UP			72
#define PGUP		73
#define LEFT		75
#define RIGHT		77
#define DOWN		80
#define PGDN		81
#define CDEL		83

#define CTLUP		141
#define CTLPGUP	132
#define CTLLEFT	115
#define CTLRIGHT	116
#define CTLDOWN	145
#define CTLPGDN	118
#define CTLCDEL	147

#define ALTUP		152
#define ALTPGUP	153
#define ALTLEFT	155
#define ALTRIGHT	157
#define ALTDWON	160
#define ALTPGDN	161
#define ALTCDEL	163
//
//------ PCI BIOS Constants and Types --------
//
#define PCI_FUNCTION_ID   	0xB1

#define PCI_BIOS_PRESENT      	0x01
#define FIND_PCI_DEVICE       	0x02
#define FIND_PCI_CLASS_CODE   	0x03
#define GENERATE_SPECIAL_CYCLE	0x06
#define READ_CONFIG_BYTE      	0x08
#define READ_CONFIG_WORD      	0x09
#define READ_CONFIG_DWORD     	0x0A
#define WRITE_CONFIG_BYTE     	0x0B
#define WRITE_CONFIG_WORD     	0x0C
#define WRITE_CONFIG_DWORD    	0x0D

#define FUNCTION_NOT_SUPPORTED  	0x81
#define BAD_VENDOR_ID           	0x83
#define DEVICE_NOT_FOUND        	0x86
#define BAD_REGISTER_NUMBER     	0x87

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef unsigned int		u_int;

//
//--- PCI Operation Registers
//
#define OMB1   0x00	// Outgoing Mailbox Register 1
#define IMB1   0x10	// Incoming Mailbox Register 1
#define IMB4   0x1C	// Incoming Mailbox Register #4
#define FIFO   0x20	// FIFO Register (bidirectional)
#define MWAR	0x24	// Master Write Address Register, a0/a1 wired as zeros
#define MWTC	0x28	// Master Write Transfer Count Register, in bytes
#define MRAR	0x2C	// Master Read Address Register, a0/a1 wired as zeros
#define MRTC	0x30	// Master Read Transfer Count Register, in bytes
#define MBEF	0x34	// Mailbox Empty/Full Status
#define INTCSR	0x38	// Interrupt Control/Status Register
#define MCSR   0x3C	// Bas Master Control/Status Register

typedef struct {
	u_long	omb[4],	// Outgoing Mailbox Register
				imb[4],  // Incoming Mailbox Register
				fifo,    // FIFO Register (bidirectional)
				mwar,		// Master Write Address Register, a0/a1 wired as zeros
				mwtc,		// Master Write Transfer Count Register, in bytes
				mrar,		// Master Read Address Register, a0/a1 wired as zeros
				mrtc,		// Master Read Transfer Count Register, in bytes
				mbef,    // Mailbox Empty/Full Status
				intcsr,	// Interrupt Control/Status Register
				mcsr;		// Bus Master Control/Status Register
} AMCC_REGS;
//
// intcsr definitions
//
#define IE_OMB			0x00000010L		// Outgoing Mailbox
#define IE_IMB			0x00001000L		// Incoming Mailbox
#define IE_WRTC		0x00004000L		// Memory Write Terminate Count
#define IE_RDTC		0x00008000L		// Memory Read Terminate Count
#define IS_OMB			0x00010000L		// Interrupt Status
#define IS_IMB			0x00020000L
#define IS_WRTC		0x00040000L		// Write Transfer Complete
#define IS_RDTC		0x00080000L
#define IS_IASS		0x00800000L
//
// mcsr definitions
//
#define P2A_FF			0x00000001L		// PCI to AddOn FIFO Full
#define P2A_FPF		0x00000002L		// PCI to AddOn FIFO Partial Full
#define P2A_FE			0x00000004L		// PCI to AddOn FIFO Empty
#define A2P_FF			0x00000008L		// AddOn to PCI FIFO Full
#define A2P_FPE		0x00000010L		// AddOn to PCI FIFO Partial Empty
#define A2P_FE			0x00000020L		// AddOn to PCI FIFO Empty
#define A2P_FIFO		0x00000038L		// AddOn to PCI FIFO Flags
#define x2x_FIFO		0x0000003FL		// FIFO Flags
#define x2x_EMPTY		0x00000026L		// FIFO Empty
#define MEMRD_ZERO	0x00000040L		// read transfer count is zero
#define MEMWR_ZERO	0x00000080L		// write transfer count is zero
#define MEMWR_PRIOTY	0x00000100L		// Memory Write Write vs Read Priority
#define MEMWR_MANMNT	0x00000200L		// Memory Write FIFO Management Scheme
#define MEMWR_ENBL	0x00000400L		// Memory Write Transfer Enable
#define MEMRD_PRIOTY	0x00001000L		// Memory Read Read vs Write Priority
#define MEMRD_MANMNT	0x00002000L		// Memory Read FIFO Management Scheme
#define MEMRD_ENBL	0x00004000L		// Memory Read Transfer Enable
#define ADON_RST		0x01000000L		// Add-on Reset
#define FIFO2AD_RST	0x02000000L		// PCI to Add-on FIFO Status Flags Reset
#define AD2FIFO_RST	0x04000000L		// Add-on to PCI FIFO Status Flags Reset
#define MBX_RST		0x08000000L		// Mailbox Flags Reset
#define NV_LAB			0x80000000L		// nvRAM low address byte
#define NV_HAB			0xA0000000L		// nvRAM high address byte
#define NV_WR			0xC0000000L		// nvRAM begin write
#define NV_RD			0xE0000000L		// nvRAM begin read

//============================================================================
//
//										PLX Controller PCI 9054-AC
//										PLX Controller PCI 9656-BA
//
//--------- PCI Vendor/Device ID
//
#define PLX_VENDOR_ID		0x10B5
#ifdef PLX64
#define PLX_DEVICE_ID		0x9656
#else
#define PLX_DEVICE_ID		0x9054
#endif
//
//
typedef struct {
	u_long	pciidr;		// 00
	u_short	pcicr;
	u_short	pcisr;
	u_char	pcirev;
	u_char	pciccr[3];
	u_char	pciclsr;
	u_char	pciltr;
	u_char	pcihtr;
	u_char	pcibistr;
	u_long	pcibar0;		// 10
	u_long	pcibar1;
	u_long	pcibar2;
	u_long	pcibar3;
	u_long	pcibar4;		// 20
	u_long	pcibar5;
	u_long	pcicis;
	u_short	pcisvid;
	u_short	pcisid;
	u_long	pcierbar;	// 30
	u_long	cap_ptr;
	u_long	space0;
	u_char	pciilr;
	u_char	pciipr;
	u_char	pcimgr;
	u_char	pcimlr;
	u_char	pmcapid;		// 40
	u_char	pmnext;
	u_short	pmc;
	u_short	pmcsr;
	u_char	pmcsr_bse;
	u_char	pmdata;
	u_char	hs_cntl;		// 48
	u_char	hs_next;
	u_short	hs_csr;
	u_char	pvpdcntl;
	u_char	pvpd_next;
	u_short	pvpdad;
	u_long	pvpdata;		// 50
} PCI_CONFIG_REGS;


typedef struct {
	u_long	dmamode;
	u_long	dmapadr;
	u_long	dmaladr;
	u_long	dmasize;
	u_long	dmadpr;
} PLX_DMA;					// 14

typedef struct {
	u_long	dmapadr;
	u_long	dmaladr;
	u_long	dmasize;
	u_long	dmadpr;
} PLX_DMA_DESC;			// 10

typedef struct {			//		complete register set
	u_long	las0rr;		// 00
	u_long	las0ba;
	u_long	marbr;		// 08
	u_char	bigend;
	u_char	lmisc1;
	u_char	prot_area;
	u_char	lmisc2;		//		PCI9656
	u_long	eromrr;		// 10
	u_long	eromba;
	u_long	lbrd0;		// 18
	u_long	dmrr;
	u_long	dmlbam;		// 20
	u_long	dmlbai;
	u_long	dmpbam;
	u_long	dmcfga;
	u_long	opqis;		// 30
	u_long	opqim;
	u_long	space1[2];	// 38
	u_long	mbox[8];		// 40
#define iqp		mbox[0]
#define oqp		mbox[1]

	u_long	p2ldbell;	// 60
	u_long	l2pdbell;
	u_long	intcsr;
#define PLX_INT_ENA		0x000100L
#define PLX_IDOOR_ENA	0x000200L
#define PLX_ILOC_ENA		0x000800L
#define PLX_IDOOR_ACT	0x002000L
#define PLX_ILOC_ACT		0x008000L
#define PLX_IDMA0_ENA	0x040000L
#define PLX_IDMA1_ENA	0x080000L
#define PLX_IDMA0_ACT	0x200000L
#define PLX_IDMA1_ACT	0x400000L

	u_long	cntrl;
#define EESK	0x01000000L
#define EECS	0x02000000L
#define EEDO	0x04000000L
#define EEDI	0x08000000L
#define EEPR	0x10000000L

	u_long	pcihidr;		// 70
	u_long	pcihrev;
	u_long	space2[2];
	PLX_DMA	dma[2];		// 80
	u_char	dmacsr[2];	// A8
#define D_ENABLE	0x01
#define D_START	0x02
#define D_CLR_INT	0x08

	u_short	space3;
	u_long	dmaarb;		// AC same as marbr
	u_long	dmathr;		// B0
	u_long	dmadac[2];
	u_long	space4;
	u_long	mqcr;			// C0
	u_long	qbar;
	u_long	ifhpr;
	u_long	iftpr;
	u_long	iphpr;		// D0
	u_long	iptpr;
	u_long	ofhpr;
	u_long	oftpr;
	u_long	ophpr;		// E0
	u_long	optpr;
	u_long	qsr;
	u_long	space5;
	u_long	las1rr;		// F0
	u_long	las1ba;
	u_long	lbrd1;
	u_long	dmdac;
	u_long	pciarb;		//100	PCI9656
	u_long	pabtadr;
} PLX_REGS;
//
//============================================================================
//
//										PCI BIOS definitions
//
typedef struct {
	u_long	signature;
	u_char	mechanism,
				present;
	u_char	minor_version,
				major_version;
	u_char	last_PCI_bus;
} PCI_PRESENT;

typedef struct {
	u_short	device_id,
				vendor_id,
				index;
	u_char	bus_no,
				device_no;		// b7:3  devive   nr
									// b2:0  function nr
} PCI_DEVICE;

typedef struct {
	u_char	prog_if,
				sub_class,
				base_class,
				dmy;
	u_short	index;
	u_char	bus_no,
				device_no;
} PCI_CLASS;

typedef struct {
	u_char	bus_no,
				device_no;
	u_short	_register;
	u_long	value;
} PCI_CONF_IO;
//
//============================================================================
//
//										PCI region and Config buffer
//
typedef struct {
	u_char		bus_no,
					device_no;
	u_char		int_line;
	u_char		revision;
	void			*r0_regs;		// 04 region 0
	u_short		r0_ioport;		// 08 case PCI IO Mapped
	struct {
		u_long	len;
		u_long	base;
		u_int		selio;			// Selector(b2:0 != 0) or IO Base(b2:0 == 0)
	} regs[C_REGION];				// 0A 14 1E 28 32 3C

	struct {
		u_int		rg_offs;			// 15:13 => region, 12:0 => offset *64k
		u_int		sel;				// Selector
	} desc[C_DESC];				// 46 additional descriptors

	u_long		xrom_len;
	u_long		xrom_base;
	u_int			xrom_sel;		// expansion ROM selector
} REGION;

typedef struct {
	u_long	ms_count;			// fÅr ZeitÅberwachungen
	char		main_menu;
	char		opr_menu;
	char		op2_menu;
	char		tmp_menu;

	REGION	region;				//010

//--------------------------- pciconf.c parameter

	char		cnf_menu;
	u_char	errlog;

	u_short	vendor_id,
				device_id,
				c_index;				//		to skip pci device
	u_short	cnfs_bus_no,		// Configuration Space
				cnfs_device_no;

//--------------------------- pcibios.c parameter

	char		pci_menu;
	char		res;

	u_short	bmen_bus_no,
				bmen_vendor_id,
				bmen_device_id,
				bmen_index,
				bmen_bcls,
				bmen_scls,
				bmen_ifcls;

	u_char	bmem_crd_bus;
	u_char	bmem_crd_dev;
	u_short	bmem_crd_addr;
	u_short	bmem_crd_sz;

	u_char	bmem_cwr_bus;
	u_char	bmem_cwr_dev;
	u_short	bmem_cwr_addr;
	u_short	bmem_cwr_sz;
	u_long	bmem_cwr_val;

//---------------------------	pcilocal.c parameter

	char		reg_menu;
	char		epr_menu;

	u_int		mver_reg;					// memory verify region
	u_long	mver_addr[C_REGION+1];
	u_int		mver_size[C_REGION+1];

	u_int		crd_reg;						// cyclic read
	u_long	crd_addr[C_REGION+1];
	u_int		crd_size[C_REGION+1];
	u_int		crd_cycles[C_REGION+1];

	u_int		cwr_reg;						// cyclic write
	u_long	cwr_addr[C_REGION+1];
	u_int		cwr_size[C_REGION+1];
	u_int		cwr_cycles[C_REGION+1];
	u_long	cwr_strt[C_REGION+1];
	u_long	cwr_inc[C_REGION+1];

	u_int		cwrvfy_reg;					// cyclic write/verify
	u_long	cwrvfy_addr[C_REGION+1];
	u_int		cwrvfy_size[C_REGION+1];
	u_long	cwrvfy_strt[C_REGION+1];
	u_long	cwrvfy_inc[C_REGION+1];
	u_long	cwrvfy_msk[C_REGION+1];

#define C_WSITEMS		8
	u_int		cwrseq_items;				// cyclic write sequence
	u_int		cwrseq_size;
	struct {
		u_int		reg;
		u_long	addr;
		u_int		wr;
		u_long	value;
	} cwrseq_objs[C_WSITEMS];

#ifdef AMCC
	u_long	mreg_values[16];
#endif
	char		save_file[80],
				load_file[80];

} PCI_CONFIG;

extern char		pci_ok;
extern char		debug;
extern char		last_key;
extern char		use_default;

extern PCI_PRESENT	bios_psnt;
extern PCI_DEVICE		pdev;
extern PCI_CONF_IO	cnf_acc;
//
//------ external functions
//
u_long mem_size(void);
u_long swap_l(u_long val);
u_short swap_w(u_short val);
void str2up(char *str);
void Writeln(void);
char *token(char str[], int *ix);
long value(char *line, int *inx, int d_base);
void timemark(void);
u_long ZeitBin(void);
void dsp_rate(u_long lp);

int WaitKey(int key);
int Readln(char *ln, u_int len);
char *Read_String(char *def, u_int len);
long Read_Num(u_long def, u_long max, u_int base);
long Read_Hexa(long def, long max);
long Read_Deci(long def, long max);
float Read_Float(float def);
void dump(void *buf, u_int lng, u_long offs, u_int flg);
char *pci_error(int err);
int MapRegions(REGION *reg);
void *pci_ptr(REGION *reg, u_short regnr, u_long offs);
int pci_attach(REGION *reg, int devid, int tst);
void WRTamcc(u_int offs, u_long val);
u_long RDamcc(u_int offs);
//
//------ xme.asm
//
int pci_bios(int fcnt, void *param);
void WPortL(u_short ioport, u_long val);
u_long RPortL(u_short ioport);

typedef struct {
	u_short	ax,bx,cx,dx;
	u_short	si,di;
} DPMIREGS;

typedef struct {
	u_long	eax,ebx,ecx,edx;
	u_long	esi,edi;
	u_short	es;
} DPMILREGS;

int DPMI_call(DPMIREGS *regs);
int DPMI_lcall(DPMILREGS *regs);
int DPMI_21call(DPMILREGS *regs);

#endif
