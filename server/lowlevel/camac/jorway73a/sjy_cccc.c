/* ********************************************************************
**  SUBROUTINE CCCC						     
**
** DESCRIPTION
**    Subroutine to CLEAR the dataway 
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
**	24-Sep-1992	EGS	Modified for MVME147/167 VxWorks
**	21-Sep-1993	DAS	Modified for VSD2992
**      05-Aug-1997     das     modified for Linux
**
** --------------------------------------------------------------- */
#include "ieee_fun_types.h"

unsigned int cccc (int ext)	/* Used to get the crate and branch	*/
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
	  /* serial */
	  slot=30;
	  subad=0;
	  f=19;
	  data=2;
	  cmd = SJY_CAMAC_EXT(f|SJY_D24_BIT,slot,subad,crate|SJY_SERIAL_BIT);
          sjy_write(branch, cmd, &data, sizeof(data));
	  }
	else{		 /* parallel */
	  slot=28;
	  subad=9;
	  f=26;
	  cmd = SJY_CAMAC_EXT(f,slot,subad,crate);
	  sjy_nondata(branch, cmd);
	  }

	qres=sjy_get_qx(branch);
	return(CAM_S_SUCCESS);

}
/* ************************************************************************* */
/* ************************************************************************* */
/*  SUBROUTINE CCCC_						     */
/*
    Subroutine to CLEAR the dataway 
**  This stub makes the cccc call possible from a FORTRAN application
									     */


void cccc_ (int *ext)		/* Used to get the crate and branch	*/
{
/*   Calling CCCC  */

	ieee_status = cccc (*ext);
}
/* --------------------------------------------------------------------- */
