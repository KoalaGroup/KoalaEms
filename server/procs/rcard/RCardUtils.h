/************************************************************************
 *
 * $Date: 2003/06/26 13:01:03 $
 * $Revision: 1.2 $
 * $Log: RCardUtils.h,v $
 * Revision 1.2  2003/06/26 13:01:03  mussgill
 * *** empty log message ***
 *
 * Revision 1.1  2002/06/26 10:58:04  mussgill
 * *** empty log message ***
 *
 *
 ************************************************************************/                           

#include "../../objects/domain/dom_ml.h"

int CAENV550readout __P((ml_entry *,int));
int CAENV550readoutCM __P((ml_entry *,int,int));
int CAENV775readout __P((ml_entry *,int));
int SIS3801readout __P((ml_entry *,int));
