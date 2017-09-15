/*
 * ems/events++/event2tade_2007.cc
 * 
 * special version for TOF beamtime May/June 2007
 * 
 * created 2007-Jun-26 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <vector>
#include <queue>
#include <map>
#include <errno.h>
#include <xdrstring.h>
#include "triple.cc"
#include "compressed_io.hxx"
#include "ems_objects.hxx"
#include "event2tade.hxx"
#include <versions.hxx>

VERSION("2007-Jun-26", __FILE__, __DATE__, __TIME__,
"$ZEL: event2tade_2007.cc,v 1.1 2007/06/26 20:48:54 wuestner Exp $")
#define XVERSION

namespace cl2tade {

/* data contained in detdef.lis */
typedef triple<string, int, int> crate_slot_chan;
typedef pair<int, int> det_elem;
typedef pair<crate_slot_chan, det_elem> detdef_pair;
typedef map<crate_slot_chan, det_elem> detdef_map;
detdef_map detdef_T; // TDCs
detdef_map detdef_Q; // QDCs
detdef_map detdef_S; // Scaler

/* data contained in moddef.lis */
typedef pair<string, int> crate_slot;
map<crate_slot, string> moddef;

struct tqval {
    tqval():t(-1), q(-1) {}
    int t;
    int q;
};

void
clear(void)
{
    detdef_T.clear();
    detdef_S.clear();
    detdef_Q.clear();
    moddef.clear();
}

class Event {
  public:
    void clear(void);
    int to_tadefile(FILE*);
    int to_scalerfile(FILE*);
    ems_u32 evno;
    ems_u32 runno;
    ems_u32 trigger;        // from synch module
    ems_u32 triggerPattern; // from C219
    map<det_elem, ems_u64> scalermap;
    map<det_elem, tqval> tqmap;
} event;

void
Event::clear(void)
{
    evno=(ems_u32)-1;
    runno=(ems_u32)-1;
    triggerPattern=(ems_u32)-1;
    scalermap.clear();
    tqmap.clear();
}

void
dump_event(void)
{
    printf("scaler: num=%llu\n", (unsigned long long)event.scalermap.size());
    map<det_elem, ems_u64>::iterator Siter=event.scalermap.begin();
    while (Siter!=event.scalermap.end()) {
        ems_u64 val=(*Siter).second;
        const det_elem& elem=(*Siter).first;
        printf("scaler %d elem %d val %llu\n", elem.first, elem.second,
                (unsigned long long)val);
        Siter++;
    }

    printf("tq: num=%llu\n", (unsigned long long)event.tqmap.size());
    map<det_elem, tqval>::iterator Diter=event.tqmap.begin();
    while (Diter!=event.tqmap.end()) {
        tqval& tqv=(*Diter).second;
        const det_elem& elem=(*Diter).first;
        printf("det %d elem %d tdc %d qdc %d\n", elem.first, elem.second,
            tqv.t, tqv.q);
        Diter++;
    }
}

int
Event::to_tadefile(FILE* f)
{
    fprintf(f, "%d %llu %d %d  Event\n",
            evno, (unsigned long long)tqmap.size(), runno, triggerPattern);
    map<det_elem, tqval>::iterator iter=event.tqmap.begin();
    while (iter!=event.tqmap.end()) {
        tqval& tqv=(*iter).second;
        const det_elem& elem=(*iter).first;
        fprintf(f, "%6d %5d %5d %5d\n", tqv.t, tqv.q, elem.first, elem.second);
        iter++;
    }
    return 1;
}

int
Event::to_scalerfile(FILE* f)
{
    if (!scalermap.size())
        return 0;
    fprintf(f, "%d %llu %d %d Scaler\n", evno,
            (unsigned long long)scalermap.size(), runno, trigger);
    map<det_elem, ems_u64>::iterator iter=scalermap.begin();
    while (iter!=event.scalermap.end()) {
        ems_u64 val=(*iter).second;
        const det_elem& elem=(*iter).first;
        fprintf(f, "%6d %5d %llu\n", elem.first, elem.second,
                (unsigned long long)val);
        iter++;
    }
    return 1;
}

