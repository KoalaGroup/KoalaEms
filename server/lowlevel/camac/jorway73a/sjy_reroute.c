/* ***********************************************************************
**  SUBROUTINE REROUTE
**
**      
** DESCRIPTION
**      Changes a remote CAMAC destination in rcam
**		include here to make executables linkable
**
** DEVELOPERS
**      D.Slimmer, J.Streets
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**
** MODIFICATIONS
**         Date       Initials  Description
**	16aug95		jms	creation
**        
**  ========================================================================
*/ 									    

#include "ieee_fun_types.h"

unsigned int reroute(int route)
{
/* dummy */
return(CAM_S_SUCCESS);
}

/* ========================================================================= */
/* ========================================================================= */
/*  SUBROUTINE reroute_
**
**
**
*/
void reroute_ (int *route)
{
    reroute(*route);
    return;
}
/* ------------------------------------------------------------------------- */
