/******* header file *********************************************************
**
** Definitions for RAL readout programs
**
** Name:    ralrout.h
** Datum:   23.09.97	implementation
**
******************************************************************************
*/
#define REGS			64			// # register of RAL111
#define MAX_ROWS		64			// # of rows (# of RAL111) per column
#define MAX_COLUMNS	3
#define FRAM_LEN		0x04000	// filter RAM length

#define RD_FIFO		((u_int)( 0 <<9) |0)
#define RD_FAST		((u_int)( 5 <<9) |0)
#define TST_FIFO		((u_int)(27 <<9) |0)
#define WR_DATA		((u_int)(16 <<9) |0)
#define WR_STA1		((u_int)(17 <<9) |0)
#define WR_STA2		((u_int)(17 <<9) |1)
#define GEN_CLR		((u_int)( 9 <<9) |0)
#define TST_LAM		((u_int)( 8 <<9) |0)
#define TSTCL_LAM		((u_int)(10 <<9) |0)
#define START			((u_int)(25 <<9) |0)
#define CLOCK			((u_int)(25 <<9) |1)
#define STROBE			((u_int)(25 <<9) |2)
#define RESET_			((u_int)(25 <<9) |3)
#define RUN_ROUT		((u_int)(25 <<9) |4)
#define WR_RAM			((u_int)(16 <<9) |1)
#define RD_RAM			((u_int)(27 <<9) |1)
#define TST_BUSY		((u_int)(27 <<9) |2)
#define TST_REQU		((u_int)(27 <<9) |3)
#define WR_VSN			((u_int)(16 <<9) |2)
//
//------ Status Register 1
//
#define TRO		0x01
#define EFB		0x02
#define ELAM	0x04
#define BWS		0x08
#define LPOS	0x10
#define DZS		0x20
//
//------ Status Register 2
//
#define CTL_TST_ONE	1
#define CTL_LD_TST	2
#define CTL_TST_ZERO	3
#define CTL_LD_DAC	4
#define CTL_ROUT		5
#define CTL_RESET		7

#define DXST		0x10
#define STP_FCLK	0x20
/*
-- data input register
*/
#define B_ROW		4
#define M_ROW		0x3F
#define B_COLUMN	10
#define M_COLUMN	0x0F
#define B_NULL		16
#define B_NEXT		17
//
typedef struct {
	u_short	romod_n;
	u_short	vsn;
	u_short	act_col;
	u_short	columns;
	struct {
		u_short	rows;						/* # of RAL111 chips */
		u_char	dac[MAX_ROWS];
		u_short	test_reg[MAX_ROWS];
		u_short	row_fltr[MAX_ROWS];	/* bad wires filter */
	} column[MAX_COLUMNS];
} CONF_ROMOD;

