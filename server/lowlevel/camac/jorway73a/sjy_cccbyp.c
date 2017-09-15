/* ***********************************************************************
**  SUBROUTINE CCCBYP
**
**
** DESCRIPTION
**	Subroutine to set/clear BYPASS serial CAMAC controller.
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**      Eric G. Stern
**      Nevis Labs
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME147/167 and VxWorks
**      21-Oct-1993	MVW	Adapting to VSD2992 serial camac module
**	22-Oct-1993	DAS	ditto
**	20-Sep-1994	JMS	JY411S
**      05-Aug-1997     das     modified for Linux
**
**  ======================================================================== 
*/ 									    

#include "ieee_fun_types.h"

unsigned int  cccbyp (int ext,		/* compacted address		*/
		      int iflag)	/* set I = 1, clear I = 0	*/
{
	int	branch;
	int	crate;
	int	slot;
	int	subad;
	int	f;
	int	cmd;
	int	data[1];
	int	qres;

/* ------------------------------------------------------------------------ */

  /* get first branch and crate */
  cgreg (ext,&branch,&crate,&slot,&subad);

  if(sjy_branch[branch].serial){
#ifdef DEBUG
    printf("cccbyp: set to serial\n");
#endif
    slot=30;
    subad=0;
    data[0]=0x000800;
    if ( iflag == 0)		/* clear BYP */
      f=23;
    else
      f=19;			/* set BYP */

    cmd = SJY_CAMAC_EXT(f|SJY_D24_BIT,slot,subad,crate|SJY_SERIAL_BIT);
    sjy_write(branch, cmd, &data, sizeof(data));
    qres=sjy_get_qx(branch);
    return(CAM_S_SUCCESS);
    }

  else{				/* parallel */
    return(CAM_S_PARALLEL);
    }

}

/* ============================================================== */
/* ************************************************************************* */
/*  SUBROUTINE CCCBYP_						     */
/*
**
** DESCRIPTION
**	Subroutine to clear branch inhibit if iflag = 0
**	and to set the inhibit if iflag = 1
**	This stub makes the ccci call possible from a FORTRAN application
**
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**
**  ======================================================================== */
/* 									     */


void cccbyp_ (int *ext,		/* This is actually not used */
	    int *iflag)
{

	ieee_status = cccbyp (*ext, *iflag);
}
/* ------------------------------------------------------------------- */
