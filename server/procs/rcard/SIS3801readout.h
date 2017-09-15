/************************************************************************
 *
 * $Date: 2006/11/14 20:04:18 $
 * $Revision: 1.2 $
 * $Log: SIS3801readout.h,v $
 * Revision 1.2  2006/11/14 20:04:18  trusov
 * Add support for CAEN_V1290 TDC readout, and SIS_3800 scaler readout
 * functions in server/procs/rcard.
 *
 * Revision 1.1  2003/06/26 13:01:03  mussgill
 * *** empty log message ***
 *
 * Revision 1.1  2002/06/26 10:58:04  mussgill
 * *** empty log message ***
 *
 *
 ************************************************************************/                           

#include "../../objects/domain/dom_ml.h"

int SIS3801readout __P((ml_entry *,int));
int SIS3800readout __P((ml_entry *,int));
