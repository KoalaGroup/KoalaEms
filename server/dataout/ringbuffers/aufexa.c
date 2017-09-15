/******************************************************************************
*                                                                             *
* aufexa.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created before 08.07.94                                                     *
* last changed 11.05.95                                                       *
*                                                                             *
******************************************************************************/

#define READOUT_PRIOR 1000
#define MAX_BLCK 0x3f00 /* in Integern */
#define HANDLERCOMNAME "handlercom"

#include <stdio.h>
#include <module.h>
#include <errno.h>
#include <types.h>
#include <modes.h>

#include "handlers.h"

static int exa,evid;
static mod_exec *head;

static int outbuf[MAX_BLCK];
static int cursize,*curpnt;
static int fertig,disabled,tapeerror,tapefull,*buffer;
static VOLATILE int *ip;

static int ppid,sig;

static int debug;

typedef struct errdet_str {
    unsigned        ed_valid  : 1,    /* error is valid */
    ed_class        : 3,    /* error class = 7 */
    ed_zero         : 4;    /* always 0 */
    u_char          ed_seg;                /* segment number always 0 */
    unsigned        ed_filemrk : 1,    /* filemark */
    ed_eom          : 1,    /* end of medium */
    ed_ili          : 1,    /* incorrect length indicator */
    ed_zero2        : 1,    /* always zero */
    ed_main         : 4;    /* main sense key */
    u_char          ed_info[4];         /* info byte [lba] */
    u_char          ed_senslen;         /* additional sense length */
    u_char          ed_res1[4];         /* reserved */
    u_char          ed_addscode;        /* add sense code */
    u_char          ed_addscdqual;      /* add sense code qualifier*/
    u_char          ed_res2[2];         /* reserved */
    u_char          ed_rwdec[3];        /* data error counter */
    unsigned        ed_pf        :1,    /* power fail */
    ed_bpe          :1,     /* bus PE */
    ed_fpe          :1,     /* buffer PE */
    ed_me           :1,     /* media error */
    ed_eco          :1,     /* error counter overflow */
    ed_tme          :1,     /* tape motion error */
    ed_tnp          :1,     /* tape not present */
    ed_bot          :1,     /* BOT */
    ed_xfr          :1,     /* transfer abort error */
    ed_tmd          :1,     /* tape mark detect error */
    ed_wp           :1,     /* write protect */
    ed_fmke         :1,     /* filemark error */
    ed_ure          :1,     /* underrun error */
    ed_we1          :1,     /* write error 1 */
    ed_sse          :1,     /* servo system error */
    ed_fe           :1,     /* formatter error */
    ed_res3         :6,     /* reserved */
    ed_wseb         :1,     /* write splice error */
    ed_wseo         :1;     /* write splice error */
    u_char          ed_res4;            /* reserved */
    u_char          ed_remtp[3];        /* remaining tape */
} errdet;

#define SC_REQSENSE 0x03
#define SC_WRITE 0x0a
#define SC_WRITEMARK 0x10
#define SC_SPACE 0x11
#define SC_MODESELECT 0x15
#define SC_MODESENSE 0x1a
#define INPUT 0
#define OUTPUT 1

#ifdef FORCEDENS
typedef struct modesel_str {
    u_char              ms_res1;        /* sense data length (sense) */
    u_char              ms_res2;        /* medium type ( sense) */
    unsigned            ms_res3 : 1,    /* write protect (sense) */
    ms_buffmode         : 3,    /* buffer mode */
    ms_speed            : 4;    /* speed = 0 */
    u_char              ms_bdlen;       /* block descriptor length: (8) */
    u_char              ms_denscode;    /* density code */
    u_char              ms_numblks[3];  /* number of blocks: (0) */
    u_char              ms_res4;        /* reserved: (0) */
    u_char              ms_blklen[3];   /* block length */
    unsigned            ms_ct   :1,     /* cartridge type */
    ms_res5             :1,     /* reserved */
    ms_nd               :1,     /* no disconnect */
    ms_res6             :1,     /* reserved */
    ms_nbe              :1,     /* no busy enable */
    ms_ebd              :1,     /* even byte disconnect */
    ms_pe               :1,     /* parity enable */
    ms_nal              :1,     /* no autoload */
    ms_res7             :7,     /* reserved */
    ms_p5               :1;     /* cartridge type */
    u_char              ms_mottr;       /* motion threshold */
    u_char              ms_rectr;       /* reconnect threshold */
    u_char              ms_gaptr;       /* gap threshold */

} modesel;

