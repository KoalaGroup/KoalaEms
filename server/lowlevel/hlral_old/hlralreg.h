/*
 * --------- Control Byte (b5/b4 = 00)
 */
#define CTL_TST_ONE		0x1	/* Test, set all channel at STROBE */
#define CTL_LD_TST		0x2	/* Load test pattern,
					   set channel as pattern */
#define CTL_TST_ZERO		0x3	/* Test, set non channel at STROBE */
#define CTL_LD_DAC		0x4	/* Load DAC register */
#define CTL_ROUT		0x5
#define CTL_RESET		0x7	/* Reset RAL111 and Coupler */
#define STP_FCLK		0x8	/* Bit ID, stop 50MHz input clock */
/*
 *--------- Data Byte (b5/b4 = 01)
 */
#define SET_DREG		0x10	/* data register for load (DAC, TEST) */
/*
 *--------- Command Byte (b5 = 1)
 */
#define CLOCK			0x20	/* send clock pulse */
#define STROBE			0x21	/* send strobe pulse */
#define STATE_REQ		0x22	/* request state */
#define ROUT_RUN		0x23	/* run readout (still activated) */
#define ROUT_DIRECT		0x24	/* run readout */
#define ROUT_ACT_NORM		0x25	/* activate normal readout */
#define ROUT_ACT_TEST		0x26	/* activate test readout */
/*
 *--------- Receive Bytes
 */
#define RxDATA			0x30	/* Received Data ID */
#define RxRO_DATA		0x40	/* Bit ID for ReadOut Data */
#define RxRO_NEXT		0x20	/* Bit ID to increment the ROW */
#define RxRO_NULL		0x10	/* Bit ID for non Data */

#define HLRAL_MAX_COLUMNS 15
#define HLRAL_MAX_ROWS 64
#define HLRAL_CHAN_BORD 16
