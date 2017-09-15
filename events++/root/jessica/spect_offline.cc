/*
 * jessica_spect_offline.c
 * 
 * created 30.Oct.2003 
 * $ZEL: spect_offline.cc,v 1.1 2004/02/11 12:37:40 wuestner Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <iostream.h>

#ifndef __CINT__
#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TTimer.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TPaveLabel.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TObjectTable.h>
#include <TFormula.h>
#include <TFrame.h>
#include <TRandom.h>
#endif

#include "spect_decode.hxx"

static int scale;
static int range;
static int binnum;
static float lowerbin;
static float upperbin;
static const char* title;
static TH1F* h1f;

static struct event_buffer eb;
static ems_u32* cldata;
static int cldatasize;
static int evcount;
static int selected_sev;
static int selected_channel;

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h] -s sev -c chan -m max [-q scale] -t title\n", argv0);
    printf("  -h: this help\n");
    printf("  -s: sev_id\n");
    printf("  -c: channel\n");
    printf("  -r: range\n");
    printf("  -q: scale\n");
    printf("  -t: title\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    struct servent* service;
    char* p;
    selected_channel=0;
    range=0;
    scale=1;
    title="unknown";

    err=0;
    while (!err && ((c=getopt(argc, argv, "hs:c:q:r:t:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 's': selected_sev=atoi(optarg); break;
        case 'c': selected_channel=atoi(optarg); break;
        case 'q': scale=atoi(optarg); break;
        case 'r': range=atoi(optarg); break;
        case 't': title=optarg; break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }

    if ((selected_sev<2) || (selected_sev>4)) {
        printf("only sev 2, 3 or 4 is allowed\n");
        return -1;
    }

    if (range<=0) {
        printf("invalid range %d\n", range);
        return -1;
    }

    if (scale<=0) {
        printf("invalid scale %d\n", scale);
        return -1;
    }

    binnum=range/scale; if (range%scale) binnum++;
    lowerbin=-scale*0.5;
    upperbin=binnum*scale+lowerbin;
    printf("binnum=%d low=%f upper=%f\n", binnum, lowerbin, upperbin);
    return 0;
}
/******************************************************************************/
static int
adjust_event_buffer(struct event_buffer* eb)
{
    if (eb->max_subevents<eb->subevents) {
        if (eb->subeventptr) free(eb->subeventptr);
        eb->subeventptr=(ems_u32**)malloc(eb->subevents*sizeof(ems_u32*));
        if (!eb->subeventptr) {
            printf("malloc %d ptrs for subeventptr: %s\n", eb->subevents, strerror(errno));
            return -1;
        }
        eb->max_subevents=eb->subevents;
    }

    return 0;
}
/******************************************************************************/
static int
xread(int p, ems_u32* buf, int len)
{
    int restlen, da;

    restlen=len*sizeof(ems_u32);
    while (restlen) {
        da=read(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2], v;
    ems_u32* data;
    int wenden, size, evnum, i;

    if (xread(p, head, 2)) return -1;
    switch (head[1]) {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default:
        printf("unkown endien 0x%x\n", head[1]);
        return -1;
    }
    size=(wenden?SWAP_32(head[0]):head[0])-1;
    if (size>cldatasize) {
        if (cldata) free(cldata);
        cldata=(ems_u32*)malloc(size*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata, size)) return -1;
    if (wenden) {
        for (i=0; i<size; i++) cldata[i]=SWAP_32(cldata[i]);
    }

    switch (cldata[0]) { /* clustertype */
    case 0: /* events */
        break;
    case 1: /* ved_info */
    case 2: /* text */
    case 3: /* wendy_setup */
    case 4: /* file */
        return 0;
    case 5: /* no_more_data */
        printf("no more data\n");
        return 1;
    }

    data=cldata+1;
    v=*data++;
    data+=v;   /* skip optional fields */
    data++;    /* skip flags */
    data++;    /* skip VED_ID */
    data++;    /* skip fragment_id */

    evnum=*data++;
    for (i=0; i<evnum; i++) {
        int evsize=*data++;
        eb.data=data;

        eb.size=evsize-3;
        eb.evno=*eb.data++;
        eb.trigno=*eb.data++;
        eb.subevents=*eb.data++;
        if (adjust_event_buffer(&eb)<0) return -1;

        if (proceed_event(&eb, h1f, selected_sev, selected_channel)<0)
                return -1;
        evcount++;
        data+=evsize;
    }

    return 0;
}
/******************************************************************************/
main(int argc, char *argv[])
{
    TCanvas* c1;
    TPad *pad1;

    if (readargs(argc, argv)) return 1;

    TApplication app("Jessica Spect", 0, 0);

    gStyle->SetOptStat(1111110);
    c1 = new TCanvas("c1","Jessica",200,10,700,450);
    c1->SetFillColor(18);

    pad1 = new TPad("Pad1","The pad with the histogram",0.05,0.05,0.95,0.95,21,-1);
    pad1->Draw();

    pad1->cd();
    pad1->GetFrame()->SetFillColor(42);
    pad1->GetFrame()->SetBorderMode(-1);
    pad1->GetFrame()->SetBorderSize(5);

    h1f = new TH1F("h1f", title, binnum, lowerbin, upperbin);
    h1f->SetFillColor(45);
    h1f->Draw();
    c1->Update();

    cldata=0;
    cldatasize=0;
    memset(&eb, 0, sizeof(struct event_buffer));

    evcount=0;
    while (!read_cluster(0)) {}
    cout<<evcount<<" events read"<<endl;
    pad1->Modified();
    pad1->Update();
    app.Run(1);
    
    return 0;
}
/******************************************************************************/
/******************************************************************************/