void
dump_sev(sev_object& sev, int max=0)
{
    int maxx=sev.data.size(), i;
    fprintf(stderr, "dump_sev: size=%d\n", maxx);
    if (max && (maxx>max)) maxx=max;
    for (i=0; i<maxx; i++) {
        if (!(i&7)) fprintf(stderr, "%s%2d ", i?"\n":"", i);
        fprintf(stderr, "%08x ", sev.data[i]);
    }
    fprintf(stderr, "\n");
}

void
dump_detdef_Q(detdef_map& q)
{
    fprintf(stderr, "detdef_Q:\n");
    detdef_map::iterator iter=q.begin();
    while (iter!=q.end()) {
        const detdef_pair& pair=*iter;
        const crate_slot_chan& csc=pair.first;
        const det_elem& de        =pair.second;
        const string& crate=csc.first;
        int slot     =csc.second;
        int channel  =csc.third;
        int det =de.first;
        int elem=de.second;
        fprintf(stderr, "%s %2d %2d -> %2d %2d\n", crate.c_str(), slot, channel,
                det, elem);
        iter++;
    }
}

int
parity(ems_u32 v)
{
    v^=(v>>16);
    v^=(v>>8);
    v^=(v>>4);
    v^=(v>>2);
    v^=(v>>1);
    return v&1;
}

