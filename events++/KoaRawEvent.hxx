#ifndef _koa_raw_event_
#define _koa_raw_event_

#include "TObject.h"

// class KoaEmsEvent;
class KoaRawEvent : public TObject
{
public:
  KoaRawEvent();
  ~KoaRawEvent();
  
public:
  // Ems Event
  // KoaEmsEvent *fEmsEvent;
  
  // Mxdc32 Timestamp
  Long64_t  fTimestamp;

  // Amplitudes
  Int_t     fSi1Amplitude[48];
  Int_t     fSi2Amplitude[64];
  Int_t     fGe1Amplitude[32];
  Int_t     fGe2Amplitude[32];
  Int_t     fRecRearAmplitude[4];
  Int_t     fFwdAmplitude[8];

  // Timestamps
  Float_t   fSi1Time[48];// unit: ns
  Float_t   fSi2Time[64];// unit: ns
  Float_t   fGe1Time[32];// unit: ns
  Float_t   fGe2Time[32];// unit: ns
  Float_t   fRecRearTime[2];// unit: ns
  Float_t   fFwdTime[8];// unit: ns

  Float_t   fTriggerTime[3];

  ClassDef(KoaRawEvent,1); //initial version for bt2019
};

#endif
