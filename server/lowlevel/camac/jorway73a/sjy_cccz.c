/* ***********************************************************************
**  SUBROUTINE CCCZ
**
**
** DESCRIPTION
**	Subroutine to generate Z in CAMAC controller.
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

unsigned int  cccz(int ext)
{
	int	branch;
	int	crate;
	int	slot;
	int	subad;
	int	f;
	int	cmd;
	int	data;
	int	qres;

/* ------------------------------------------------------------------------ */

  /* get first branch and crate */
  cgreg (ext,&branch,&crate,&slot,&subad);

  if(sjy_branch[branch].serial){
    slot=30;
    subad=0;
    data=0x0001;
    f=19;			/* set Z */
    crate |= SJY_SERIAL_BIT;
    cmd = SJY_CAMAC_EXT(f|SJY_D24_BIT,slot,subad,crate);
    sjy_write(branch, cmd, &data, sizeof(data));
    }
  else{				/* parallel */
    slot=28;
    subad=8;
    f=26;
    cmd = SJY_CAMAC_EXT(f,slot,subad,crate);
    sjy_nondata(branch, cmd);
    } 

  qres=sjy_get_qx(branch);
  return(CAM_S_SUCCESS);

}

/* ============================================================== */
/* ************************************************************************* */
/*  SUBROUTINE CCCZ_						     */
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


void cccz_ (int *ext)
{

	ieee_status = cccz (*ext);
}
/* ------------------------------------------------------------------- */
