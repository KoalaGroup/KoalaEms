#ifndef _koa_ems_event_hxx_
#define _koa_ems_event_hxx_

#include "TObject.h"

class KoaEmsEvent : public TObject
{
public:
  KoaEmsEvent();
  ~KoaEmsEvent();

public:
  // Time tags
  Long64_t  fSecond;
  Long64_t  fUSecond;

  // Scalers
  UInt_t  fScalerRec[4];// 0-->Si#1, 2-->Si#2, 3->Ge#1, 4-->Ge#2
  UInt_t  fScalerFwd[4];// 0-->1&2, 1-->3&4, 2-->5&6, 3-->7&8
  UInt_t  fScalerCommonOr;
  UInt_t  fScalerGeOverlap[2];
  UInt_t  fScalerSiFwdOutsideCon[2]; // coincidence between Si and FwdOutside
  UInt_t  fScalerSiRear[2];
  UInt_t  fScalerFwdCh[8];

  //
  UInt_t  fEventNr;

  ClassDef(KoaEmsEvent,2);// initial version for bt2019 march
};

#endif
