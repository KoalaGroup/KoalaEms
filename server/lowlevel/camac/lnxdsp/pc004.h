/* 	$ZEL: pc004.h,v 1.2 2002/10/18 16:46:28 wuestner Exp $	 */

#include <errno.h>

#define PC004_MAJOR  15

#define PC004_RETURN sizeof(struct PC004_DATA_TYPE)	/*number of bytes returned by PC004_read*/
#define PC004_TRUE 1
#define PC004_FALSE 0
#define PC004_MAX 4					/*Max number of PC004's*/

/* -------------------------------------------------------------------------------------------- */
/* Hardware stuff										*/
/* -------------------------------------------------------------------------------------------- */
#define PC004_PORT 0x240		/*io port for PC004 operations*/
#define PC004_IRQ  0x00A
#define PC004_DMA  0x001

/* -------------------------------------------------------------------------------------------- */
/* Checking											*/
/* -------------------------------------------------------------------------------------------- */

#if PC004_IRQ == 3
#  define PC004_IRQ_MASK 0x10
#elif PC004_IRQ == 5
#  define PC004_IRQ_MASK 0x20
#elif PC004_IRQ == 10
#  define PC004_IRQ_MASK 0x80
#else
#error "Supported IRQ = 3, 5, 10."
#endif

#if PC004_DMA == 1
#  define PC004_DMA_MASK 0x01
#elif PC004_DMA == 3
#  define PC004_DMA_MASK 0x04
#else
#error "Supported DMA = 1, 3."
#endif




/* -------------------------------------------------------------------------------------------- */
/* CAMAC-commands:										*/
/* -------------------------------------------------------------------------------------------- */
#define PC004_CRATE 		0x01  /* not supported */
#define PC004_CAMI		0x02
#define PC004_CAMO		0x03
#define PC004_CAMCL		0x04
#define PC004_CAMCLC            0x05
#define PC004_CAMCLI0           0x06
#define PC004_CAMCLI1           0x07
#define PC004_CAMCLZ            0x08
#define PC004_DMASET		0x09  /* not supported */
#define PC004_DMAI		0x0A  /* not supported */
#define PC004_DMAO		0x0B  /* not supported */
#define PC004_CAMCYC		0x0C  /* not supported */
#define PC004_CAML		0x0D
#define PC004_GETQX		0x0E

#define PC004_CRATE_TST		0x10
/* -------------------------------------------------------------------------------------------- */
/* Misc-commands:										*/
/* -------------------------------------------------------------------------------------------- */
#define PC004_GET_TIMEOUT	0x10
#define PC004_SET_TIMEOUT	0x11
#define PC004_GET_TIMELIMIT	0x12
#define PC004_SET_TIMELIMIT	0x13
#define PC004_GET_ALL		0x14
#define PC004_SET_ALL		0x15
#define PC004_BUSY              0x16

/*This union is used for the ioctl */

struct PC004_DATA_T {
  char	N, A, F;
  int DATA;
};

/* This struct is used for misc data about the PC004 */
struct PC004_INFO_T {
	int	PC004_TIMEOUT;			/* FUTURE: timeout for this reqeust*/
	long	PC004_EXPIRETIME;		/* FUTURE: Time to re-read*/
	long	PC004_TIMELIMIT;		/* FUTUTE: Max time before data is invalid*/
	int	BUSY;				/* FUTURE: This minor PC004 is in use*/
	struct	PC004_DATA_T PC004_DATA;	/* last tranfered data*/
};

/* This struct ist for static common data (See orig. source cam6001.asm) */
struct PC004_STDATA_TYPE {
           unsigned int	busy,           /* global busy signal */
	                dma_con,	/* control register */
			dma_q,		/* ==1 for q-mode  block transfer */
			dma_xfr,	/* total number of bytes to transfer */
			dma_nob,	/* number of bytes / dadaway cycle (1 to 3) */
			dma_arr,	/* array starting address */
			dma_rem,	/* for 3-byte transfers */
			dma_scr,	/* scratch register */
			dma_scr2,	/* scratch register */
			dma_scr3;	/* number of q-mode dma transferes */
					/* less the first camac cycle */
};