static int forcedens(code)
int code;
{
    int res;
    modesel mode_sel;
    res=scsi_xfer(exa,SC_MODESENSE,0,sizeof(modesel),0,&mode_sel,
                        sizeof(modesel),0,INPUT,0);
    if(res)return(E_BMODE);
    mode_sel.ms_res1=mode_sel.ms_res2=mode_sel.ms_res3=0;
    mode_sel.ms_buffmode=1;
    mode_sel.ms_speed=0;
    mode_sel.ms_bdlen=8;
    mode_sel.ms_denscode=code;
    mode_sel.ms_numblks[0]=mode_sel.ms_numblks[1]=mode_sel.ms_numblks[2]=0;
    mode_sel.ms_res4=0;
    mode_sel.ms_blklen[0]=mode_sel.ms_blklen[1]=mode_sel.ms_blklen[2]=0;
    res=scsi_xfer(exa,SC_MODESELECT,0,12,0,&mode_sel,
                  12,0,OUTPUT,0);
    if(res)return(E_BMODE);
    return(0);
}
#endif

/*****************************************************************************/

void seterror(int error, char* text)
{
if (debug) printf("seterror: %d --> %d (%s)\n", tapeerror, error, text);
tapeerror=error;
}

/*****************************************************************************/

int flushbuffer()
{
int res;

if ((!(tapeerror||tapefull||disabled))&&(cursize))
  {
  if (write(exa,outbuf,cursize*sizeof(int))==-1)
    {
    seterror(errno, "flushbuffer");
    res=-1;
    if ((tapeerror==E_SEEK)||(tapeerror==E_FULL))
      {
      tapefull=1;
      seterror(0, "flushbuffer, band voll");
      res=0;
      }
    }
  }
else
  res=0;
cursize=0;
curpnt=outbuf;
return(res);
}

/*****************************************************************************/

void gib_weiter(buf,size)
int *buf,size;
{
if ((cursize+size)>MAX_BLCK)
  {
  if (!disabled)
    {
    if (write(exa,outbuf,cursize*sizeof(int))==-1)
      {
      seterror(errno, "gib_weiter");
      if ((tapeerror==E_SEEK)||(tapeerror==E_FULL))
        {
        tapefull=1;
        seterror(0, "gib_weiter, band voll");
        }
      fertig=3;
      kill(ppid,sig);
      cursize=0;
      curpnt=outbuf;
      return;
      }
    }
  cursize=0;
  curpnt=outbuf;
  }
memcpy(curpnt,buf,size*sizeof(int));
cursize+=size;
curpnt+=size;
}

/*****************************************************************************/

cleanup()
{
if (evid!=-1) _ev_unlink(evid);
if (head!=(mod_exec*)-1) munlink(head);
if (exa!=-1) close(exa);
exit(0);
}

/*****************************************************************************/

void liesringbuffer()
{
    register int len;
    if(!(*ip)){
#ifndef NOSMARTPOLL
        tsleep(1);
#endif
        return;
    }
    len= *(ip+1);
    if(len>-1){
        gib_weiter(ip+1,len+1);
        *ip=0;
        ip+=len+2;
    }else{
        *ip=0;
        if(len==-2)fertig=2;
        else ip=buffer;
    }
}

/*****************************************************************************/

void testenv()
{
char* val;

debug=0;
val=(char*)getenv("EMS_HDEBUG");
if (val)
  {
  if (sscanf(val, "%d", &debug)!=1)
    {
    printf("aufexa (EMS_HDEBUG): cannot convert %s to int\n");
    debug=1;
    }
  }
if (debug) printf("aufexa: debug=%d\n", debug);
}

/*****************************************************************************/

