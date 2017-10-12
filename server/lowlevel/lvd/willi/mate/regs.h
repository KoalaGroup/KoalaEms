/*!
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * ---
 * Copyright (C) 2008, Willi Erven <w.erven@fz-juelich.de>
 */

#ifndef _REGS_H
#define _REGS_H

// ======================================================================= ==
//										register der F1 TDC Karte                    --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	int16_t		trg_lat;			// 04
	u_int16_t	trg_win;
	u_int16_t	ro_data;			// 08
	u_int16_t	f1_addr;			// 0A
#define F1_ALL		8

	u_int16_t	cr;				//	0C	2 bit
#define ICR_TMATCHING	0x01	// Trigger Matching (with time window)
#define ICR_RAW			0x02	// raw F1 data with min 19.2 us hold time
#define ICR_LETRA			0x04	// F1 b0 is edge indicator
#define ICR_EXT_RAW		0x08	// extended raw mode (2 long word per hit)

	u_int16_t	sr;				// 0E
#define IN_FIFO_ERR	0x1	// no double word

	u_int16_t	f1_state[8];	// 10
#define WRBUSY		0x01
#define NOINIT		0x02
#define NOLOCK		0x04
#define PERM_HOFL	0x08	// permanent Hit Overflow
#define HOFL		0x10	// F1 Hit FIFO Overflow
#define OOFL		0x20	// F1 Output FIFO Overflow
#define SEQERR		0x40	// F1 Daten falsch (Reihenfolge)
#define IOFL		0x80	// FPGA Input FIFO Overflow
#define F1_INPERR	(IOFL|SEQERR|OOFL|HOFL)

	u_int16_t	f1_reg[16];		// 20
	u_int16_t	jtag_csr;		// 40
#define JT_TDI				0x001
#define JT_TMS				0x002
#define JT_TCK				0x004
#define JT_TDO				0x008
#define JT_ENABLE			0x100
#define JT_AUTO_CLOCK	0x200
#define JT_SLOW_CLOCK	0x400

	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int32_t	res48;
	u_int16_t	f1_range;		// 4C
	u_int16_t	res4E;
	u_int16_t	hitofl[4];		// 50
	u_int16_t	res58[12];
	u_int16_t	res70[7];
	u_int16_t	ctrl;				// 7E
#define PURES		0x02
#define SYNCRES	0x04
#define TESTPULSE	0x08

} TDCF1_RG;						// 80
//
// -------------------------- F1 TDC broadcast register ------------------ --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						any offline unit
	int16_t		trg_lat;			// 04	value
	u_int16_t	trg_win;			//		value
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	f1_addr;			//		value
	u_int16_t	cr;				//	0C	value
	u_int16_t	f1_error;		//	0E	sel clr		F1 error mask
	u_int16_t	f1_state[8];	// 10	sel clr
	u_int16_t	f1_reg[16];		// 20	value
	u_int16_t	jtag_csr;		// 40	value

	u_int16_t	res42[3];
	u_int32_t	trigger;			// 48 fuer die Software ohne Bedeutung
	u_int16_t	f1_range;		// 4C	value
	u_int16_t	res4E;
	u_int16_t	hitofl;			// 50	clear
	u_int16_t	res52[15];
	u_int16_t	res70[7];
	u_int16_t	ctrl;				// 7E	value
#define MRESET		0x01
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define TSTART		0x80

} TDCF1_BC;						// 80
//
// ======================================================================= ==
//										register structure of the GPX TDC card       --
//										                                             --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	int16_t		trg_lat;			// 04
	u_int16_t	trg_win;
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;
	u_int16_t	cr;				//	0C	4 bit RW
//define ICR_TMATCHING	0x01	// Trigger Matching (with time window)
//define ICR_RAW			0x02	// raw F1 data with min 19.2 us hold time
//define ICR_LETRA		0x04	// F1 b0 is edge indicator
//define ICR_EXT_RAW		0x08	// extended raw mode (2 long word per hit)
#define ICR_NO_EIND		0x10	// no error indication

	u_int16_t	sr;				// 0E
// b0:

	u_int16_t	gpx_int_err;	// 10
