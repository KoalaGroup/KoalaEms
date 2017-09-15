/* ************************************************************************* 
**  SUBROUTINE CFMAD
**
**      
** DESCRIPTION
**      Subroutine to execute a 24 bit mode CAMAC function in repeat mode,
**	incrementing 
**	the slot and subadress bits, depending on the previous Q response.
**
** DEVELOPERS
**      J.Streets D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**
** MODIFICATIONS
**         Date       Initials  Description
**	19-Jul-1994	JAC	Creation from small modifications to CSSA
**	27-Sep-1994	JMS	JY411S
**      05-Aug-1997     das     modified for Linux
**      09-sep-1998     das     fixed SJY_D24_BIT bug for JOR 73A
**
**  ======================================================================== 
*/ 									     

#include "ieee_fun_types.h"

unsigned int cfmad (int  fun,
		    int  ext_add,
		    int	*data_ptr,
		    int	*block)
{
/* Local storage	*/
  int	branch;
  int 	cmd;
  int	qres;


/* ------------------------------------------------------------------------ */
	/* Complete encoding address with function - 
	   ext_add has crate, slot, and subaddress */

    branch = ext_add >> 24;
    cmd = (ext_add&0x00FFFFFF) | (fun<<24) | SJY_D24_MODE | (SJY_ADD_SCAN<<16);

    block[1]=0;
    errno=0;

    if(fun < 0x8)
	/* read */
	block[1] = sjy_read(branch, cmd, data_ptr, sizeof(int)*block[0]);
    else if(fun & 0x8)
	/* non data */
        block[1] = sjy_nondata(branch, cmd);
    else
	/* write */
        block[1] = sjy_write(branch, cmd, data_ptr, sizeof(int)*block[0]);

    /* 4 bytes per 24 bit transfer */
    block[1]=block[1]/sizeof(int);

    qres=sjy_get_qx(branch);

    return(CAM_S_SUCCESS);


}


/* ==========================================================================*/
/* ************************************************************************* */
/*  SUBROUTINE CFMAD_						     	     */
/*
**      
** DESCRIPTION
**      Subroutine stub to execute a 16 bit mode repeat CAMAC function
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      19-Jul-1994     JAC     Creation
**
**  ======================================================================== */
/* 									     */


void cfmad_ (int  *fun, int *ext_add, int *data_ptr, int *block)
{
	
	ieee_status = cfmad (*fun,*ext_add,data_ptr,block);
}
/* ==================================================================== */
