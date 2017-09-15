/* *********************************************************************
** SUBROUTINE	CAMERR
**
** DESCRIPTION
**	Subroutine to return whether an error occured in the last operation
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
**	16-Sept-1994	JMS	make it return vsd_status
**	16-Sept-1994	JMS	JY411S
**
**  ======================================================================== */
/* 									     */

#include "ieee_fun_types.h"

unsigned int camerr ( void )
{
	return (ieee_status);
}
/* *********************************************************************
** SUBROUTINE	CAMERR_
**
** DESCRIPTION
**	Subroutine to return whether an error occured in the last operation 
**	This stub makes camerr callable from a FORTARN application
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

unsigned int camerr_ (void)
{
	return ( camerr() );
}
/* ********************************************************************* */