#define GPX_ERRFL		0xFF

	u_int16_t	gpx_emptfl;		// 12
	u_int16_t	gpx_dcm_shift;	// 14
	u_int16_t	res12[5];		// 16
	u_int32_t	gpx_data;		// 20
	u_int16_t	gpx_seladdr;	// 24
	u_int16_t	gpx_dac;			// 26
	u_int16_t	res26[4+1*8];	// 28
	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[2];
	u_int32_t	gpx_range;		// 4C
	u_int16_t	res50[2*8+7];
	u_int16_t	ctrl;				// 7E not used

} TDCGPX_RG;					// 80
//
// -------------------------- GPX broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						b0: any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	any_error;		//	0E					gen error
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48 trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
// b0: sgl shot, master reset

} TDCGPX_BC;					// 80
//
// ======================================================================= ==
//										register der synch Master Karte              --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	u_int16_t	sr2;				// 04
	u_int16_t	res06;
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	sm_cr;			//	0C
#define ST_RING		0x0001
#define ST_GO			0x0002
#define ST_TAW_ENA	0x0004
#define B_ST_AUX0		4
#define ST_AUX_MASK	0x00F0
#define ST_T4_UTIL	0x0100

	u_int16_t	sm_sr;			// 0E
#define CS_GO_RING	0x0010
#define CS_INH			0x0080
#define CS_EOC			0x1000
#define CS_SI_RING	0x2000
#define CS_TDT_RING	0x8000

	u_int16_t	trg_inh;			// 10
	u_int16_t	tlg_inh;			// 12
	u_int16_t	trg_inp;			// 14
	u_int16_t	trg_acc;			// 16
#define CS_TRIGGER	0x000F

	u_int16_t	res18;			// 18
	u_int16_t	sm_fclr;			// 1A
	u_int16_t	res1C[2];		// 1C
	u_int16_t	pwidth[9];		// 20
	u_int16_t	setpoti;			// 32
	u_int32_t	sm_tmc;			// 34
	u_int16_t	res38[2];		// 38
	u_int32_t	sm_tdt;			// 3C
	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48;
	u_int16_t	sm_fcat;			// 4A
	u_int32_t	sm_evc;			// 4C
	u_int16_t	res50[2*8+7];
	u_int16_t	sm_ctrl;			// 7E
#define RST_TRG	0x02

} SYMST_RG;						// 80
//
// -------------------------- sync master broadcast register ------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	f1_error;		//	0E
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48	trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
//#define TSTART		0x80

} SYMST_BC;						// 80
//
// ======================================================================= ==
//										register der synch slave Karte               --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	u_int16_t	bsy_tmo;			// 04
	u_int16_t	bsy_state;		// 06
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	cr;				//	0C
#define TS_RUN		0x100

	u_int16_t	sr;				// 0E

	u_int16_t	trg_in;			// 10
	u_int16_t	sram_addr;		// 12
	u_int16_t	sram_data;		// 14
	u_int16_t	res16[5+2*8];	// 16
	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[4+2*8+7];
	u_int16_t	ctrl;				// 7E

} SYSLV_RG;						// 80
//
// -------------------------- sync slave broadcast register -------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	f1_error;		//	0E
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48	trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E

} SYSLV_BC;						// 80
//
// ======================================================================= ==
//										register der VERTEX VA32TA Karte                    --
// ======================================================================= ==
#define SEQ_VACLK			0
#define SEQ_FRAME			1
#define SEQ_HOLD			2
#define SEQ_TSTON			3
#define SEQ_CS				4
#define SEQ_HOLD_B		5
#define SEQ_RESET			6
#define SEQ_DTP 			7
#define SEQ_BUSY			8
//
//-- LV/HV register
//
typedef struct {
	u_int32_t	swreg;					// 20
	u_int16_t	counter[2];				// 24
	u_int16_t	lp_counter[2];			// 28
	u_int16_t	dgap_counter[2];		// 2C
	u_int16_t	clk_delay;				// 30
	u_int16_t	nr_chan;					//	32
	u_int16_t	noise_thr;				//	34
	u_int16_t	comval;					//	36
	u_int16_t	comcnt;					//	38
	u_int16_t	poti;						//	3A
	u_int16_t	dac;						//	3C
	u_int16_t	reg_cr;					//	3E
	u_int16_t	reg;						//	40
	u_int16_t	adc_value;				//	42
	u_int16_t	adc_otr_cnt;			// 44
	u_int16_t	udw;						// 46	user data word
} VTX_GRP;									// 48

