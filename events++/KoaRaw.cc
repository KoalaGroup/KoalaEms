#include "KoaRaw.hxx"
#include "global.hxx"

#define UNDER_THRESHOLD -5
#define ADC_OVERFLOW 0x2000
#define QDC_OVERFLOW 0x1000


namespace DecodeUtil
{
  KoaRaw::~KoaRaw()
  {
    if(fModuleId)   delete [] fModuleId;
    if(fResolution) delete [] fResolution;
    if(fNrWords)    delete [] fNrWords;
    if(fTimestamp)  delete [] fTimestamp;
    if(fData)       delete [] fData;

    if(fScaler)     delete [] fScaler;
  }

  void KoaRaw::Init()
  {
    if(fFile==nullptr){
      printf("Error: setup TFile first (KoaRaw)\n");
      return;
    }

    fFile->cd();

    fModuleId = new UChar_t[nr_mesymodules];
    fResolution = new Char_t[nr_mesymodules];
    fNrWords  =  new Short_t[nr_mesymodules];
    fTimestamp = new Long64_t[nr_mesymodules];
    fData      = new Int_t[nr_mesymodules][34];
    fScaler = new UInt_t[34];

    InitImp();
  }

  void KoaRaw::FillKoala(koala_event* koala)
  {
    DecodeKoala(koala);
    FillKoalaImp();
  }

  void KoaRaw::FillEms(ems_event* ems)
  {
    if(ems->tv_valid && ems->scaler_valid){
      DecodeEms(ems);
      FillEmsImp();
    }
  }

  void KoaRaw::Done()
  {
    fFile->cd();
    DoneImp();
  }


  void KoaRaw::DecodeKoala(koala_event* koala)
  {
    mxdc32_event *event;
    for(int mod=0;mod<nr_mesymodules;mod++){
      event = koala->modules[mod];
      //
      fTimestamp[mod] = event->timestamp;
      fModuleId[mod]= (event->header>>16)&0xff;
      fNrWords[mod] = event->header&0xfff;
      switch(mesymodules[mod].mesytype)
        {
        case mesytec_madc32:
          {
            fResolution[mod] = (event->header>>12)&0x7;
            for(int ch=0;ch<32;ch++){
              if(event->data[ch]){
                if((event->data[ch]>>14)&0x1){
                  fData[mod][ch] = ADC_OVERFLOW;
                }
                else{
                  switch(fResolution[mod])
                    {
                    case 0:
                      fData[mod][ch] = (event->data[ch])&0x7ff;
                      break;
                    case 1:
                    case 2:
                      fData[mod][ch] = (event->data[ch])&0xfff;
                      break;
                    case 3:
                    case 4:
                      fData[mod][ch] = (event->data[ch])&0x1fff;
                      break;
                    default:
                      printf("Unknown resolution (KoaRawSimple::Decode)\n");
                    }
                }
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        case mesytec_mqdc32:
          {
            for(int ch=0;ch<32;ch++){
              if(event->data[ch]){
                if((event->data[ch]>>15)&0x1){
                  fData[mod][ch] = QDC_OVERFLOW;
                }
                else{
                  fData[mod][ch] = (event->data[ch])&0xfff;
                }
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        case mesytec_mtdc32:
          {
            fResolution[mod] = (event->header>>12)&0xf;
            for(int ch=0;ch<34;ch++){
              if(event->data[ch]){
                fData[mod][ch] = (event->data[ch])&0xffff;
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        default:
          break;
        }
    }
  }

  void KoaRaw::DecodeEms(ems_event* ems)
  {
    for(int i=0;i<34;i++){
      fScaler[i]=ems->scaler[i];
    }
    //
    fEmsTimeSecond=ems->tv.tv_sec;
    fEmsTimeUSecond=ems->tv.tv_usec;
  }
}
