/*
 * dataout/cluster/auftape.c
 * 
 * created 28.04.1997
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: auftape.c,v 1.13 2011/04/06 20:30:21 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sconf.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#define _main_c_
#include <debug.h>
#include "handlercom.h"
#include "../../main/nowstr.h"
#include <cluster.h>
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/scsicompat/scsicompat.h"

#define VERSION 2
#define P 3
hndlr_msg message;
raw_scsi_var* scsi_var;
int* clbuffer;
modresc ref;
int linked;
char* modulname;
volatile int xxx=0;
int pid;
char* filename;

RCS_REGISTER(cvsid, "")

/*****************************************************************************/
const char* nowstr(void)
{
    static char ts[50];
    struct timeval now;
    struct tm* tm;
    time_t tt;

    gettimeofday(&now, 0);
    tt=now.tv_sec;
    tm=localtime(&tt);
    strftime(ts, 50, NOWSTR, tm);
    return ts;
}
/*****************************************************************************/
static int msg_xread(int* buf, int size)
{
int res, da=0, csize=size*sizeof(int);
/* T(auftape::msg_xread) */
while (da<csize)
  {
  if ((res=read(P, (char*)buf+da, csize-da))>=0)
    da+=res;
  else if (errno!=EINTR)
    {
    printf("auftape_%d::msg_xread: %s\n", pid, strerror(errno));
    return -1;
    }
  }
return 0;
}
/*****************************************************************************/
static int msg_read(void)
{
/* T(auftape::msg_read) */
if (msg_xread((int*)&message, 4)<0)
  {
  fflush(stdout);
  return -1;
  }

if (message.size>100)
  {
  printf("auftape_%d::msg_read: size (%d) wrong\n", pid, message.size);
  fflush(stdout);
  return -1;
  }
if (msg_xread(message.data, message.size)<0)
  {
  fflush(stdout);
  return -1;
  }

fflush(stdout);
return 0;
}
/*****************************************************************************/
static int msg_write(void)
{
int res, size, dort=0;
/* T(auftape::msg_write) */
message.pid=pid;
/*
 * {
 * int i;
 * printf("msg_write: size=%d\n", message[0]+1);
 * for (i=0; i<message[0]+1; i++) printf("%02x ", message[i]);
 * printf("\n");
 * }
 */
size=(message.size+4)*sizeof(int);
while (dort<size)
  {
  if ((res=write(P, ((char*)&message)+dort, size-dort))>=0)
    dort+=res;
  else if (errno!=EINTR)
    {
    printf("auftape_%d::msg_write: %s\n", pid, strerror(errno));
    return -1;
    }
  }
return 0;
}
/*****************************************************************************/
static void send_none(void)
{
T(auftape::send_none)
message.size=0;
message.code.conf=hndlc_none;
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void send_version(int code)
{
T(auftape::send_version)
switch (code)
  {
  case 0:
    message.size=1;
    message.data[0]=VERSION;
    break;
  default:
    message.size=0;
  }
message.code.conf=hndlc_version;
msg_write();
fflush(stdout);
}
/*****************************************************************************/
/*
code 0: exit verweigert
code 1: auf Wunsch des Servers
code 2: wegen Fehlers
*/
static void send_exit(int code)
{
T(auftape::send_exit)
message.size=1;
message.code.conf=hndlc_exit;
message.data[0]=code;
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void send_status(void)
{
int res;
T(auftape::send_status)
/*
0: size
1: pid
2: errorcode (?)
3: fertig
4: disabled
   # exabyte #
5:    remaining tape
6:    data error counter
7:    filemrk/eom/ili/sense key/
      pf/bpe/fpe/me/eco/tme/tnp/lbot/
      xfr/tmd/wp/fmke/ure/we1/sse/fe/
      wseb/wseo
8: add. sense code/ add. sense code qualifier
   # dlt4000 (7000?) #
5: filemrk/eom/ili/sense key
6: add sense code
7: add sense code qualifier
8: internal status code
9: tape motion hours
10: power on hours
11: write compression ratio
12: MBytes transferred from host
13: Bytes transferred from host
14: MBytes written to tape
15: Bytes written to tape
*/

res=scsi_var->stat_proc(scsi_var, scsi_var->inquiry_data.devicename,
    message.data+1);
if (res<0)
  {
  printf("auftape_%d::status(): error in getting status\n", pid);
  message.size=1;
  message.code.conf=hndlc_error;
  message.data[0]=hndle_status;
  }
else
  {
  message.size=res+1;
  message.code.conf=hndlc_status;
  message.data[0]=0; /* errorcode */
  }
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static int force_density(int density)
{
T(auftape::force_density)
printf("auftape_%d: force_density(%d)\n", pid, density);
fflush(stdout);
if (density==-1) return 0;
return scsi_mode_select(scsi_var, density);
}
/*****************************************************************************/
static int check_at_eod(void)
{
scsi_position_data pos;
T(auftape::check_at_eod)

printf("check_at_eod skipped; return always 1\n");
fflush(stdout);
return 1;

if (scsi_var->sanity.records==0)
  {
  printf("auftape_%d: check_at_eod: no records written.\n", pid);
  fflush(stdout);
  return 0;
  }
if (scsi_var->sanity.filemarks!=0)
  {
  printf("auftape_%d: check_at_eod: %d filemarks written before.\n", pid,
      scsi_var->sanity.filemarks);
  }
if (scsi_var->sanity.sw_position<0)
  {
  printf("auftape_%d: check_at_eod: sw_position not valid\n", pid);
  }
printf("check_at_eod: call scsi_read_position()\n");
if (scsi_read_position(scsi_var, &pos)<0)
  {
  printf("auftape_%d: check_at_eod: scsi_read_position failed.\n", pid);
  fflush(stdout);
  return 0;
  }
printf("check_at_eod: scsi_read_position() called\n");
if (pos.fbl!=scsi_var->sanity.sw_position)
  {
  printf("auftape_%d: check_at_eod: position %d --> %d\n", pid,
      scsi_var->sanity.sw_position, pos.fbl);
  /* XXX any mismatch should be considdered fatal */
  if (pos.fbl<scsi_var->sanity.sw_position)
    {
    fflush(stdout);
    return 0;
    }
  }
printf("check_at_eod: return 1\n");
fflush(stdout);
return 1;
}
/*****************************************************************************/
static void scsi_control(void)
{
int code=message.data[0];
int res=0, len;
ems_u32* help=(ems_u32*)message.data;
ems_u32* out=(ems_u32*)message.data;
unsigned char* buf=0;
scsi_position_data position_data;
T(auftape::scsi_control)

switch (code)
  {
  case hndlcode_none:
    break;
  case hndlcode_Rewind:
    if (message.size!=2)
      {
      printf("scsi_control:Rewind: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_rewind(scsi_var, message.data[1]/*immediate*/);
    break;
  case hndlcode_Write:
    printf("%s auftape: scsi_control(hndlcode_Write) called\n", nowstr());
    if (message.size<2)
      {
      printf("scsi_control:Write: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      {
      res=scsi_write(scsi_var, message.data[1]/*num*/,
          message.data+2/*data*/);
      if (res!=0) printf("aftape: scsi_write returns %d\n", res);
      }
    break;
  case hndlcode_WriteFileMarks:
    if (message.size!=4)
      {
      printf("scsi_control:Filemark: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      {
      if (message.data[3] || check_at_eod())
        {
        res=scsi_write_filemark(scsi_var, message.data[1]/*num*/,
          message.data[2]/*immediate*/);
        }
      }
    break;
  case hndlcode_Inquiry:
    printf("auftape: hndlcode_Inquiry\n");
    res=scsi_inquiry(scsi_var, &buf, &len);
    printf("auftape: hndlcode_Inquiry; res=%d\n", res);
    break;
  case hndlcode_Load:
    if (message.size!=3)
      {
      printf("scsi_control:Load: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_load_unload(scsi_var, message.data[1]/*load*/,
          message.data[2]/*immediate*/);
      printf("scsi_load_unload returns %d\n", res);
    break;
  case hndlcode_Prevent_Allow:
    if (message.size!=2)
      {
      printf("scsi_control:Prevent: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_prevent_allow(scsi_var, message.data[1]/*prevent*/);
      printf("scsi_prevent_allow returns %d\n", res);
    break;
  case hndlcode_Erase:
    /* XXX dataout does not know anything about extend */
    if (message.size!=3)
      {
      printf("scsi_control:Erase: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_erase(scsi_var, message.data[1]/*immediate*/, message.data[2]/*extend*/);
    break;
  case hndlcode_Locate:
    if (message.size!=3)
      {
      printf("scsi_control:Erase: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_locate(scsi_var, message.data[1]/*position*/,
          message.data[2]/*immediate*/);
    break;
  case hndlcode_Space:
    if (message.size!=3)
      {
      printf("scsi_control:Space: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
      res=scsi_space(scsi_var, message.data[1]/*code*/,
          message.data[2]/*count*/);
    break;
  case hndlcode_ReadPos:
    res=scsi_read_position(scsi_var, &position_data);
    break;
  case hndlcode_LogSelect:
    res=scsi_log_select(scsi_var);
    break;
  case hndlcode_ModeSelect:
    if (message.size!=2)
      {
      printf("scsi_control:ModeSelect: wrong # of args (%d)\n", message.size-1);
      res=-1;
      }
    else
    res=scsi_mode_select(scsi_var, message.data[1]/*density*/);
    break;
  default: printf("hndlcode (A): %d; not decoded\n", code);
  }
if (res<0)
  {
  int i;
  for (i=0; i<sizeof(scsi_error_info)/4; i++)
      *out++=((int*)&(scsi_var->scsi_error))[i];
  }
else
  {
  *out++=0;
  switch (code)
    {
    case hndlcode_none: break;
    case hndlcode_Rewind: break;
    case hndlcode_Erase: break;
    case hndlcode_Write: break;
    case hndlcode_WriteFileMarks: break;
    case hndlcode_Space: break;
    case hndlcode_Inquiry:
      {
      *out++=2;
      out=outnstring(out, (char*)(buf+16), 16);
      out=outnstring(out, (char*)(buf+8), 8);
      }
      break;
    case hndlcode_ModeSelect: break;
    case hndlcode_Load: break;
    case hndlcode_Locate: break;
    case hndlcode_LogSelect: break;
    case hndlcode_LogSense: break;
    case hndlcode_ModeSense: break;
    case hndlcode_Prevent_Allow: break;
    case hndlcode_ReadPos:
      {
      *out++=position_data.fbl;
      *out++=position_data.lbl;
      *out++=position_data.bop;
      *out++=position_data.eop;
      *out++=position_data.blib;
      *out++=position_data.byib;
      }
      break;
    default: printf("hndlcode (B): %d; not decoded\n", code);
    }
  }

message.size=out-help;
message.code.conf=hndlc_control;
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static int link_memory(int flag)
{
T(auftape::link_memory)
if (flag) /* link */
  {
  printf("auftape_%d: try link_datamod(%s, %p)\n", pid, modulname, &ref);
  ref.len=0;
  clbuffer=link_datamod(modulname, &ref);
  if ((clbuffer==0) || (ref.len==0))
    {
    printf("auftape_%d: ging schief\n", pid);
    if (clbuffer!=0) errno=-1;
    message.size=2;
    message.code.conf=hndlc_error;
    message.data[0]=hndle_linkdata;
    message.data[1]=errno;
    msg_write();
    }
  else
    {
    printf("auftape_%d: ging gut: clbuffer=%p, len=%d Bytes\n", pid, clbuffer,
        ref.len);
    message.size=1;
    message.code.conf=hndlc_linkdata;
    message.data[0]=1;
    msg_write();
    linked=1;
    {
    /* shared memory must be really used before using it in write(...) */
    /* otherwise BSD will crash */
    int i; int l=ref.len/sizeof(int); for (i=0; i<l; i++) xxx+=clbuffer[i];
    }
    }
  }
else /* unlink */
  {
  printf("auftape_%d: try unlink_datamod(%s, %p)\n", pid, modulname, &ref);
  if (linked==0)
    {
    printf("auftape_%d: not linked\n", pid);
    message.size=2;
    message.code.conf=hndlc_error;
    message.data[0]=hndle_linkdata;
    message.data[1]=0;
    }
  else
    {
    unlink_datamod(&ref);
    message.size=1;
    message.code.conf=hndlc_linkdata;
    message.data[0]=0;
    msg_write();
    linked=0;
    }
  }
fflush(stdout);
return 0;
}
/*****************************************************************************/
static void write_data(int offset)
{
struct Cluster* cluster=(struct Cluster*)(clbuffer+offset);
int res;
T(auftape::write_data)

if (!linked)
  {
  message.size=1;
  message.code.conf=hndlc_error;
  message.data[0]=hndle_protocol;
  printf("auftape_%d: write data without datamodule.\n", pid);
  fflush(stdout);
  return;
  }
/*#if 0*/
switch (cluster->type)
  {
  case clusterty_events:
    /*printf("auftape_%d: cluster %s\n", "events");*/
    break;
  case clusterty_ved_info:
    printf("%s auftape_%d: cluster %s\n", nowstr(), pid, "ved_info");
    break;
  case clusterty_text:
    printf("%s auftape_%d: cluster %s\n", nowstr(), pid, "text");
    break;
  case clusterty_file:
    printf("%s auftape_%d: cluster %s\n", nowstr(), pid, "file");
    break;
  case clusterty_no_more_data:
    printf("%s auftape_%d: cluster %s\n", nowstr(), pid, "no_more_data");
    break;
  default:
    printf("%s auftape_%d: cluster %d\n", nowstr(), pid, ClTYPE(cluster->data));
    break;
  }
/*#endif*/
res=scsi_write(scsi_var, cluster->size, (int*)(cluster->data));
if (res==0)
  {
  message.size=2;
  message.code.conf=hndlc_write;
  message.data[0]=offset;
  message.data[1]=scsi_var->scsi_error.eom;
  }
else
  {
  printf("auftape: write_data: res=%d\n", res);
  if ((scsi_var->scsi_error.sense_key==0) &&
      (scsi_var->scsi_error.add_sense_code==0) &&
      (scsi_var->scsi_error.add_sense_code_qual==2) &&
      (scsi_var->scsi_error.eom==1))
    { /* early end of tape warning */
    printf("%s auftape: early end of tape warning\n", nowstr());
    message.size=2;
    message.code.conf=hndlc_write;
    message.data[0]=offset;
    message.data[1]=1;
    }
  else
    {
    printf("%s auftape: real error, size=%d (%d bytes)\n", nowstr(),
            cluster->size, cluster->size*4);
    message.size=6;
    message.code.conf=hndlc_error;
    message.data[0]=hndle_write;
    message.data[1]=scsi_var->scsi_error.error_code;
    message.data[2]=scsi_var->scsi_error.sense_key;
    message.data[3]=scsi_var->scsi_error.add_sense_code;
    message.data[4]=scsi_var->scsi_error.add_sense_code_qual;
    message.data[5]=scsi_var->scsi_error.eom;
    }
  }
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void write_filemark(int num)
{
T(auftape::write_filemark)
if (check_at_eod())
  {
  message.size=1;
  message.code.conf=hndlc_filemark;
  message.data[0]=scsi_write_filemark(scsi_var, num, 0/*not immediate*/);
  printf("%s auftape_%d: wrote %d filemark%s\n",
    nowstr(), pid, num, num==1?"":"s");
  msg_write();
  }
else
  {
  message.size=1;
  message.code.conf=hndlc_filemark;
  message.data[0]=1;
  printf("%s auftape_%d: wrote NO filemarks\n", nowstr(), pid);
  msg_write();
  }
fflush(stdout);
}
/*****************************************************************************/
static void t_wind(int offs)
{
T(auftape::t_wind)
message.size=1;
message.code.conf=hndlc_wind;
if (offs>0)
  printf("auftape_%d: moving %d file%s forward\n", pid, offs, offs==1?"":"s");
else if (offs<0)
  printf("auftape_%d: moving %d file%s backward\n", pid, -offs, offs==-1?"":"s");
else
  printf("auftape_%d: does not move\n", pid);
message.data[0]=scsi_space(scsi_var, 1/*filemarks*/, offs);
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void t_rewind(void)
{
T(auftape::t_rewind)
message.size=1;
message.code.conf=hndlc_rewind;
message.data[0]=scsi_rewind(scsi_var, 0/*not immediate*/);
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void t_seod(void)
{
T(auftape::t_seod)
message.size=1;
message.code.conf=hndlc_seod;
message.data[0]=scsi_space(scsi_var, 3/*end of data*/, 0/*count*/);
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void t_unload(void)
{
T(auftape::t_unload)
message.size=1;
message.code.conf=hndlc_unload;
message.data[0]=scsi_load_unload(scsi_var, 0/*not load*/, 0/*not immediate*/);
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void set_debug(int d)
{
#ifdef DEBUG
printf("auftape_%d: debug set to 0x%08x\n", pid, d);
message.data[0]=debug;
debug=d;
#else
printf("auftape_%d: debug not compiled in\n", pid);
message.data[0]=0;
#endif
message.size=1;
message.code.conf=hndlc_debug;
msg_write();
fflush(stdout);
}
/*****************************************************************************/
static void msg_write_fatal(void)
{
T(auftape::msg_write_fatal)
msg_write();
fflush(stdout);
sleep(2);
exit(0);
}
/*****************************************************************************/
int main(int argc, char* argv[])
{
pid=getpid();
filename=argv[2];
printf("%s auftape_%d started\n", nowstr(), pid);
if (argc!=5)
  {
  int i;
  printf("usage: %s buffername filename density version\n", argv[0]);
  printf("got args:");
  for (i=0; i<argc; i++) printf(" >%s<", argv[i]);
  printf("\n");
  message.size=1;
  message.code.conf=hndlc_error;
  message.data[0]=hndle_args;
  msg_write_fatal();
  }
{
int version;
version=atoi(argv[4]);
if (version!=VERSION)
  {
  printf("auftape_%d: Requested Version: %d; my own Version: %d\n",
      pid, version, VERSION);
  message.size=2;
  message.code.conf=hndlc_error;
  message.data[0]=hndle_version;
  message.data[1]=VERSION;
  msg_write_fatal();
  }
}
printf("auftape_%d: buffer=%s; file=%s\n", pid, argv[1], filename);
{
message.size=2;
message.code.conf=hndlc_pid;
message.data[0]=getpid();
message.data[1]=VERSION;
msg_write();
}

if ((scsi_var=scsicompat_init(filename, pid))==0)
  {
  printf("auftape_%d: scsicompat_init sclug fehl\n", pid);
  message.size=2;
  message.code.conf=hndlc_error;
  message.data[0]=hndle_init_scsi;
  message.data[1]=errno;
  msg_write_fatal();
  }
{
int density;
density=atoi(argv[3]);
force_density(density);
}
/* do_tape_get(path); */
modulname=argv[1];
linked=0;

while (1)
  {
  if (msg_read()<0)
    {
    send_exit(2);
    printf("auftape_%d: error_exit\n", pid);
    goto raus;
    }
  if (message.pid!=pid)
    {
    printf("\007\007auftape_%d: message for pid %d\n", pid, message.pid);
    }
  else
    {
    switch (message.code.conf)
      {
      case hndlr_write: write_data(message.data[0]); break;
      case hndlr_none: send_none(); break;
      case hndlr_exit:
        printf("%s auftape_%d: exit\n", nowstr(), pid);
        send_exit(1);
        goto raus;
      case hndlr_status: send_status(); break;
      case hndlr_linkdata: link_memory(message.data[0]); break;
      case hndlr_filemark:
        if (message.size!=1)
          {
          printf("auftape_%d: wrong # of arguments for hndlr_filemark (%d)\n",
              pid, message.size);
          }
        write_filemark(message.data[0]);
        break;
      case hndlr_wind: t_wind(message.data[0]); break;
      case hndlr_rewind: t_rewind(); break;
      case hndlr_seod: t_seod(); break;
      case hndlr_unload: t_unload(); break;
      case hndlr_control: scsi_control(); break;
      case hndlr_debug: set_debug(message.data[0]); break;
      case hndlr_version: send_version(message.data[0]); break;
      default: printf("auftape_%d: unknown message %d\n", pid, message.code.conf);
      }
    }
  }
raus:
printf("%s auftape_%d: raus!!\n", nowstr(), pid);
scsicompat_done(scsi_var);
unlink_datamod(&ref);
fflush(stdout);
sleep(1);
return 0;
}
/*****************************************************************************/
/*****************************************************************************/