typedef struct {
	u_int16_t	ADR_IDENT;				// 00
	u_int16_t	ADR_SERIAL;				// 02
	u_int16_t	seq_csr;					// 04
#define SEQ_CSR_LV_HALT	0x0001
#define SEQ_CSR_HV_HALT	0x0100

	u_int16_t	aclk_par;				// 06
	u_int16_t	ADR_FIFO;				// 08
	u_int16_t	res0A;					// 0A
	u_int16_t	cr;						// 0C
#define VCR_RUN		0x0001
#define VCR_RAW		0x0002
#define VCR_VERB		0x0004
#define VCR_TM_MODE	0x0008

#define VCR_IH_LV		0x0010
#define VCR_IH_HV		0x0020
#define VCR_IH_TRG	0x0040
#define VCR_VA_TRG	0x0080
#define VCR_MON		0x0100
#define VCR_UDW		0x0800
#define VCR_NOCOM		0x1000

	u_int16_t	ADR_SR;					// 0E

	u_int32_t	ADR_SRAM_ADR;			// 10
#define C_HV_SRAM_OFFSET	0x080000
#define C_HV_TRAM_OFFSET	0x100000

	u_int16_t	ADR_SRAM_DATA;			// 14
	u_int16_t	res16;					// 16
	u_int32_t	ADR_RAMT_ADR;			//	18
#define BLOCK1_HV				0x100000

	u_int16_t	ADR_RAMT_DATA;			//	1C
	u_int16_t	res1E;					//	1E
	VTX_GRP		lv;						// 20
	u_int16_t	res48[2];				// 48
	u_int16_t	ADR_SCALING;			// 4C
	u_int16_t	ADR_DIG_TST;			// 4E
	VTX_GRP		hv;						// 50
	u_int32_t	ADR_JTAG_DATA;			// 78
	u_int16_t	ADR_JTAG_CSR;			// 7C
	u_int16_t	ADR_ACTION;				// 7E
//#define MRESET		0x01
//#define PURES		0x02
//#define SYNCRES		0x04
//#define TESTPULSE	0x08

#define R_CNTRSTLV		0x0100
#define R_CNTRSTHV		0x0200
#define R_CLEARERROR		0x0400
#define R_RESETSEQ		0x0800

} VTEX_RG;									// 80
//
// ======================================================================= ==
//										register der VERTEX MATE3 Karte                    --
// ======================================================================= ==
#define SEQ3_VACLK		0
#define SEQ3_FRAME		1
#define SEQ3_HOLD			2
#define SEQ3_TSTON		3
#define SEQ3_CS			4
#define SEQ3_HOLD_B		5
#define SEQ3_RESET		6
#define SEQ3_DTP 			7
#define SEQ3_BUSY			8
//
//-- LV/HV register
//
typedef struct {
	u_int32_t	swreg;				// 20
	u_int16_t	counter[2];			// 24
	u_int16_t	lp_counter[2];		// 28
	u_int16_t	dgap_counter[2];	// 2C
	u_int16_t	clk_delay;			// 30
	u_int16_t	nr_chan;				// 32
	u_int16_t	noise_thr;			// 34
	u_int16_t	comval;				// 36
	u_int16_t	comcnt;				// 38
	u_int16_t	poti;					// 3A
	u_int16_t	dac;					// 3C
	u_int16_t	i2c_csr;				// 3E
#define I2C_CHIPS			0x001F	// number of chips mask
#define I2R_BSY_MRST		0x0100	// RD:busy; WR:master reset
#define I2R_DAV_IFRST	0x0200	// RD:data available; WR:input FIFO reset
#define I2R_ERR_RST		0x0400	// RD:I2C error; WR:error reset

#define I2R_SETUP			0x8000	// WO:setup I2C bus, determine number of chips

	u_int16_t	i2c_data;			// 40
	u_int16_t	adc_value;			// 42
	u_int16_t	adc_otr_cnt;		// 44
	u_int16_t	udw;					// 46 user data word
} VTX3_GRP;								// 48

