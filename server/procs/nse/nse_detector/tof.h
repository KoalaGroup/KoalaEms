/* ******* TOF.h  IN15   definitions ***********/

/* channel cards */

#define CH_CARD_NB 1		/* channel card number 			  */
#define DET_NB     1024         /* detector number maxi                   */
				/* VME address detector memory		  */

#define AD_C1       0xFFE00000  /* card address 1			  */
#define AD_C2       0xFFE80000  /*  "      "    2                         */
#define AD_C3       0xFFE80000
#define AD_C4       0xFFE80000
#define AD_C5       0xFFE80000
#define AD_C6       0xFFE80000
#define AD_C7       0xFFE80000
#define AD_C8       0xFFE80000

#define DEB_INCR_O  0x6000/2	/* offset incrementation value in words    */
#define DEF_W_LENGTH  1		/* length word = 32 bits		   */
#define PRO_OF_CHA  0x4000	/* protocol offsset			   */

/* time generator card */

#define T_BA	    0xFFF40000	/* VME address of timer 		   */
#define PG_COM      PRO_OF_TIM>>8      /* page number of protocol page 	   */
#define DPWR_O      0xa/2       /* DPWR offset in words			   */
#define TRIG_VAL_O  (0x100+0x10)/2 /* trigger value offset in words	   */
#define TRIG_NB	    64		/* trigger value number			   */
#define BASE_CLOCK  8		/* base clock , normaly 8 Mhz		   */
#define DEF_CHWIDT  4		/* variable channel width (not use yet)    */
#define PRO_OF_TIM  0x1000	/* protocol offset                         */
#define MEM_SIZE    0x20000     /* card channel memory size                */

/* time generator and channel cards */

#define MULTI_DET   TRUE	/* TRUE if multidetector		   */
#define PRO_O	    0x4000	/* protocol offset 			   */
#define WAIT_MAX    2		/* maxi time executed function (sec.)      */

/* program definitions */

#define B_S	1024		/* buffer size for sum and spectra */
#define MAX_RES 1024            /* maxi resoltution time           */     
