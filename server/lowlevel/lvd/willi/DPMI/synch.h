// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	sync.h -- Test for Sync/Trigger Module test

#define SYNC_DEVICE_ID	0x812F		// Sync/Trigger Module

#define SM_CSR		0x00
#define SM_CR		0x04
#define SM_CVT		0x08
#define SM_PUWI	0x0C
#define SM_TMC		0x10
#define SM_TDT		0x14
#define SM_FCAT	0x18
#define SM_EVC		0x20
#define SM_REJ		0x24
#define SM_GAP		0x28

//----------------------------------------------------------------------------
// CSR and CR bit definitions
//----------------------------------------------------------------------------

#define ST_MASTER		0x00000001L
#define ST_GO			0x00000002L
#define ST_T4_UTIL	0x00000004L
#define ST_TAW_ENA	0x00000008L
#define ST_AUX0		0x00000010L
#define ST_AUX1		0x00000020L
#define ST_AUX2		0x00000040L
#define ST_AUX_MASK	0x00000070L
#define ST_AUX0_RST	0x00000080L
#define IE_AUX_I0		0x00000100L
#define IE_AUX_I1		0x00000200L
#define IE_AUX_I2		0x00000400L
#define IE_AUX_I		0x00000700L
#define IE_EOC			0x00000800L
#define IE_SI_RING	0x00001000L
#define IE_TRIGGER	0x00002000L
#define IE_GAP_DATA	0x00004000L

#define SET_CR(cr)	((~(u_long)(cr) <<16) |((cr) &0xFFFF))

//----------------------------------------------------------------------------
// CSR bit definitions
//----------------------------------------------------------------------------

#define CS_TRIGGER1	0x00010000L
#define CS_TRIGGER2	0x00020000L
#define CS_TRIGGER3	0x00040000L
#define CS_TRIGGER4	0x00080000L
#define CS_TRIGGER	0x000F0000L
#define CS_GO_RING	0x00100000L
#define CS_SI			0x00400000L
#define CS_INH			0x00800000L
#define CS_AIX_IN0	0x01000000L
#define CS_AIX_IN1	0x02000000L
#define CS_AIX_IN2	0x04000000L
#define CS_AIX_IN		0x07000000L
#define CS_EOC			0x08000000L
#define CS_SI_RING	0x10000000L
#define CS_RVS			0x20000000L
#define CS_GAP_DATA	0x40000000L
#define CS_TDT_RING	0x80000000L

#define CS_SC_RVS			0x01000000L
#define CS_SC_AUX_INT	CS_AIX_IN		// clear auxiliary interrupt (mailbox)
#define CS_SC_TRG_INT	0x20000000L		// clear trigger interrupt (mailbox)
#define CS_RST_TRG		0x80000000L		// reset TRIGGER, clear EOC

//----------------------------------------------------------------------------
// Mailbox #4 definitions
// bit 28:24 => CS_SI_RING:CS_AIX_IN0
//----------------------------------------------------------------------------

#define TRIG_VALID	0x20000000L		// end of FCAT

//----------------------------------------------------------------------------
// Trigger GAP FIFO definitions
//----------------------------------------------------------------------------

#define GAP_OVERRUN	0x20000000L
#define GAP_MORE		0x40000000L
#define GAP_NO_GAP	0x80000000L

//----------------------------------------------------------------------------
// Device register region
//----------------------------------------------------------------------------

typedef struct {
	u_long	sm_csr,
				sm_cr,
				sm_cvt,
				sm_puwi,
				sm_tmc,
				sm_tdt,
				sm_fcat,
				sm_free0,
				sm_evc,
				sm_rej,
				sm_gap;
} SYNC_REGS;