typedef struct {
	u_int16_t	ADR_IDENT;			// 00
	u_int16_t	ADR_SERIAL;			// 02
	u_int16_t	seq_csr;				// 04
//#define SEQ_CSR_LV_HALT	0x0001
//#define SEQ_CSR_HV_HALT	0x0100

	u_int16_t	aclk_par;			// 06
	u_int16_t	ADR_FIFO;			// 08
	u_int16_t	res0A;				// 0A
	u_int16_t	cr;					// 0C
//#define VCR_RUN			0x0001
//#define VCR_RAW			0x0002
//#define VCR_VERB		0x0004
//#define VCR_TM_MODE	0x0008

//#define VCR_IH_LV	   0x0010
//#define VCR_IH_HV		0x0020
//#define VCR_IH_TRG		0x0040
//#define VCR_VA_TRG		0x0080
//#define VCR_MON			0x0100
//#define VCR_UDW			0x0800
//#define VCR_NOCOM		0x1000
#define VCR_MI2C			0x2000	// maintenance I2C bus

	u_int16_t	ADR_SR;				// 0E

	u_int32_t	ADR_SRAM_ADR;		// 10
//#define C_HV_SRAM_OFFSET	0x080000
//#define C_HV_TRAM_OFFSET	0x100000

	u_int16_t	ADR_SRAM_DATA;		// 14
	u_int16_t	res16;				// 16
	u_int32_t	ADR_RAMT_ADR;		//	18
//#define BLOCK1_HV				0x100000

	u_int16_t	ADR_RAMT_DATA;		// 1C
	u_int16_t	res1E;				// 1E
	VTX3_GRP		lv;					// 20
	u_int16_t	res48[2];			// 48
	u_int16_t	ADR_SCALING;		// 4C
	u_int16_t	ADR_DIG_TST;		// 4E
	VTX3_GRP		hv;					// 50
	u_int32_t	ADR_JTAG_DATA;		// 78
	u_int16_t	ADR_JTAG_CSR;		// 7C
	u_int16_t	ADR_ACTION;			// 7E
//	#define MRESET		0x01
//	#define PURES		0x02
//	#define SYNCRES	0x04
//	#define TESTPULSE	0x08

//#define	R_CNTRSTLV     		0x0100
//#define	R_CNTRSTHV				0x0200
//#define	R_CLEARERROR   		0x0400
//#define	R_RESETSEQ     		0x0800

} VTEX3_RG;								// 80
//
//-- VERTEX broadcast register VA32TA and MATA3
//
typedef struct {						// Bus FPGA
	u_int16_t	ADR_CARD_ONL;			// 00
	u_int16_t	ADR_CARD_OFFL;			// 02
	u_int16_t	res02;					// 04
	u_int16_t	res03;         		// 06
	u_int16_t	ADR_FIFO;				// 08
	u_int16_t	res05;	        		// 0A
	u_int16_t	res06;					//	0C
	u_int16_t	ADR_ERROR; 				// 0E
	u_int16_t	res08[28];				// 10
	u_int16_t	ADR_TRIGGER_L;			// 48 => used by system controller
	u_int16_t	ADR_TRIGGER_H;
	u_int16_t	res25[25];				// 4A
	u_int16_t	ADR_ACTION;				// 7E Master Reset
} VTEX_BC;
//
// ======================================================================= ==
//										register structure of the fast QDC test card --
//										developed by Pawel Marciniewski              --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	u_int16_t	mean_tol;		// 04
	u_int16_t	trig_level;		// 06
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define QFT_CLKPOL	0x008	// DAC clock polarity
#define QFT_ADCPOL	0x040	// inverse ADC data
#define QFT_RAW		0x080	// insert ADC data
#define QFT_SGRD		0x100	// single gradient

	u_int16_t	sr;				// 0E
