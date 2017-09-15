/*
 * lowlevel/scsicompat/scsicompat.c
 * created 04.02.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsicompat.c,v 1.13 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <rcs_ids.h>
#include "scsicompat.h"
#include "../../main/nowstr.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

RCS_REGISTER(cvsid, "lowlevel/scsicompat")

typedef struct
  {
  char* name;
  int (*stat_proc)(struct raw_scsi_var*, char*, int*);
  } tape_types;

static tape_types ttypes[]=
  {
    {"DLT4000", status_DLT},
    {"DLT4500", status_DLT},
    {"DLT4700", status_DLT},
    {"DLT7000", status_DLT},
    {"DLT8000", status_DLT},
    {"EXB-8505", status_EXABYTE},
    {"EXB8500", status_EXABYTE},
    {"TKZ09", status_EXABYTE},
    {"EXB", status_EXABYTE},
    {"Ultrium 1-SCSI", status_EXABYTE},
    {0, status_UNKNOWN}
  };

/*****************************************************************************/
void scsi_dump_buffer(unsigned char* buf, int size)
{
int l, i;
int lines=size/16;
int rest=size%16;
for (l=0; l<lines; l++)
  {
  for (i=0; i<16; i++)
    {
    printf("%02X ", buf[l*16+i]);
    }
  printf("    ");
  for (i=0; i<16; i++)
    {
    unsigned int c=buf[l*16+i];
    if ((c>' ') && (c<127))
      printf("%c", c);
    else
      printf(".");
    }
  printf("\n");
  }
if (rest)
  {
  for (i=0; i<rest; i++)
    {
    printf("%02X ", buf[lines*16+i]);
    }
  for (i=rest; i<16; i++)
    {
    printf("   ");
    }
  printf("    ");
  for (i=0; i<rest; i++)
    {
    unsigned int c=buf[lines*16+i];
    if ((c>' ') && (c<127))
      printf("%c", c);
    else
      printf(".");
    }
  printf("\n");
  }
}
/*****************************************************************************/
raw_scsi_var* scsicompat_init(char* filename, int pid)
{
int res, i;
raw_scsi_var* var;

var=(raw_scsi_var*)malloc(sizeof(raw_scsi_var));
if (!var)
  {
  printf("%s scsicompat_init_%d: malloc raw_scsi_var: %s\n",
    nowstr(), pid, strerror(errno));
  return 0;
  }

if (scsicompat_init_(filename, var, pid)<0)
  {
  free(var);
  return 0;
  }
var->stat_proc=0;
var->policy.ignore_medium_changed=1;
if ((res=inquire_devicedata(var)))
  {
  scsicompat_done_(var);
  free(var);
  return 0;
  }
printf("scsicompat_init_%d: device: \"%s\"\n", pid,
    var->inquiry_data.devicename);
printf("scsicompat_init_%d: vendor: \"%s\"\n", pid,
    var->inquiry_data.vendorname);
printf("scsicompat_init_%d: revision: \"%s\"\n", pid,
    var->inquiry_data.revision);
if (var->inquiry_data.device_type!=1)
  {
  printf("scsicompat_init_%d: %s is not a tape (actual type=%d)\n",
      pid, filename, var->inquiry_data.device_type);
  scsicompat_done_(var);
  free(var);
  return 0;
  }

i=0;
while (ttypes[i].name &&
        strncmp(var->inquiry_data.devicename,
                ttypes[i].name, strlen(ttypes[i].name))) {
    i++;
}
var->stat_proc=ttypes[i].stat_proc;
if (ttypes[i].name)
  printf("type matches \"%s\".\n", ttypes[i].name);
else
  printf("type is unknown.\n");
if (set_position(var, "scsicompat_init")<0)
  {
  printf("scsicompat_init: can't read position\n");
  /*scsicompat_done_(var);
  free(var);
  return 0;*/
  }
return var;
}
/*****************************************************************************/
int scsicompat_done(raw_scsi_var* var)
{
scsicompat_done_(var);
free(var);
return 0;
}
/*****************************************************************************/
int inquire_devicedata(raw_scsi_var* device)
{
int len, i;
unsigned char *buf;
char *str;

if (scsi_inquiry(device, &buf, &len)<0)
  {
  printf("inquire_devicename: scsi_inquiry failed.\n");
  return -1;
  }
/* dump_buffer(buf, len); */

str=device->inquiry_data.vendorname;
bcopy(buf+8, device->inquiry_data.vendorname, 8);
str[8]=0; i=7;
while (i>=0 && str[i]==' ') {str[i]=0; i--;}

str=device->inquiry_data.devicename;
bcopy(buf+16, device->inquiry_data.devicename, 16);
str[16]=0; i=15;
while (i>=0 && str[i]==' ') {str[i]=0; i--;}

str=device->inquiry_data.revision;
bcopy(buf+32, device->inquiry_data.revision, 4);
str[4]=0; i=3;
while (i>=0 && str[i]==' ') {str[i]=0; i--;}

device->inquiry_data.device_type=buf[0]&0x1f;

return 0;
}
/*****************************************************************************/
int set_position(raw_scsi_var* var, char* caller)
{
scsi_position_data pos;

if (scsi_read_position(var, &pos)<0)
  {
  printf("%s (set_position): error in read position\n", caller);
  var->sanity.sw_position=-1;
  return -1;
  }
var->sanity.sw_position=pos.fbl;
return 0;
}
/*****************************************************************************/
int check_position(raw_scsi_var* var, char* caller)
{
scsi_position_data pos;

printf("check_position skipped; return always 0\n");
fflush(stdout);
return 1;

if (var->sanity.sw_position<0)
  {
  printf("%s: sw_position unknown; nothing written\n", caller);
  return -1;
  }
if (scsi_read_position(var, &pos)<0)
  {
  printf("%s: error in read position; nothing written\n", caller);
  return -1;
  }
if (pos.fbl!=var->sanity.sw_position)
  {
  printf("%s: hw_position=%d; sw_position=%d; nothing written\n", caller,
      pos.fbl, var->sanity.sw_position);
  return -1;
  }
return 0;
}
/*****************************************************************************/
/*****************************************************************************/
