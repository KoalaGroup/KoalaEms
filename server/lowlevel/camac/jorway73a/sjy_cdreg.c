/* ******************************************************************
**  SUBROUTINE CDREG						     
**
** DESCRIPTION
**    Subroutine to encode ext from B C N A
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
**	24-Sep-1992	EGS	Modified for MVME147/167 and VxWorks
**	21-Oct-1993	DAS	Modified for VSD2992
**	05-Aug-1994	JAC	Added error messages
**	20-sep-1994	JMS	JY411S
**
** --------------------------------------------------------------- */

#include "ieee_fun_types.h"


unsigned int cdreg (int *ext_add,	/* Encoded address	*/
		    int branch,
		    int crate,		/* Crate number         */
		    int slot,		/* Slot  number         */
		    int subad)		/* Subaddress           */
{
   /* Local storage */

/* ------------------------------------------------------------ */

  *ext_add=0;

#ifdef SJY411S
   if( sjy_branch[branch].serial ){
	if(crate < 1 || crate > 62)
	  return(CAM_S_INVLD_CRATE);
	crate = crate | SJY_SERIAL_BIT;
	}
   else{
	if(crate<1 || crate>7)
	  return(CAM_S_INVLD_CRATE);
        }
#endif
#ifdef SJY73A
   crate=0;
#endif

   if( branch<0 || branch > SJY_BRANCH_MAX)
	return(CAM_S_INVLD_BRANCH);

   if( slot<1 || slot>30)
	return(CAM_S_INVLD_SLOT);

   if( subad<0 || subad >15)
	return(CAM_S_INVLD_SUBADD);


   *ext_add=SJY_CAMAC_EXT(branch,slot,subad,crate);

   return (CAM_S_SUCCESS);
}
/* ************************************************************ */
/* ************************************************************ */
/*  SUBROUTINE CDREG_						*/
/*
    Subroutine to make the call possible from a FORTRAN program
								*/

void cdreg_ (int *ext_add_f,	/* Encoded address	*/
		 int *branch_f,
		 int *crate_f,	/* Crate number         */
		 int *slot_f,	/* Slot  number         */
		 int *subad_f)	/* Subaddress           */
{
/*    Call CDREG  */

	ieee_status = cdreg (ext_add_f,*branch_f,*crate_f,*slot_f,*subad_f);
}
/* ************************************************************* */
