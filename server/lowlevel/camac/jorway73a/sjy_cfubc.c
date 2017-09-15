/* ************************************************************************* 
**  SUBROUTINE CFUBC
**
**      
** DESCRIPTION
**      Subroutine to execute a 24 bit mode CAMAC function in repeat mode.
**	until run out of words or No Q.
**
** DEVELOPERS
**      J.Streets, D.Slimmer
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
**      19-feb-1998     das     change write to sjy_write
**
**  ======================================================================== 
*/ 									     

#include "ieee_fun_types.h"

unsigned int cfubc (int fun,
		    int ext_add,
		    int	*data_ptr,
		    int	*block)
{
/* Local storage	*/
  int	branch;
  int 	cmd;


/* ------------------------------------------------------------------------ */
    /* Complete encoding address with function - 
       ext_add has crate, slot, and subaddress */
    branch = ext_add >> 24;
    cmd = (ext_add&0x00FFFFFF) | (fun<<24) | SJY_D24_MODE;

    /* Check for STOP-ON-WORD */
    SJY_CLEAR_MODE(cmd);
    if(block[3]==1)
	cmd |= (SJY_STOP_ON_WORD<<16);
    else
        cmd |= (SJY_Q_STOP<<16);


    /* returns # of CAMAC ops - 4 bytes per 24bit transfer */
    block[1]=0;
    errno = 0;

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

    sjy_get_qx(branch);

    return(CAM_S_SUCCESS);


}


/* ==========================================================================*/
/* ************************************************************************* */
/*  SUBROUTINE CFUBC_						     	     */
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


void cfubc_ (	int 	*fun,
		int 	*ext_add,
		int	*data_ptr,
		int	*block)
{
	
	ieee_status = cfubc (*fun,*ext_add,data_ptr,block);
}
/* ==================================================================== */
