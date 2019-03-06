#include "KoaParsers.hxx"
#include "ems_data.hxx"
#include "mxdc32_data.hxx"
#include "KoaLoguru.hxx"
#include "decode_util.hxx"

#define TIMESTR "%Y-%m-%d %H:%M:%S %Z"

namespace DecodeUtil
{
  const char*
  ParserVirtual::GetName()
  {
    return fName;
  }

  int
  ParserTime::Parse(const uint32_t *buf, int size)
  {
    if (size!=2) {
      LOG_F(ERROR,"timestamp has wrong size: %d instead of 2\n", size);
      dump_data(GetName(), buf, size, 0);
      return -1;
    }

    ems_event* event_data=fPrivate->get_prepared();
    event_data->tv.tv_sec =buf[0];
    event_data->tv.tv_usec=buf[1];
    event_data->tv_valid=true;

#if 0
    char ts[50];
    struct tm* tm;
    time_t tt;

    tt=event_data->tv.tv_sec;
    tm=localtime(&tt);
    strftime(ts, 50, TIMESTR, tm);
    printf("timestamp[%d]: %s %06ld\n", event_data->event_nr,
           ts, event_data->tv.tv_usec);
#endif

    return 0;
  }

  //
  int
  ParserScalor::Parse(const uint32_t *buf, int size)
  {
    if (size!=32) {
      printf("scaler data have wrong size: %d instead of 32\n",
             size);
      dump_data(GetName(), buf, size, 0);
      return -1;
    }

    ems_event* event_data=fPrivate->get_prepared();
    for (int i=0; i<32; i++) {
      event_data->scaler[i]=buf[i];
    }
    event_data->scaler_valid=true;

#if 0
    printf("scaler:\n");
    for (i=0; i<32; i++) {
      printf("%10x%s", event_data->scaler[i], (i+1)%4?"":"\n");
    }
#endif

    return 0;
  }


  //
  int
  ParserMxdc32::Parse(const uint32_t *buf, int size)
  {
    mxdc32_private *priv=0;
    mxdc32_event *event=0;
    int mod=-1;

    for (int i=0; i<size; i++) {
        uint32_t d=buf[i];

        switch ((d>>30)&3) {
        case 0x1: { /* header */
                int id=(d>>16)&0xff;
                mod=mxdc32_get_idx_by_id(id);
                if (mod<0) {
                    LOG_S(ERROR) <<"mxdc32 private data not found";
                    return 0;
                }
                priv= fPrivate+mod;
                event=priv->prepare_event();

                event->header=d;
                event->len=d&0xfff;
                priv->statist.events++;
                priv->statist.words+=event->len;
                event->evnr=priv->statist.events;
            }
            break;
        case 0x0: /* data, ext. timestamp or dummy */
            switch ((d>>22)&0xff) {
            case 0x00: /* dummy */
                /* do nothing */
                break;
            case 0x10: { /* data */
                    if (event==0) {
                        LOG_F(ERROR,"mxdc32 event buffer not allocated, as NO HEADER found in this event");
                        return -1;
                    }
                    if (mod>=0 && (mesymodules[mod].mesytype==mesytec_mtdc32 || mesymodules[mod].mesytype==mesytec_mqdc32)) {
                        int chan=(d>>16)&0x3f;
                        if (event->data[chan]) {
                          LOG_S(WARNING) << "TDC: chan "<<chan<<" already filled";
                        } else if(chan>33){
                          LOG_F(ERROR,"channel %d not valid in %08x\n", chan, d);
                          return -1;
                        }else {
                          event->data[chan]=d;
                        }
                    } else {
                        int chan=(d>>16)&0x3f;
                        if (chan>33) {
                            LOG_F(ERROR,"channel %d not valid in %08x\n", chan, d);
                            return -1;
                        }
                        event->data[chan]=d;
                    }
                }
                break;
            case 0x12: { /* extended timestamp */
                    if (event==0) {
                        LOG_F(ERROR,"mxdc32 event buffer not allocated, as NO HEADER found in this event");
                        return -1;
                    }
                    event->ext_stamp=d&0xffff;
                    event->ext_stamp_valid=true;
                }
                break;
            default:
                LOG_F(WARNING,"mxdc32: illegal data word %08x\n", d);
            }
            break;
        case 0x3: { /* footer */
                if (event==0) {
                    LOG_F(ERROR,"mxdc32 event buffer not allocated, as NO HEADER found in this event");
                    return -1;
                }
                event->footer=d;
                int64_t stamp=d&0x3fffffff;
                event->timestamp=stamp;
                if (event->ext_stamp_valid)
                    event->timestamp|=event->ext_stamp<<30;
                priv->store_event();
                event=0;
            		mod=-1;
            }
            break;
        default: /* does not exist */
            LOG_F(WARNING,"mxdc32: illegal data word %08x\n", d);
            break;
        }
    }

    return 0;
  }

}
