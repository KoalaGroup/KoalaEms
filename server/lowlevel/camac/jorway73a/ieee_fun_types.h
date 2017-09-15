/* ************************************************************** */
/*                                                                */
/*   ieee_fun_types - IEEE CAMAC routine prototypes     	  */
/* -------------------------------------------------------------- */

/*
 *	03nov93 das     added argument prototypes
 *	01sep94 das	added cdchn
 *	07oct94 das	chged cdrpc and cdchn args
 *	11oct94 das	added crmode tp replace cdrpc
 *	25oct94 das	chg crmode to cdroute because of symbol conflict
 *			with curses
 *	16aug95	jms	Version for CAMAC product.
 *	24aug95 das	camac_mur_msg.h to camac_murmur_msg_c.h
 *	30aug95	das	fixed cdlam, ctlm, and ctstat prototypes
 *
 */

#ifndef INCieee_fun_typesh
#define INCieee_fun_typesh

#ifndef XDEFLG
#define xdef extern
#else
#define xdef
#endif

/* Murmur Messages */
#include "camac_murmur_msg_c.h"

/* Driver specific globals */
#include "camac_globals.h"

#ifndef VXWORKS
#include <sys/types.h>
#endif


/* status of last FORTRAN CALL */
xdef    int     ieee_status;

/* X and Q from Last IEEE command */
xdef    int     ieee_last_Q;
xdef    int     ieee_last_X;



/******************************************************************************
 * IEEE Standard Routines
 */

/* Subroutine to encode ext from B C N A */
extern 	unsigned int	cdreg  (int *ext_add,
				int branch,
				int crate,
				int slot,
				int subad);
extern 	void 	 	cdreg_(int *ext_add,
			       int *branch,
			       int *crate,
			       int *slot,
			       int *subad);


/* Subroutine to decode ext into B C N A */
extern  unsigned int	cgreg (int add_mask,
                               int *branch,
                               int *crate,
                               int *slot,
                               int *subad);
extern 	void 	 	cgreg_(int *add_mask,
                               int *branch,
                               int *crate,
                               int *slot,
                               int *subad);


/* Subroutine to execute a branch C */
extern  unsigned int	cccc (int ext);
extern  void 	 	cccc_(int *ext);

/* Subroutine to set or clear branch Inhibit (I) */
extern unsigned int	ccci (int ext,
		      	      int iflag);
extern void    		ccci_(int *ext,
		      	      int *iflag);

/* Subroutine to execute a branch Z */
extern unsigned int	cccz (int ext);
extern void    		cccz_(int *ext);


/* Subroutine to execute a single 24 bit data transfer function */
extern unsigned int	cfsa (int fun,
		      	      int ext_add,
		              int *data24,
		              int *qres);
extern void    		cfsa_(int *fun,
		              int *ext_add,
		              int *data24,
		              int *qres);

/* Subroutine to execute a scaning 24 bit "DMA" CAMAC operation */
extern unsigned int	cfmad (int fun,
		               int ext_add,
		  	       int *data_ptr,
		  	       int *block);
extern void 		cfmad_(int *fun,
		               int *ext_add,
		               int *data_ptr,
		               int *block);

/* Subroutine to execute a single slot 24 bit "DMA" CAMAC operation */
extern unsigned int	cfubc (int fun,
		               int ext_add,
		               int *data_ptr,
		               int *block);
extern void 		cfubc_(int *fun,
		               int *ext_add,
		               int *data_ptr,
		               int *block);


/******************************************************************************
 * IEEE System Dependent Routines
 */

/* Subroutine to return Q and X responses, and errors */
extern unsigned int	ctstat (int *k);
extern void    		ctstat_(int *k);

/* Subroutine to declare channel (system dependent configuration) */
extern unsigned int	cdchn (int branch, int lflag, int route);
extern void    		cdchn_(int *branch, int *lflag, int *route);

/* Subroutine to execute a single 16 bit data transfer function */
extern unsigned int	cssa (int  fun,
		              int  ext_add,
		              unsigned short *data16,
		              int      *qres);
extern void    		cssa_(int *fun,
		              int *ext_add,
		              unsigned short *data16,
		              int      *qres);

/* Subroutine to execute a scaning 16 bit "DMA" CAMAC operation */
extern unsigned int	csmad (int fun,
		               int ext_add,
		               unsigned short *data_ptr,
		  	       int *block);
extern void 		csmad_(int *fun,
		               int *ext_add,
		               unsigned short *data_ptr,
		               int *block);

/* Subroutine to execute a single slot 16 bit "DMA" CAMAC operation */
extern unsigned int	csubc (int fun,
		               int ext_add,
		               unsigned short *data_ptr,
		               int *block);
extern void 		csubc_(int *fun,
		               int *ext_add,
		               unsigned short *data_ptr,
		               int *block);


/******************************************************************************
 * FNAL extensions to CAMAC Standard Routines
 */

/* Subroutine to initialize RPC server 	*/
extern unsigned int	cdroute (char *rsys,
				char *rlink,
				int   rdev,
				char *rnode,
				int   rte_num);
extern void    		cdroute_(char *rsys,
				char *rlink,
				int  *rdev,
				char *rnode,
				int  *rte_num);

/* Subroutine to change the destination of the next CAMAC operation */
extern unsigned int	reroute (int route);
extern void 		reroute_(int *route);


/* Subroutine to return error from last CAMAC operaton */
extern unsigned int	camerr (void);
extern unsigned int	camerr_(void);

/* Set or clear SCC bypass */
extern unsigned int	cccbyp (int ext_add,
		                int flag);
extern void 		cccbyp_(int *ext_add,
		                int *flag);

/* Set or clear SCC offline */
extern unsigned int	cccoff (int ext_add,
		                int flag);
extern void 		cccoff_(int *ext_add,
		                int *flag);

/* Return register information */
extern unsigned int	cmsg (char *sname, int data, int branch);
extern void 		cmsg_(char *sname, int *data, int *branch);

/* Return Q response from last CAMAC operation */
extern short 		qrespn (void);
extern short 		qrespn_(void);

/* Return X response from last CAMAC operation */
extern short 		xrespn (void);
extern short 		xrespn_(void);

/* Declares CAMAC branch to be serial, parallel or mixed */
extern unsigned int 	ccctype (int branch, int lserial, int lparall);
extern void 		ccctype_(int *branch, int *lserial, int *lparall);

/* Declares action and information printed on error */
extern unsigned int 	cderr (int lexit, int llog, int ltrace);
extern void 		cderr_(int *lexit, int *llog, int *ltrace);

/* Declares data read via cssa() to be signed or unsigned */
extern unsigned int	csuns (int lsign);
extern void 		csuns_(int *lsign);

/* Subroutine to handle LAM */

extern unsigned int	cdlam (int *lam, int branch, int crate, int slot,
			       int m, int *inta);
extern void    		cdlam_(int *lam, int *branch, int *crate, int *slot,
			       int *m, int *inta);

extern unsigned int	ctlm (int ext_add, int *iflag);
extern void    		ctlm_(int *ext_add, int *iflag);

/* -------------------------------------------------------------- */
#endif /* INCieee_fun_typesh */ 
