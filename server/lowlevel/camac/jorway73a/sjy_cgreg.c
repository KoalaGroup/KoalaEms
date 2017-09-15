/* *************************************************************************
**  SUBROUTINE CGREG						     
** DESCRIPTION
**
**    Subroutine to decode ext into C N A
**
** DEVELOPERS
**      J.Streets, D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME147/167
**	21-Oct-1993	DAS	Modified for VSD2992
**	26-Sep-1994	jms	JY411S
**      05-Aug-1997     das     modified for Linux
**
** ---------------------------------------------------------------
*/

#include "ieee_fun_types.h"

unsigned int cgreg (int add_mask,	/* Input     */
		    int *branch,
		    int *crate,
		    int *slot,
		    int *subad)
  {

/*-------------------------------------------------------------------------*/

        *branch = (add_mask & 0xFF000000) >> 24;
        *slot   = (add_mask & 0x00FF0000) >> 16;
        *subad  = (add_mask & 0x0000FF00) >>  8;


#ifdef SJY411S
	*crate  = (add_mask & 0x000000FF);

	if(*crate & SJY_SERIAL_BIT){
	  *crate = *crate & ~SJY_SERIAL_BIT;
	  }
	else{
	  switch(*crate){
	    case 1: case 2:
	     *crate = *crate;
	     break;
	    case 4:
	     *crate=3;
	     break;
	    case 8:
	     *crate=4;
	     break;
	    case 16:
	     *crate=5;
	     break;
	    case 32:
	     *crate=6;
	     break;
	    case 64:
	     *crate=7;
	     break;
	    default:
	     *crate=0;
	     return(CAM_S_INVLD_CRATE);
	     }
	}
#endif

	return (CAM_S_SUCCESS);
}
/* ************************************************************************* */
/*  SUBROUTINE CGREG_						     */
/*
    Subroutine to decode ext into B C N A
    Stub to make cgreg callable from a FORTRAN application
									     */

void cgreg_ (int *add_mask_f,	/* Input     */
		 int *branch_f,
		 int *crate_f,
		 int *slot_f,
		 int *subad_f)
{

	ieee_status = cgreg(*add_mask_f,branch_f,crate_f,slot_f,subad_f);
}
/* ************************************************************************* */
