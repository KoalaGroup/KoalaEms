/* ***********************************************************************
**  SUBROUTINE CCCD
**
**
** DESCRIPTION
**	Subroutine to set/clear Branch Demands on CAMAC controller.
**
** DEVELOPERS
**      J.Streets D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
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

int  cccd(int ext,		/* compacted address		*/
	  int iflag)		/* set I = 1, clear I = 0	*/
{
	int	branch;
	int	crate;
	int	slot;
	int	subad;
	int	f;
	int	cmd;
	int	data;

/* ------------------------------------------------------------------------ */

  /* decode branch and crate */
  cgreg (ext,&branch,&crate,&slot,&subad);

  slot=30;
  if(sjy_branch[branch].serial){
    subad=0;
    data=0x0100;
    if ( iflag == 0)		/* disable BD */
      f=23;
    else
      f=19;			/* enable BD */
    crate |= SJY_SERIAL_BIT;
    cmd = SJY_CAMAC_EXT(f|SJY_D24_BIT,slot,subad,crate);
    sjy_write(branch, cmd, &data, sizeof(data));
    }
  else{				/* parallel */
    subad=10;
    if ( iflag == 0)		/* disable BD */
      f=24;
    else			/* enable BD */
      f=26;
    cmd = SJY_CAMAC_EXT(f,slot,subad,crate);
    sjy_nondata(branch, cmd);
    } 

  sjy_get_qx(branch);
  return(CAM_S_SUCCESS);

}

/* ============================================================== */
/* ************************************************************************* */
/*  SUBROUTINE CCCD_						     */
/*
**
** DESCRIPTION
**	Subroutine to enable or disable branch demands
**	This stub makes the cccd call possible from a FORTRAN application
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


void cccd_ (int *ext,		/* This is actually not used */
	    int *iflag)
{

	ieee_status = cccd (*ext, *iflag);
}
/* ------------------------------------------------------------------- */
