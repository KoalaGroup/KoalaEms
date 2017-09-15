/* ************************************************************************* 
**  SUBROUTINE CSMAD
**
**      
** DESCRIPTION
**      Subroutine to execute a 16 bit mode CAMAC function in repeat mode,
**	incrementing 
**	the slot and subadress bits, depending on the previous Q response.
**
** DEVELOPERS
**      J.Streets D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**	19-Jul-1994	JAC	Creation from small modifications to CSSA
**	27-Sep-1994	JMS	JY411S
**      05-Aug-1997     das     modified for Linux
**
**  ======================================================================== 
*/ 									     

#include "ieee_fun_types.h"

unsigned int csmad (int 	    fun,
		    int 	    ext_add,
		    unsigned short *data_ptr,
		    int	           *block)
{
/* Local storage	*/
  int	branch;
  int	cmd;


/* ------------------------------------------------------------------------ */
	/* Complete encoding address with function - 
	   ext_add has crate, slot, and subaddress */

    branch = ext_add >> 24;
    cmd = (ext_add&0x00FFFFFF) | (fun<<24) | (SJY_ADD_SCAN<<16);


    /* returns block[1] */
    block[1]=0;
    errno = 0;

    if(fun < 0x8)
	/* read */
	block[1] = sjy_read(branch, cmd, data_ptr, sizeof(unsigned short)*block[0]);
    else if(fun & 0x8)
	/* non data */
        block[1] = sjy_nondata(branch, cmd);
    else
	/* write */
        block[1] = sjy_write(branch, cmd, data_ptr, sizeof(unsigned short)*block[0]);

    /* block [1] needs number of CAMAC transfers */
    block[1] = block[1]/sizeof(unsigned short);

    sjy_get_qx(branch);

    return(CAM_S_SUCCESS);

}


/* ==========================================================================*/
/* ************************************************************************* */
/*  SUBROUTINE CSMAD_						     	     */
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


void csmad_ (	int 	*fun,
		int 	*ext_add,
		unsigned short *data_ptr,
		int	*block)
{
	
	ieee_status = csmad (*fun,*ext_add,data_ptr,block);
}
/* ==================================================================== */
