/* ***********************************************************************
**  SUBROUTINE CFSA						     
**
**      
** DESCRIPTION
**      Subroutine to execute a 24 bit mode CAMAC function
**
** DEVELOPERS
**      D.Slimmer, J.Streets
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991     MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME147/167 and VxWorks
**	22-Oct-1993	DAS	Modified for VSD2992
**	17-Nov-1993	DAS	Add check for command completion
**	12-Jul-1994	JAC	removed all read and write delays, removed poll
**	20-sep-1994	jms	JY411S
**      05-aug-1997     das     modified for Linux
**      09-sep-1998     das     fixed SJY_D24_BIT bug for JOR 73A
**
**  ========================================================================
*/ 									    

#include "ieee_fun_types.h"

unsigned int cfsa  (int fun,
		    int ext_add,
		    int *data24,
		    int *qres)
{
/* Local storage	*/
  int	branch;
  int	cmd;


/* ------------------------------------------------------------------------ */

	/* Complete encoding address with function 
	   ext_add has crate, slot, and subaddress */
  *qres=0;
  branch = ext_add >> 24;

  errno=0;

  if( fun < 0x8 ) {
     cmd = (ext_add & 0x00FFFFFF) | (fun<<24) | SJY_D24_MODE | (SJY_STOP_ON_WORD<<16);
     sjy_read(branch, cmd, data24, sizeof(*data24));
     }
  else if( fun & 0x8 ) {
     cmd = (ext_add & 0x00FFFFFF) | (fun<<24) | SJY_D24_MODE;
     sjy_nondata(branch, cmd);
     }
  else{
     cmd = (ext_add & 0x00FFFFFF) | (fun<<24) | SJY_D24_MODE | (SJY_STOP_ON_WORD<<16);
     sjy_write(branch, cmd, data24, sizeof(*data24));
     }

  *qres = sjy_get_qx(branch);
  
  return (CAM_S_SUCCESS);
}

/* ==========================================================================*/
/* ************************************************************************* */
/*  SUBROUTINE CFSA_						             */
/*
**      
** DESCRIPTION
**      Subroutine stub to execute a 24 bit mode CAMAC function
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991     MVW     Creation
**
**  ======================================================================== */
/* 									     */


void cfsa_ (int *fun,
	     int *ext_add,
	     int *data24,
	     int      *qres)
{
	ieee_status = cfsa(*fun,*ext_add,data24,qres);
}

/* ==========================================================================*/