main(argc, argv)
int argc; char **argv;
{
char *handlercomname;
int was;

if (argc<4)
  {
  printf("%s: argc==%d; argc>=4 expected\n", argv[0], argc);
  exit(0);
  }

testenv();

seterror(0, "start main");
tapefull=0;
disabled=0;

exa=open(argv[1],3);
if (exa==-1) seterror(errno, "open failed");

if(!tapeerror){
    int blsz;
    _ss_mdsel(exa,0);
    if(_ss_blsz(exa,&blsz)) seterror(errno, "var. blocksize");
}
#ifdef FORCEDENS
if(!tapeerror) seterror(forcedens(FORCEDENS), "forcedens");
#endif

head=(mod_exec*)modlink(argv[2],mktypelang(MT_DATA,ML_ANY));
if (head==(mod_exec*)-1) seterror(errno, "link data module");
buffer=(int*)((char*)head+head->_mexec);

evid=_ev_link(argv[2]);
if(evid==-1)exit(errno);

if (sscanf(argv[3],"%d",&sig)!=1) seterror(E_ILLARG, "command line");
ppid=getppid();

if(argc>4)handlercomname=argv[4];
else handlercomname=HANDLERCOMNAME;

intercept(cleanup);
fertig=1;
cursize=0;
curpnt=outbuf;
for (;;)
  {
  if (fertig)
    {
    was=_ev_wait(evid,HANDLER_MINVAL,HANDLER_MAXVAL);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  else
    {
    int i;
    for (i=0; (i<READOUT_PRIOR) && !fertig; i++) liesringbuffer();
    was=_ev_read(evid);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  switch (was)
    {
    case HANDLER_START:
      if (debug) printf("HANDLER_START\n");
      if ((tapeerror==E_NOTRDY)||(tapeerror==E_DIDC)) seterror(0, "START");
      if (!(tapeerror||tapefull))
        {
        fertig=0;
        ip=buffer;
        }
      break;
    case HANDLER_QUIT:
      if (debug) printf("HANDLER_QUIT\n");
      while (!(tapeerror||fertig)) liesringbuffer();
      if (!(tapeerror||tapefull)) flushbuffer();
      cleanup();
      break;
    case HANDLER_GETSTAT:
      if (debug) printf("HANDLER_GETSTAT\n");
      {
      mod_exec *hmod;
      int *buf;
      hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
      if (hmod==(mod_exec*)-1) exit(errno);
      buf=(int*)((char*)hmod+hmod->_mexec);
      buf[3]=fertig;
      buf[4]=disabled;
      /*
      if (tapeerror)
        {
        buf[2]=tapeerror;
        buf[1]=3;
        }
      else
      */
        {
        errdet err_det;
        seterror(scsi_xfer(exa, SC_REQSENSE, 0, sizeof(err_det), 0,
            &err_det, sizeof(err_det), 0, INPUT, 0), "request sense");
        buf[5]=(err_det.ed_remtp[0]<<16)+(err_det.ed_remtp[1]<<8)+
            err_det.ed_remtp[2];
        buf[6]=(err_det.ed_rwdec[0]<<16)+(err_det.ed_rwdec[1]<<8)+
            err_det.ed_rwdec[2];
        buf[7]=((&err_det.ed_seg)[1]<<24)+(err_det.ed_rwdec[3]<<16)+
            (err_det.ed_rwdec[4]<<8)+
        err_det.ed_rwdec[5];
        buf[8]=(err_det.ed_addscode<<8)+err_det.ed_addscdqual;
        buf[2]=(tapeerror?tapeerror:(tapefull?E_FULL:0));
        buf[1]=7;
        }
      buf[0]=0;
      munlink(hmod);
      }
      break;
    case HANDLER_ENABLE:
      if (debug) printf("HANDLER_ENABLE\n");
      if (!tapeerror) disabled=0;
      break;
    case HANDLER_DISABLE:
      if (debug) printf("HANDLER_DISABLE\n");
      disabled=1;
      if (fertig==3) fertig=0;
      break;
    case HANDLER_WRITE:
      if (debug) printf("HANDLER_WRITE\n");
      {
      mod_exec *hmod;
      int *buf;
      hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
      if(hmod==(mod_exec*)-1){
        seterror(errno, "link communication module");
        break;
      }
      buf=(int*)((char*)hmod+hmod->_mexec);
      flushbuffer();
      if (!tapeerror)
        {
        /*write(exa,buf+2,buf[1]*sizeof(int));*/
        scsi_xfer(exa, SC_WRITE, 0, buf[1]*sizeof(int), 0, buf+2,
            buf[1]*sizeof(int), 0, OUTPUT, 0);
        /* geht auch, wenn tape full (early warning) */
        }
      buf[0]=0;
      munlink(hmod);
      }
      break;
    case -MAX_WIND:
      if (debug) printf("-MAX_WIND\n");
      {
      int savederror=0;
      if (flushbuffer()!=0) savederror=tapeerror;
      if((!tapeerror)||(tapeerror==E_NOTRDY)||(tapeerror==E_DIDC))
        {
        seterror(_ss_rwd(exa),"-MAX_WIND; rewind");
        tapefull=0;
        }
      if (savederror) seterror(savederror, "-MAX_WIND; rewunden");
      }
      break;
    case MAX_WIND:
      if (debug) printf("+MAX_WIND\n");
      flushbuffer();
      if (!tapeerror)
        {
        scsi_xfer(exa,SC_SPACE,3,0,0,0,0,0,0,0);
        _ss_rfm(exa,-1);
        }
      break;
    case 0:
      if (debug) printf("WRITEMARK\n");
      flushbuffer();
      if (!tapeerror)
        {
        /*_ss_wfm(exa,2);*/
        scsi_xfer(exa,SC_WRITEMARK,0,2,0,0,0,0,0,0);
        _ss_rfm(exa,-1);
        }
      break;
    default:
      if ((was>-MAX_WIND)&&(was<MAX_WIND))
        {
        if (debug) printf("WIND %d\n", was);
        flushbuffer();
        if (!tapeerror)_ss_rfm(exa,was);
        }
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