int
decode_LC_ADC_1881M(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    if (d.size()-1!=d[0]) {
        printf("LC_ADC_1881M: sev size=%llu, data[0]=%d\n", (unsigned long long)d.size(), d[0]);
        return -1;
    }
    // check parity
    for (int i=d.size()-1; i>=1; i--) {
        if (parity(d[i])!=0) {
            printf("LC_ADC_1881M: wrong parity of word %d: 0x%08x\n",
                i, d[i]);
            return 0;
        }
    }
    vector<ems_u32>::size_type idx=1;
    while (idx<d.size()) {
        ems_u32 head=d[idx++];
        int wordcount=(head&0x7f)-1; // we do not count the header itself
        if (idx+wordcount>d.size()) {
            printf("LC_ADC_1881M: wordcount ecceeds sev size\n");
            return -1;
        }
        int geo=(head>>27)&0x1f;
        for (int i=0; i<wordcount; i++) {
            ems_u32 v=d[idx++];
            if ((v^head)&0xf8000000) {
                printf("LC_ADC_1881M: geo changed inside module\n");
                dump_sev(sev);
                return -1;
            }
            int val=v&0x3fff;
            int channel=(v>>17)&0x3f;

            detdef_map::iterator detdef_iter;
            detdef_iter=detdef_Q.find(make_triple(descr.crate, geo, channel));
            if (detdef_iter==detdef_Q.end()) {
                if (ems.ignore_missing) {
                    ems.ignored_missings++;
                    continue;
                } else {
                    printf("LC_ADC_1881M: no QDC entry in detdef "
                            "for crate %s slot %d channel %d\n",
                            descr.crate.c_str(), geo, channel);
dump_sev(sev);
dump_detdef_Q(detdef_Q);
                    return -1;
                }
            }
            detdef_pair dp=*detdef_iter;
            det_elem& detchan=dp.second;

            map<det_elem, tqval>::iterator tqiter=event.tqmap.find(detchan);
            if (tqiter==event.tqmap.end()) {
                tqval tqv;
                tqv.q=val;
                tqv.t=-1;
                event.tqmap[detchan]=tqv;
            } else {
                tqval& tqv=(*tqiter).second;
                tqv.q=val;
            }
        }
    }
    if (idx!=d.size()) {
        printf("LC_ADC_1881M: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_PH_10CX(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    if (d.size()-1!=d[0]) {
        printf("PH_10CX: sev size=%llu, data[0]=%d\n", (unsigned long long)d.size(), d[0]);
        return -1;
    }

    vector<ems_u32>::size_type idx=1;
    while (idx<d.size()) {
        ems_u32 h=d[idx++];
        // int id=(h>>24)&0xff;
        // int null=(h>>21)&7;
        int geo=(h>>16)&0x1f;
        int chans=(h>>10)&0x3f;
        // int evnr=h&0x3ff;
        //printf("%08x id %d null %d geo %d chans %d event %d\n",
        //    h, id, null, geo, chans, event);
        int words=(chans+1)/2;
        for (int i=0; i<words; i++) {
            ems_u32 v=d[idx++];
            ems_u16 v2[2];
            v2[0]=(v>>16)&0xffff;
            v2[1]=v&0xffff;
            //printf("  > %04x %04x <\n", v2[0], v2[1]);
            for (int j=0; j<2; j++) {
                ems_u16 v=v2[j];
                if (!(v&0x8000)) {
                    int chan=(v>>10)&0x1f;
                    int val=v&0x3ff;
                    //printf("    chan %d val %d\n", chan, val);
                    // we have geo, channel and value
                    // but we do not know yet whether it is time or charge
                    // we will ask the detdef map ...
                    detdef_map::iterator detdef_iterT;
                    detdef_map::iterator detdef_iterQ;
                    detdef_iterT=detdef_T.find(
                            make_triple(descr.crate, geo, chan));
                    detdef_iterQ=detdef_Q.find(
                            make_triple(descr.crate, geo, chan));
                    if        ((detdef_iterQ==detdef_Q.end()) &&
                               (detdef_iterT!=detdef_T.end())) {
                        // it is as TDC
                        detdef_pair dp=*detdef_iterT;
                        det_elem& detchan=dp.second;

                        map<det_elem, tqval>::iterator tqiter=
                                event.tqmap.find(detchan);
                        if (tqiter==event.tqmap.end()) {
                            tqval tqv;
                            tqv.t=val;
                            tqv.q=-1;
                            event.tqmap[detchan]=tqv;
                        } else {
                            tqval& tqv=(*tqiter).second;
                            tqv.t=val;
                        }
                    } else if ((detdef_iterQ!=detdef_Q.end()) &&
                               (detdef_iterT==detdef_T.end())){
                        // it is a QDC
                        detdef_pair dp=*detdef_iterQ;
                        det_elem& detchan=dp.second;

                        map<det_elem, tqval>::iterator tqiter=
                                event.tqmap.find(detchan);
                        if (tqiter==event.tqmap.end()) {
                            tqval tqv;
                            tqv.q=val;
                            tqv.t=-1;
                            event.tqmap[detchan]=tqv;
                        } else {
                            tqval& tqv=(*tqiter).second;
                            tqv.q=val;
                        }
                    } else if ((detdef_iterQ==detdef_Q.end()) &&
                               (detdef_iterT==detdef_T.end())){
                        // it is neither a Q- nor a T-DC
                        if (ems.ignore_missing) {
                            ems.ignored_missings++;
                            continue;
                        } else {
                            printf("PH_10CX: no entry in detdef "
                                    "for crate %s slot %d channel %d\n",
                                    descr.crate.c_str(), geo, chan);
                            return -1;
                        }
                    } else {
                        // it is a  QTDC !!
                        printf("PH_10CX: entries for both TDC and QDC in detdef "
                                "for crate %s slot %d channel %d\n",
                                descr.crate.c_str(), geo, chan);
                        return -1;
                    }
                } else {
                    //printf("    invalid\n");
                }
            }
        }
    }
    if (idx!=d.size()) {
        printf("PH_10CX: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_LC_TDC_1877(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    dump_sev(sev);
    printf("decode_LC_TDC_1877 not implemented\n");
    return -1;
}

int
decode_LC_ADC_1875A(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    if (d.size()-1!=d[0]) {
        printf("LC_ADC_1875A: sev size=%llu, data[0]=%d\n", (unsigned long long)d.size(), d[0]);
        return -1;
    }

    vector<ems_u32>::size_type idx=1;
    while (idx<d.size()) {
        ems_u32 v=d[idx++];
        int geo=(v>>27)&0x1f;
        //int event=(v>>24)&7;
        int range=(v>>23)&1;
        int channel=(v>>16)&0x7f;
        int val=v&0xfff;
        if (range) val*=8;  

        detdef_map::iterator detdef_iter;
        detdef_iter=detdef_T.find(make_triple(descr.crate, geo, channel));
        if (detdef_iter==detdef_T.end()) {
            if (ems.ignore_missing) {
                ems.ignored_missings++;
                continue;
            } else {
                printf("LC_ADC_1875A: no TDC entry in detdef "
                        "for crate %s slot %d channel %d\n",
                        descr.crate.c_str(), geo, channel);
                return -1;
            }
        }
        detdef_pair dp=*detdef_iter;
        det_elem& detchan=dp.second;

        map<det_elem, tqval>::iterator tqiter=event.tqmap.find(detchan);
        if (tqiter==event.tqmap.end()) {
            tqval tqv;
            tqv.t=val;
            tqv.q=-1;
            event.tqmap[detchan]=tqv;
        } else {
            tqval& tqv=(*tqiter).second;
            tqv.t=val;
        }
    }
    if (idx!=d.size()) {
        printf("LC_ADC_1875A: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_buffercheck(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    /* ignored at this time */
    //dump_sev(sev);
    return 0;
}

int
decode_TrigData(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    /* ignored */
    return 0;
}

void
dump_scalermap(map<det_elem, ems_u64> smap)
{
    map<det_elem, ems_u64>::iterator iter=smap.begin();
    while (iter!=smap.end()) {
        pair<det_elem, ems_u64> paar;
        det_elem elem=iter->first;
        ems_u64 val=iter->second;
        printf("<%2d %2d> <%llu>\n", elem.first, elem.second,
            (unsigned long long)val);
        iter++;
    }
}

int
insert_scaler_value(isdescriber& descr, ems_object& ems,
        int channel, ems_u64 val,
        const char* text)
{
    detdef_map::iterator detdef_iter;
    detdef_iter=detdef_S.find(make_triple(descr.crate, descr.is_id, channel));
    if (detdef_iter==detdef_S.end()) {
        if (ems.ignore_missing) {
            ems.ignored_missings++;
            return 0;
        } else {
            printf("%s: no entry in detdef_S "
                    "for crate %s IS %d channel %d\n",
                    text, descr.crate.c_str(), descr.is_id, channel);
            return -1;
        }
    }
    detdef_pair dp=*detdef_iter;
    det_elem& detchan=dp.second;

    map<det_elem, ems_u64>::iterator iter=event.scalermap.find(detchan);
    if (iter!=event.scalermap.end()) {
        const det_elem& elem=iter->first;
        printf("%s: duplicate value "
                "for crate %s IS %d channel %d "
                "(detector %d, element %d)\n",
                text, descr.crate.c_str(), descr.is_id, channel,
                elem.first, elem.second);
        //dump_scalermap(event.scalermap);
        return -1;
    }
    event.scalermap[detchan]=val;
    return 0;    
}

int
decode_Timestamp(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    if (sev.data.size()!=2) {
        printf("invalid sev for timestamp:\n");
        dump_sev(sev);
        printf("\n");
        return -1;
    }
    // seconds
    if (insert_scaler_value(descr, ems, 0, sev.data[0], "Timestamp")<0)
        return -1;
    // mikroseconds
    if (insert_scaler_value(descr, ems, 1, sev.data[1], "Timestamp")<0)
        return -1;

    return 0;
}

int
decode_sis3600(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    dump_sev(sev);
    printf("decode_sis3600 not implemented\n");
    return -1;
}
int decode_v792cblt(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    dump_sev(sev);
    printf("decode_v792cblt not implemented\n");
    return -1;
}
int decode_runnr(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    if (sev.data.size()!=1) {
        printf("invalid sev for runnr:\n");
        dump_sev(sev);
        printf("\n");
        return -1;
    }
    ems_u32 runnr=sev.data[0];
    event.runno=runnr;
//     if (runnr!=???) {
//         printf("runnr changed: %d --> %d\n", ???, runnr);
//         return -1;
//     }
    return 0;
}
int
decode_sis3800Shadow(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    vector<ems_u32>::size_type idx=0;

    ems_u32 modules=d[idx++];
    int channel=0;
    for (ems_u32 m=0; m<modules; m++) {
        ems_u32 count=d[idx++];
        if (count!=32) {
            printf("SIS3800Shadow: wrong wordcount %d\n", count);
            return -1;
        }
        for (ems_u32 i=0; i<count; i++) {
            ems_u64 high=d[idx++];
            ems_u64 low=d[idx++];
            ems_u64 val=(high<<32)+low;
            if (insert_scaler_value(descr, ems, channel, val, "SIS3800Shadow")<0)
                return -1;
            channel++;
        }
    }
    if (idx!=d.size()) {
        printf("SIS3800Shadow: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_LC_SCALER_2155(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    vector<ems_u32>::size_type idx=0;

    int channel=0;
    while (idx<d.size()) {
        ems_u32 count=d[idx++];
        if (count!=12) {
            printf("LC_SCALER_2155: wrong wordcount %d\n", count);
            return -1;
        }
        for (ems_u32 i=0; i<count; i++) {
            ems_u64 high=d[idx++];
            ems_u64 low=d[idx++];
            ems_u64 val=(high<<32)+low;
            if (insert_scaler_value(descr, ems, channel, val, "LC_SCALER_2155")<0)
                return -1;
            channel++;
        }
    }
    if (idx!=d.size()) {
        printf("LC_SCALER_2155: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_LC_SCALER_4434(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    vector<ems_u32> d=sev.data;
    vector<ems_u32>::size_type idx=0;

    int channel=0;
    while (idx<d.size()) {
        ems_u32 count=d[idx++];
        // format depends on trigger ID!!!
        if ((count!=32) && (count!=1)) {
            printf("LC_SCALER_4434: wrong wordcount %d\n", count);
            dump_sev(sev);
            return -1;
        }
        for (ems_u32 i=0; i<count; i++) {
            ems_u64 high=d[idx++];
            ems_u64 low=d[idx++];
            ems_u64 val=(high<<32)|low;
            if (insert_scaler_value(descr, ems, channel, val, "LC_SCALER_4434")<0)
                return -1;
            channel++;
        }
    }
    if (idx!=d.size()) {
        printf("LC_SCALER_4434: length mismatch of sev\n");
        return -1;
    }
    return 0;
}

int
decode_CAEN_LATCH_C219(isdescriber& descr, ems_object& ems, sev_object& sev)
{
    if (sev.data.size()!=1) {
        printf("invalid sev for LATCH_C219:\n");
        dump_sev(sev);
        printf("\n");
        return -1;
    }
    event.triggerPattern=sev.data[0]&0xffff;
    return 0;
}

isdescriber descr[]={
    {   0, "t01", decode_runnr},
    {   4, "t01", decode_LC_SCALER_2155},
    {   5, "t01", decode_LC_SCALER_4434},
    {  10, "t01", decode_CAEN_LATCH_C219},
    {1000, "t01", decode_TrigData},
    {  50, "t02", decode_buffercheck},
    {  51, "t03", decode_buffercheck},
    {  52, "t04", decode_buffercheck},
    {  53, "t06", decode_buffercheck},
    {  32, "t02", decode_LC_ADC_1881M},
    {  33, "t02", decode_PH_10CX},
    {  34, "t02", decode_LC_TDC_1877},
    {  35, "t02", decode_LC_ADC_1875A},
    {1002, "t02", decode_TrigData},
    {  20, "t03", decode_LC_ADC_1881M},
    {  21, "t03", decode_PH_10CX},
    {  27, "t03", decode_LC_TDC_1877},
    {  28, "t03", decode_LC_ADC_1875A},
    {1003, "t03", decode_TrigData},
    {  22, "t04", decode_LC_ADC_1881M},
    {  23, "t04", decode_PH_10CX},
    {  24, "t04", decode_LC_TDC_1877},
    {  30, "t04", decode_LC_ADC_1875A},
    {1004, "t04", decode_TrigData},
    {  26, "t06", decode_LC_ADC_1881M},
    {  25, "t06", decode_PH_10CX},
    {  29, "t06", decode_LC_TDC_1877},
    {  31, "t06", decode_LC_ADC_1875A},
    {1006, "t06", decode_TrigData},
    {   1, "t05", decode_Timestamp},
    {   2, "t05", decode_sis3600},
    {   3, "t05", decode_sis3800Shadow},
    {0xffffffff, "", 0}
};

int
decode_to_tade_2007(deque<sev_object>& evdata, ems_object& ems,
        int nr_files,
        compressed_output *files,
        int* nevents)
{
    static int count=0;
    if (evdata.empty()) {
        if (ems.next_evno>0)
            cerr<<"decode_to_tade: empty event ("<<ems.next_evno<<")"<<endl;
        return 0;
    }
    event.clear();
    event.evno=evdata.front().event_nr;
    event.trigger=evdata.front().trigger;

    do {
        sev_object& sev=evdata.front();
        ems_u32 is_id=sev.sev_id;
        // find the decoder
        int i=0;
        while ((descr[i].is_id!=is_id) && (descr[i].is_id!=0xffffffff)) i++;
        if (descr[i].is_id==0xffffffff) {
            printf("decode_to_tade: unknown is %d\n", is_id);
            return -1;
        }
        int res=descr[i].decoder(descr[i], ems, sev);
        if (res<0)
            return -1;

        evdata.pop_front();
    } while (!evdata.empty());

    count++;

    //if (count<10) dump_event();

    if (event.trigger&1) { // regular event
        {
            int n=event.to_tadefile(files[0].file());
            if (nevents) nevents[0]+=n;
        }
    }
    if (true/*event.trigger&8*/) { // scaler event
        int n=event.to_scalerfile(files[1].file());
        if (nevents) nevents[3]+=n;
    }

    return 0;
}

/*
 * This procedure should not exist!
 * It is necessary until detdef.lis contains the scaler definitions
 */
void
add_scaler_to_detdef_map(void)
{
    string crate;
    // t05; timestamp
    crate="t05";
    {
        detdef_pair dpt(make_triple(crate, 1, 0), make_pair(0, 0));
        detdef_S.insert(dpt);
    }
    {
        detdef_pair dpt(make_triple(crate, 1, 1), make_pair(0, 1));
        detdef_S.insert(dpt);
    }
    // LC_SCALER_2551 in t01; slot 13 and 14
    crate="t01";
    int isid=4;
    for (int detector=1, channel=0;  detector<=2; detector++) {
        for (int element=0; element<12; element++, channel++) {
            detdef_pair dpt(make_triple(crate, isid, channel),
                                make_pair(detector, element));
            detdef_S.insert(dpt);
        }
    }
    // LC_SCALER_4434 in t01; slot 17, 18, 19
    crate="t01";
    isid=5;
    for (int detector=3, channel=0;  detector<=5; detector++) {
        for (int element=0; element<32; element++, channel++) {
            detdef_pair dpt(make_triple(crate, isid, channel),
                                make_pair(detector, element));
            detdef_S.insert(dpt);
        }
    }
    // 6 SIS_SCALER_3800 in t05; addr 0x2000 ... 0x7000
    crate="t05";
    isid=3;
    for (int detector=6, channel=0;  detector<=11; detector++) {
        for (int element=0; element<32; element++, channel++) {
            detdef_pair dpt(make_triple(crate, isid, channel),
                                make_pair(detector, element));
            detdef_S.insert(dpt);
        }
    }
}

/*
 * parse_detdef reads detdef.lis and creates a map
 * <crate, slot, cannel> --> <Detector, Element>
 */
int
parse_detdef(file_object& file, bool ignore)
{
    istringstream ff(file.content);
    string ss;
    int linenr=0;
    int warn_99=0;

    while (getline(ff, ss)) {
        linenr++;
//printf(">>> %s <<< %d\n", ss.c_str(), linenr);
        ss.erase(0, ss.find_first_not_of(" \t")); // erase leading white space
        ss.erase(ss.find_last_not_of(" \t")+1);   // erase trailing white space
        if (ss.empty() || ss[0]=='!' || ss[0]=='#' || ss[0]==';')
            continue;
        istringstream is(ss);
        string Tcrate, Qcrate;
        int detector, element;
        int Taddr, Tchannel, Tlth, Tuth, Tped;
        int Qaddr, Qchannel, Qlth, Quth, Qped;
        is>>detector>>element
          >>Tcrate
          >>Taddr>>Tchannel >>Tlth>>Tuth>>Tped
          >>Qcrate
          >>Qaddr>>Qchannel >>Qlth>>Quth>>Qped;

        pair<detdef_map::iterator, bool> rp;
        if (Tcrate!="none") {
            //detdef_T[make_triple(Tcrate, Taddr, Tchannel)]=make_pair(detector, element);
            detdef_pair dpt(make_triple(Tcrate, Taddr, Tchannel),
                            make_pair(detector, element));
            rp=detdef_T.insert(dpt);
            if (!rp.second) {
                if (detector==99) {
                    warn_99++;
                } else {
                    printf("duplicate TDC entry in detdef: "
                            "crate %s slot %d channel %d (line %d)\n",
                        Tcrate.c_str(), Taddr, Tchannel, linenr);
                    printf("%s\n", ss.c_str());
                    if (ignore)
                        printf("ignored!\n");
                    else
                        return -1;
                }
            }
        }

        if (Qcrate!="none") {
            //detdef_Q[make_triple(Qcrate, Qaddr, Qchannel)]=make_pair(detector, element);
            detdef_pair dpq(make_triple(Qcrate, Qaddr, Qchannel),
                            make_pair(detector, element));
            rp=detdef_Q.insert(dpq);
            if (!rp.second) {
                if (detector==99) {
                    warn_99++;
                } else {
                    printf("duplicate QDC entry in detdef: "
                            "crate %s slot %d channel %d (line %d)\n",
                        Qcrate.c_str(), Qaddr, Qchannel, linenr);
                    printf("%s\n", ss.c_str());
                    if (ignore)
                        printf("ignored!\n");
                    else
                        return -1;
                }
            }
        }
    }
    if (warn_99) {
        cerr<<warn_99<<" duplicate entr"<<((warn_99==1)?"y":"ies")<<" for detector 99!"<<endl;
    }
    add_scaler_to_detdef_map();
    return 0;
}

/*
 * parse_moddef reads moddef.lis and creates a map
 *   <crate, addr> --> module_type
 * It is useless because the association to IS is missing
 */
int
parse_moddef(file_object& file)
{
    istringstream ff(file.content);
    string ss;
    while (getline(ff, ss)) {
        ss.erase(0, ss.find_first_not_of(" \t"));
        ss.erase(ss.find_last_not_of(" \t")+1);
        if (ss.empty() || ss[0]=='!' || ss[0]=='#')
            continue;
        //cout<<">>>"<<ss<<"<<<"<<endl;
        istringstream is(ss);
        string key, crate;
        int slot;
        is>>key;
        //cout<<"Key="<<key<<endl;
        if (key=="CRATE") {
            is>>crate;
            //cout<<"crate "<<crate<<endl;
        } else {
            is>>slot;
            //cout<<"Module "<<key<<" slot "<<slot<<" useped "<<(useped?"True":"False")<<endl;
            moddef[make_pair(crate, slot)]=key;
        }
    }
    return 0;
}


} // end of namespace cl2tade

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
