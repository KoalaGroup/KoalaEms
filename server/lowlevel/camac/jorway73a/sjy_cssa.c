/* ************************************************************************* 
**  SUBROUTINE CSSA						     
**
**      
** DESCRIPTION
**      Subroutine to execute a 16 bit mode CAMAC function
**
** DEVELOPERS
**      J.Streets, D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991     MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME146/167 and VxWorks
**	27-Oct-1993	DAS	Modified for VSD2992
**	17-Nov-1993	DAS	Adding command completion check
**	11-Jul-1994	JAC	Removed read and write delays, polling
**	20-Sep-1994	jms	JY411S
**
**  ======================================================================== 
*/ 									     

#include "ieee_fun_types.h"

unsigned int cssa  (int 	fun,
	            int 	ext_add,
	            unsigned short *data16,
	            int	        *qres)
{
/* Local storage	*/
  int branch;
  int cmd;
  unsigned short locdat[2]={0,0};	/* SCSI has a problem with unaligned
					   int 16 words */

/* ------------------------------------------------------------------------ */

	/* Complete encoding address with function - 
	   ext_add has crate, slot, and subaddress */

  *qres = 0;

  branch = ext_add>>24;

  errno = 0;

  if( fun < 0x8 ) {
  	cmd = (ext_add & 0x00FFFFFF) | (fun <<24) | (SJY_STOP_ON_WORD<<16);
	sjy_read(branch, cmd, &locdat[0], sizeof(unsigned short));
	*data16 = locdat[0];
        }
  else if( fun & 0x8 ) {
  	cmd = (ext_add & 0x00FFFFFF) | (fun <<24);
        sjy_nondata(branch, cmd);
        }
  else{
  	cmd = (ext_add & 0x00FFFFFF) | (fun <<24) | (SJY_STOP_ON_WORD<<16);
	locdat[0] = *data16;
	sjy_write(branch, cmd, &locdat[0], sizeof(unsigned short));
        }

  *qres=sjy_get_qx(branch);

  return(CAM_S_SUCCESS); 

}

/* ==========================================================================*/
/* ************************************************************************* */
/*  SUBROUTINE CSSA_						     	     */
/*
**      
** DESCRIPTION
**      Subroutine stub to execute a 16 bit mode CAMAC function
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

void cssa_ (	int 	*fun,
		int 	*ext_add,
		unsigned short *data16,
		int	       	*qres)
{
	
	ieee_status = cssa (*fun,*ext_add,data16,qres);
}
/* ==================================================================== */