// b0:	FIFO read error, WR: selective clear
// b1: NIM input signal

	u_int16_t	res10[2];		// 10
	u_int16_t	mean_level;		// 14
	u_int16_t	res16;			// 16
	u_int16_t	mean_noise;		// 18
	u_int16_t	res1A;			// 1A
	int16_t		latency;			// 1C
	u_int16_t	window;			// 1E
	u_int16_t	res20[2*8];		// 20
	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[4+2*8+7];
	u_int16_t	ctrl;				// 7E not used

} FQDCT_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						b0: any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	any_error;		//	0E not used
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48	trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
// b0: sgl shot, master reset

} FQDCT_BC;						// 80
//
// ======================================================================= ==
//										register structure of the fast QDC card      --
//										16 channel, developed by Pawel Marciniewski  --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	int16_t		latency;			// 04
	u_int16_t	window;			// 06
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define Q_VERBOSE		0x008	// all analysed data
#define Q_ADCPOL		0x010	// inverse ADC data
#define Q_SGRD			0x020	// single gradient
#define QF_TESTON		0x040	// test pulse on
#define QF_PWROFF		0x080	// ADC power off
#define Q_F1			0x100	// F1 mode

	u_int16_t	sr;				// 0E
// b0: FIFO read error, WR: selective clear
// b1: ADC power fail

	u_int16_t	trig_level;		// 10
	u_int16_t	anal_ctrl;		// 12
	u_int16_t	inp_ovr;			// 14
	u_int16_t	dcm_shift;		// 16
	u_int16_t	cha_inhibit;	// 18
	u_int16_t	cha_raw;			// 1A
	u_int16_t	iw_start;		// 1C
	u_int16_t	iw_length;		// 1E
	u_int16_t	mean_noise[16];// 20
#define q_thresh	mean_noise

	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[2];
	u_int16_t	i_length;		// 4C
	u_int16_t	res4E;
	u_int32_t	tdc_range;		// 50
	u_int16_t	res54[6];
	u_int16_t	mean_level[15];// 60
	u_int16_t	ctrl;				// 7E not used

} FQDC_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						b0: any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	any_error;		//	0E not used
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48	trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
// b0: sgl shot, master reset

} FQDC_BC;						// 80
//
// ======================================================================= ==
//										register structure of the slow QDC card      --
//										test piggy pack with VERTEX base card        --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	u_int16_t	mean_tol;		// 04
	u_int16_t	trig_level;		// 06
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	cr;				//	0C	8 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define QST_CLKPOL	0x008	// DAC clock polarity
#define QST_DIS0		0x010	// disable channel 0
#define QST_DIS1		0x020	// disable channel 1
#define QST_ADCPOL	0x040	// inverse ADC data
#define QST_RAW		0x080	// insert ADC data
#define QST_SGRD		0x100	// single gradient

	u_int16_t	sr;				// 0E
// b0:	FIFO read error, WR: selective clear
// b1: NIM input signal

	u_int16_t	dac[2];			// 10
	u_int16_t	mean_level[2];	// 14
	u_int16_t	mean_noise[2];	// 18
	int16_t		latency;			// 1C
	u_int16_t	window;			// 1E
	u_int32_t	qram_addr;		// 20
	u_int16_t	qram_data;		// 24
	u_int16_t	res26;			// 26
	u_int32_t	tram_addr;		// 28
	u_int16_t	tram_data;		// 2C
	u_int16_t	res2E[1+8];		// 2E
	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[4+2*8+7];
	u_int16_t	ctrl;				// 7E not used

} SQDCT_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						b0: any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	any_error;		//	0E not used
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48	trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
// b0: sgl shot, master reset

} SQDCT_BC;						// 80
//
// ======================================================================= ==
//										register structure of the slow QDC card      --
//										16 channel                                   --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	serial;			// 02
	int16_t		latency;			// 04
	u_int16_t	window;			// 06
	u_int16_t	ro_data;			// 08
	u_int16_t	res0A;			// 0A
	u_int16_t	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define Q_VERBOSE		0x008	// all analysed data
#define Q_ADCPOL		0x010	// inverse ADC data
#define Q_SGRD			0x020	// single gradient

	u_int16_t	sr;				// 0E
