
/* sjy_ccctype.c */
/*	Sets the Branch type as Serial or Parallel */
/*	27-Sep-1994	JMS JY411S */


#include "ieee_fun_types.h"

unsigned int ccctype (int	branch,
	              int	lserial,
	              int	lparall)
{
    if(branch<0 || branch > SJY_BRANCH_MAX)
	return(CAM_S_INVLD_BRANCH);

    if(lserial & lparall)
	return(CAM_S_SER_OR_PAR_ONLY);

    if(lserial)
	sjy_branch[branch].serial=1;

    if(lparall)
	sjy_branch[branch].serial=0;

    return(CAM_S_SUCCESS);
}

/* FORTRAN version */

void ccctype_  (int *branch,
		int	 *lserial,
		int	 *lparall)
{
    ieee_status=ccctype(*branch,*lserial,*lparall);
}
