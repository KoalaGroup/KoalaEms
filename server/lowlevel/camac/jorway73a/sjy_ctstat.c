/* *********************************************************************
** SUBROUTINE	CTSTAT
**
** DESCRIPTION
**	Subroutine to return the Q and X responses, and errors.
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
**	20-July-1994	JAC	Creation
**
**  ======================================================================== */
/* 									     */

#include "ieee_fun_types.h"


unsigned int ctstat(int	*k)
{

/* Local storage */


/* --------------------------------------------------------------------- */

  if( ieee_last_Q == 1 && ieee_last_X == 1 )
	/* Q,X returned */
	*k=0;
  else if(ieee_last_X == 1 && ieee_last_Q == 0)
	/* X, no Q */
	*k=1;
  else if(ieee_last_X == 0 && ieee_last_Q == 1)
	/* no Q, X */
	*k=2;
  else
	/* no X, no Q */
	*k=3;

	return(CAM_S_SUCCESS);
}

/* *********************************************************************
** SUBROUTINE	CTSTAT_
**
** DESCRIPTION
**	Subroutine to return the Q and X responses, and errors
**	This stub makes ctstat callable from a FORTARN application
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      20-Jul-1994	JAC     Creation
**
**  ======================================================================== */
/* 									     */


void ctstat_ (int	*k)
{

	ieee_status = ctstat(k);
}
/* ********************************************************************* */