// b0:	FIFO read error, WR: selective clear

	u_int16_t	trig_level;		// 10
	u_int16_t	anal_ctrl;		// 12
	u_int16_t	inp_ovr;			// 14
	u_int16_t	dcm_shift;		// 16
	u_int16_t	cha_inhibit;	// 18
	u_int16_t	cha_raw;			// 1A
	u_int16_t	iw_start;		// 1C
	u_int16_t	iw_length;		// 1E
	u_int16_t	mean_noise[16];// 20
#define q_thresh	mean_noise

	u_int16_t	jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	jtag_data;		// 44
	u_int16_t	res48[2];
	u_int16_t	i_length;		// 4C
	u_int16_t	res4E;
	u_int32_t	tdc_range;		// 50
	u_int16_t	res54[6];
	u_int16_t	mean_level[15];// 60
#define dac_data	mean_level

	u_int16_t	ctrl;				// 7E not used

} SQDC_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_int16_t	card_onl;		// 00 address		online units
	u_int16_t	card_offl;		//						b0: any offline unit
	u_int16_t	res04[2];
	u_int16_t	ro_data;			// 08					data available
	u_int16_t	res0A[2];
	u_int16_t	any_error;		//	0E not used
	u_int16_t	res10[3*8+4];	// 10
	u_int16_t	busy;				// 48 trigger		=> used by system controller
	u_int16_t	res4A[3+2*8+7];// 4A
	u_int16_t	ctrl;				// 7E
