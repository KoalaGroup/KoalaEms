/* ********************************************************************
**  SUBROUTINE CCLM
**
** DESCRIPTION
**    Subroutine to Enable/Disable LAM
**
** DEVELOPERS
**      J.Streets D.Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME147/167 VxWorks
**	21-Sep-1993	DAS	Modified for VSD2992
**	26-Sep-1994	jms	creation
**      05-Aug-1997     das     modified for Linux
**
** --------------------------------------------------------------- */
#include "ieee_fun_types.h"

unsigned int cclm (int ext, int lvalue)
{
	int	branch;
	int	crate;
	int	slot;
	int	subad;
	int	f;
	int	cmd;
	int	qres;
        int     data;

/* ------------------------------------------------------------------------ */

  /* decode branch */

  cgreg (ext,&branch,&crate,&slot,&subad);

  slot=30;
  if(sjy_branch[branch].serial){        /* serial */
        subad=0;
        data=0x0100;
        if (lvalue == 0)  /* disable */
          f=23;
        else
          f=19;           /* enable */
        cmd = SJY_CAMAC_EXT(f|SJY_D24_BIT,slot,subad,crate|SJY_SERIAL_BIT);
        sjy_write(branch, cmd, &data, sizeof(data));
	}
  else{		 			/* parallel */
        subad=10;
        if (lvalue == 0)  /* disable */
          f=24;
        else
          f=26;           /* enable */
        cmd = SJY_CAMAC_EXT(f,slot,subad,crate);
        sjy_nondata(branch, cmd);
  }


  qres=sjy_get_qx(branch);
  return(CAM_S_SUCCESS);

}
/* ************************************************************************* */
/* ************************************************************************* */
/*  SUBROUTINE CCLM_						     */
/*
    Subroutine to Enable or disable LAMS
									     */


void cclm_ (int *ext, int *lvalue)
{

	ieee_status = cclm (*ext, *lvalue);
}
/* --------------------------------------------------------------------- */
