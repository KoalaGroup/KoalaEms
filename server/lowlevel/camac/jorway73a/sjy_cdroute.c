/* ***********************************************************************
**  SUBROUTINE CDROUTE
**
**      
** DESCRIPTION
**      Defines a remote CAMAC destination in rcam
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

unsigned int cdroute(char *rsys,         /* remote system type; vxworks or unix */
                     char *rlink,        /* remote link type; ces, sjy, vsd */
                     int   rdev,         /* rpc program number */
                     char *rnode,        /* remote node name */
                     int  rte_num)       /* CAMAC subsystem route number -
                                            idx to clnt_handle */
{
/* dummy */
  return(CAM_S_SUCCESS);
}
/* ========================================================================= */
/* ========================================================================= */
/*  SUBROUTINE cdroute_
**
**
**
*/

void cdroute_ (char *rsys,
                char *rlink,
                int  *rdev,
                char *rnode,
                int  *rte_num)
{
    cdroute(rsys, rlink, *rdev, rnode, *rte_num);
    return;
}
/* ------------------------------------------------------------------------- */
