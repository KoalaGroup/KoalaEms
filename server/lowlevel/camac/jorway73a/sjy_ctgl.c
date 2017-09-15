/* ********************************************************************
**  SUBROUTINE CTGL
**
** DESCRIPTION
**    Subroutine to Test for GL
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
**	26-Sep-1994	jms	creation
**      05-Oct-1997     das     modified for Linux
**
** --------------------------------------------------------------- */
#include "ieee_fun_types.h"

unsigned int ctgl (int ext, int *lvalue)
{
	int	branch;
	int	crate;
	int	slot;
	int	subad;
	int	f;
	int	cmd;

/* ------------------------------------------------------------------------ */

       /* get first branch */

  cgreg (ext,&branch,&crate,&slot,&subad);
  *lvalue=0;

  if(sjy_branch[branch].serial){
	  /* serial */
	  crate = SJY_SERIAL_BIT;
	  slot=30;
	  }
  else{		 /* parallel */
	  crate = 0;
	  slot=30;
	  }
  f=0;
  subad=0;
  cmd = SJY_CAMAC_EXT(f,slot,subad,crate|SJY_SERIAL_BIT);
  sjy_read(branch, cmd, lvalue, sizeof(*lvalue));
  sjy_get_qx(branch);
  return(CAM_S_SUCCESS);

}
/* ************************************************************************* */
/* ************************************************************************* */
/*  SUBROUTINE CTGL_						     */
/*
    Subroutine to Test for GL
**  This stub makes the cccc call possible from a FORTRAN application
									     */


void ctgl_ (int *ext, int *lvalue)
{
/*   Calling CCCC  */

	ieee_status = ctgl (*ext, lvalue);
}
/* --------------------------------------------------------------------- */
