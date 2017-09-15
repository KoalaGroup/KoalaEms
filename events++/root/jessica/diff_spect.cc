/*
 * spect_online.cc
 * 
 * created 30.Oct.2003 
 * $ZEL: diff_spect.cc,v 1.1 2004/02/11 12:37:38 wuestner Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
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
#include <TSocket.h>
#include <TInetAddress.h>
#include <TMonitor.h>
#endif

#include "spect_decode.hxx"

static char *inname;
static int inport;
static char inhost[1024];
static int scale;
static int range;
static int binnum;
static float lowerbin;
static float upperbin;
static const char* title;
static TH1F* h1f[3];
static int selected_sev;
static const int num_chan=2;
static int selected_channel[2];

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h] -s sev -c chan -m max [-q scale] -t title inpath\n", argv0);
    printf("  -h: this help\n");
    printf("  -s: sev_id\n");
    printf("  -c: channel (Must be used twice!)\n");
    printf("  -r: range\n");
    printf("  -q: scale\n");
    printf("  -t: title\n");
    printf("  input path: hostname:(portnum|portname)\n");
}
/******************************************************************************/
static int
portname2portnum(char* name)
{
    struct servent* service;
    int port;

    if ((service=getservbyname(name, "tcp"))) {
        port=ntohs(service->s_port);
    } else {
        char* end;
        port=strtol(name, &end, 0);
        if (*end!=0) {
            printf("cannot convert \"%s\" to integer.\n", name);
            port=-1;
        }
    }
    return port;
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    struct servent* service;
    char* p;
    range=0;
    scale=1;
    title="unknown";
    int chanidx=0;

    err=0;
    while (!err && ((c=getopt(argc, argv, "hs:c:q:r:t:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 's': selected_sev=atoi(optarg); break;
        case 'c':
            if (chanidx>=num_chan) {
                printf("-c used more than %d times\n", num_chan);
                err=1;
                break;
            }
            selected_channel[chanidx++]=atoi(optarg);
            break;
        case 'q': scale=atoi(optarg); break;
        case 'r': range=atoi(optarg); break;
        case 't': title=optarg; break;
        default: err=1;
        }
    }
    if (err || (chanidx!=num_chan)) {
        printusage(argv[0]);
        return -1;
    }

    if (optind>=argc) {
        printf("input path missing\n");
        printusage(argv[0]);
        return -1;
    } else
        inname=argv[optind];

    if ((p=index(inname, ':'))) {
        int idx=p-inname;
        strncpy(inhost, inname, idx);
        inhost[idx]=0;
        if ((inport=portname2portnum(p+1))<0) return 1;
    } else {
        printf("invalid socket name %s\n", inname);
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
static void
sigpipe(int num)
{}
/******************************************************************************/
static void
sigsegv(int num)
{
    *(int*)0=0;
}
/******************************************************************************/
static int
adjust_event_buffer(struct event_buffer* eb)
{
    if (eb->max_size<eb->size) {
        if (eb->data) free(eb->data);
        eb->data=(ems_u32*)malloc(eb->size*sizeof(ems_u32));
        if (!eb->data) {
            printf("malloc %d words for event: %s\n", eb->size, strerror(errno));
            return -1;
        }
        eb->max_size=eb->size;
    }

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
/*****************************************************************************/
static int
xread(TSocket* sock, ems_u32* buf, int len)
{
    int restlen, da;
    char* cbuf=(char*)buf;

    restlen=len*sizeof(ems_u32);
    while (restlen) {
        da=sock->RecvRaw(cbuf, restlen);
        if (da>0) {
            cbuf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: root error\n");
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static int
read_event(TSocket* sock, struct event_buffer* eb)
{
    size_t n;
    ems_u32 head[4];
    int swap, i;

    if (xread(sock, head, 4)<0) return -1;

    if ((head[3]&0xffff0000) || ((head[3]==0) && (head[0]&0xffff0000)))
        swap=1;
    else
        swap=0;

    if (swap) {
        for (i=0; i<4; i++) head[i]=SWAP_32(head[i]);
    }
    eb->size=head[0]-3;
    eb->evno=head[1];
    eb->trigno=head[2];
    eb->subevents=head[3];

    if (adjust_event_buffer(eb)<0) return -1;
/*
    printf("size=%d\n", eb->size);
*/
    if (xread(sock, eb->data, eb->size)<0) return -1;
    if (swap) {
        for (i=0; i<eb->size; i++) eb->data[i]=SWAP_32(eb->data[i]);
    }
    return 0;
}
/******************************************************************************/
static void
main_loop(void)
{
    TCanvas* c1;
    TPad *pad[3];
    TSocket* sock;
    TMonitor* monitor;
    struct event_buffer eb;
    int inpath, i;
    char* title[3];
    char* name[3];

    memset(&eb, 0, sizeof(struct event_buffer));

    gROOT->Reset();

    sock=new TSocket(inhost, inport);
    if (!sock->IsValid()) {
        int error=sock->GetErrorCode();
        cerr<<"socket not valid: ";
        switch (error) {
            case -1: cerr<<"creation failed"<<endl; break;
            case -2: cerr<<"bind failed"<<endl; break;
            case -3: cerr<<"connect failed"<<endl; break;
            default: cerr<<"unknown error code "<<error<<endl;
        }
        return;
    }

    cerr<<"Socket is "<<(sock->IsValid()?"valid":"invalid")<<endl;
    monitor=new TMonitor;
    monitor->Add(sock, TMonitor::kRead);

    // gStyle->SetOptStat(1111110);
    c1 = new TCanvas("c1","Jessica",200,10,700,450);
    c1->SetFillColor(18);

    pad[0]=new TPad("Pad1","left pad",  0.02, 0.52, 0.48, 0.98, 21, -1);
    pad[0]->Draw();
    pad[1]=new TPad("Pad2","right pad", 0.52, 0.52, 0.98, 0.98, 21, -1);
    pad[1]->Draw();
    pad[2]=new TPad("Pad3","lower pad", 0.02, 0.02, 0.98, 0.48, 21, -1);
    pad[2]->Draw();

    title[0]="left pad";
    title[1]="right pad";
    title[2]="lower pad";
    name[0]="left pad";
    name[1]="right pad";
    name[2]="lower pad";

    for (i=0; i<3; i++) {
        c1->cd();
        pad[i]->Draw();

        pad[i]->cd();
        pad[i]->GetFrame()->SetFillColor(42);
        pad[i]->GetFrame()->SetBorderMode(-1);
        pad[i]->GetFrame()->SetBorderSize(5);

        h1f[i]=new TH1F(name[i], title[i], binnum, lowerbin, upperbin);
        h1f[i]->SetFillColor(45);
        h1f[i]->Draw();
    }
    c1->Update();

    while (1) {
        int val, i;
        TSocket* s;
        for (i=0; i<1; i++) {
            s=monitor->Select();
            if (s!=sock) {
                cerr<<"socket mismatch"<<endl;
                return;
            }
            if (read_event(sock, &eb)<0) return;
            if (proceed_event2diff(&eb, h1f, selected_sev, selected_channel)<0)
                    return;
        }
        for (i=0; i<3; i++) {
            pad[i]->Modified();
            pad[i]->Update();
        }
        gSystem->ProcessEvents();
    }
}
/******************************************************************************/
main(int argc, char *argv[])
{
    char *p;

    signal (SIGPIPE, sigpipe);
    signal (SIGSEGV, SIG_DFL);

    if (readargs(argc, argv)) return 1;

    TApplication app("Jessica Spect", 0, 0);

    main_loop();
}
/******************************************************************************/
/******************************************************************************/