// b0: sgl shot, master reset

} SQDC_BC;						// 80
//
// ======================================================================= ==
//										JTAG analysing tool                          --
// ======================================================================= ==
//
typedef struct {
	u_int16_t	ident;			// 00
	u_int16_t	res02;			// 02
	u_int32_t	timer;			// 04
	u_int32_t	ro_data;			// 08
	u_int16_t	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable

	u_int16_t	sr;				// 0E
// b0:	FIFO read error, WR: selective clear

	u_int16_t	tck_count;		// 10
	u_int16_t	res12;			// 12
	u_int16_t	acc_state;		// 14
	u_int16_t	res16[5];		// 16
	u_int16_t	res20[6*8];		// 20

} JTAG_RG;						// 80
//
// ======================================================================= ==
//										register einer Controller Karte              --
// ======================================================================= ==
//
// -------------------------- register einer Controller Karte ------------ --
//
//	Diese Struktur gilt auch fuer die lokalen Register des Master Controllers
//
typedef struct {
	u_int16_t  ident;			// 00
	u_int16_t	serial;			// 02
	u_int16_t  res04;
	u_int16_t  res06;			// 06
	u_int16_t  cr;				// 08
//#define MCR_DAQ				0x03	// mask for DAQ mode
// #define MCR_DAQTRG		0x01	// automatische Erfassung der Input Daten
// #define MCR_DAQRAW		0x02	// Input Ueberwachung mit Interrupt Meldung
//#define MCR_DAQ_TEST		0x04	// enable test pulse only
//#define MCR_DAQ_HANDSH	0x08	// event synchronisation by handshake
//#define MCR_LOC_EVC		0x10	// use local event counter
//#define MCR_NO_F1STRT		0x40	// synchron mode durch F1 Start Signal

	u_int16_t  sr;				//	0A	siehe auch f1_state Definitionen
//							0x0008	// F1 Trigger FIFO Overflow
#define SC_F1_ERR		0x0040	// F1 error (no lock) Summenfehler Inputkarten
#define SC_TRG_LOST	0x0080	// Trigger overrun
#define SC_DATA_AV	0x0100	// FIFO Data available
#define SC_EVENT		0x0200	// Main Trigger Time available
#define SC_F1_DERR	0x8000	// F1 data error

	u_int32_t	event_nr;		// 0C
	u_int16_t  ro_data;			// 10	Blocktransfer aus Event FIFO
	u_int16_t	res12;
	u_int16_t	ro_delay;		// 14	Delay nach Event fuer ReadOut Input Cards
	u_int16_t	res16;
	u_int32_t	trg_data;		// 18 Trigger Time Stamp
	u_int32_t	ev_info;			// 1C Adresse und Laenge des letzten Trigger Events
	u_int16_t  f1_reg[16];		// 20
	u_int16_t	x_jtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	x_jtag_data;	// 44
	u_int16_t  res48[27];
	u_int16_t  ctrl;				// 7E
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define F1START	0x10

} SYSCONF1_RG;					// 80
//
// -------------------------- broadcast register der Controller Karten --- --
//										nur Front Bus Controller
//
//	Die scan Moeglichkeit am Front Bus ist eingeschraenkt.
//		Die erste Version hat keine scan Moeglichkeit, alle Infos erscheinen auf Bit0
//		die zweite Version kann die ersten 4 Adressen scannen, die restlichen
//			erscheinen alle auf Bit 4
//
typedef struct {
	u_int16_t  card_onl;		// 00	RW
	u_int16_t  card_offl;		//	02	RO
	u_int16_t  transp;			// 04	WO
	u_int16_t  res06;			// 06 WO
	u_int16_t  cr;				// 08 WO
	u_int16_t  sr;				//	0A	WR, RD:f1_err -> any DAQ F1 error
#define f1_err	sr

	u_int32_t	event_nr;		// 0C WO
	u_int16_t  ro_data;			// 10	RO Data available
	u_int16_t	fifo_pf;			//	12	RO	Event FIFO partial full
	u_int16_t	ro_delay;		// 14	WO Delay nach Event fuer ReadOut Input Cards
	u_int16_t	res16;
	u_int16_t  res18[4];
	u_int16_t  f1_reg[16];		// 20	WO
	u_int16_t	jtag_csr;		// 40	WO
	u_int16_t	res42;
	u_int16_t  res44[29];
	u_int16_t  ctrl;				// 7E WO
//	MRESET
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
//	F1START

} COUPLER_BC;					// 80
//
// ======================================================================= ==
//										register of the system controller            --
// ======================================================================= ==
//
//	Diese Struktur gilt auch fuer die lokalen Register des System Controllers mit GPX
//
typedef struct {
	u_int16_t  ident;			// 00
	u_int16_t  serial;			// 02
	u_int16_t  res04;
	u_int16_t  res06;			// 06
	u_int16_t  cr;				// 08
#define MCR_DAQ			0x03	// mask for DAQ mode
 #define MCR_DAQTRG		0x01	// automatische Erfassung der Input Daten
 #define MCR_DAQRAW		0x02	// Input Ueberwachung mit Interrupt Meldung
#define MCR_DAQ_TEST		0x04	// enable test pulse only
#define MCR_DAQ_HANDSH	0x08	// event synchronisation by handshake
#define MCR_LOC_EVC		0x10	// use local event counter
#define MCR_F1TIME		0x20	// F1 time format
#define MCR_NO_F1STRT	0x40	// synchron mode durch F1 Start Signal

	u_int16_t  sr;				//	0A	siehe auch f1_state Definitionen
//							0x0008	// F1 Trigger FIFO Overflow
#define SC_F1_ERR		0x0040	// F1 error (no lock) Summenfehler Inputkarten
#define SC_TRG_LOST	0x0080	// Trigger overrun
#define SC_DATA_AV	0x0100	// FIFO Data available
#define SC_EVENT		0x0200	// Main Trigger Time available
#define SC_F1_DERR	0x8000	// F1 data error

	u_int32_t	event_nr;		// 0C
	u_int16_t	ro_data;			// 10	Blocktransfer aus Event FIFO
	u_int16_t	res12;
	u_int16_t	ro_delay;		// 14	Delay nach Event fuer ReadOut Input Cards
	u_int16_t	res16;
	u_int32_t	trg_data;		// 18 Trigger Time Stamp
	u_int32_t	ev_info;			// 1C Adresse und Laenge des letzten Trigger Events
	u_int16_t	gpx_reg;			// 20
	u_int16_t	res22;
	u_int32_t	gpx_data;		// 24
	u_int32_t	gpx_range;		// 28
	u_int16_t	ro_blen[2];		// 2C	KWS2
	u_int16_t	ro_srcpat;		// 30	KWS2
	u_int16_t	res2C[7];		// 32
	u_int16_t	xjtag_csr;		// 40
	u_int16_t	res42;
	u_int32_t	xjtag_data;		// 44
	u_int16_t	res48[27];
	u_int16_t	ctrl;				// 7E
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define F1START	0x10

} SYSCON_RG;					// 80

#endif
