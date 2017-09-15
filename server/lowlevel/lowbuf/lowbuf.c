/*
 * lowlevel/lowbuf/lowbuf.c
 * created      16.05.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lowbuf.c,v 1.6 2011/04/06 20:30:25 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sconf.h>
#include <debug.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "../oscompat/oscompat.h"
#include "lowbuf.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

static char* buffername;
static int* buffer;
static int buffersize;
static modresc modref;

static int* dataout_buffer;
static int  dataout_size;
static int* out_buffer;
static int  out_size;
#ifdef LOWBUFFER_EXTRA
static int* extra_buffer;
static int  extra_size;
#endif

typedef enum {meth_shm, meth_malloc, meth_mmap} methods;

static methods method;
/*#define int_size(x) x=(x+sizeof(int)-1)/sizeof(int)*/

RCS_REGISTER(cvsid, "lowlevel/lowbuf")

/*****************************************************************************/
char* lowbuffername()          {return buffername;}
int* lowbuf_buffer()           {return buffer;}
int lowbuf_buffersize()        {return buffersize;}

int* lowbuf_outbuffer()        {return out_buffer;}
int lowbuf_outbuffersize()     {return out_size;}

int* lowbuf_dataoutbuffer()    {return dataout_buffer;}
int lowbuf_dataoutbuffersize() {return dataout_size;}

#ifdef LOWBUFFER_EXTRA
int* lowbuf_extrabuffer()      {return extra_buffer;}
int lowbuf_extrabuffersize()   {return extra_size;}
#endif
/*****************************************************************************/
int lowbuf_low_printuse(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:lowbuf=malloc|mmap|shm][:lowbufdata#size][:lowbufout#size]\n"
#ifdef LOWBUFFER_EXTRA
"  [:lowbufextra#size]\n"
#endif
"    defaults: lowbuf=%s lowbufdata#%d lowbufout#%d"
#ifdef LOWBUFFER_EXTRA
" lowbufextra#%d"
#endif
"\n",
LOWBUFFER_TYPE, LOWBUFFER_DATAOUT, LOWBUFFER_OUT
#ifdef LOWBUFFER_EXTRA
, LOWBUFFER_EXTRA
#endif
);
return 1;
}
/*****************************************************************************/
errcode lowbuffer_init(char* arg)
{
char* defmethodstr=LOWBUFFER_TYPE;
char* methodstr=0;
long argval;

T(lowbuffer_init)
dataout_size=LOWBUFFER_DATAOUT;
out_size=LOWBUFFER_OUT;
#ifdef LOWBUFFER_EXTRA
extra_size=LOWBUFFER_EXTRA;
#endif
if (arg!=0)
  {
  int warn=0;
  cgetstr(arg, "lowbuf", &methodstr);
  if (cgetnum(arg, "lowbufdata", &argval)==0)
    {
    dataout_size=argval;
    warn=1;
    }
  if (cgetnum(arg, "lowbufout", &argval)==0)
    {
    out_size=argval;
    warn=1;
    }
#ifdef LOWBUFFER_EXTRA
  if (cgetnum(arg, "lowbufextra", &argval)==0)
    {
    extra_size=argval;
    warn=1;
    }
#endif
  if (warn)
    {
    printf("WARNING!\nChanging the size of the buffers used for replies "
        "(lowbufout) and dataout (lowbufdata) is VERY experimental und will "
        "probably lead to some confusion inside the server.\n");
    }
  }

if (methodstr==0) methodstr=defmethodstr;
if (strcmp(methodstr, "mmap")==0)
  method=meth_mmap;
else if (strcmp(methodstr, "shm")==0)
  method=meth_shm;
else if (strcmp(methodstr, "malloc")==0)
  method=meth_malloc;
else
  {
  printf("%s not known; only mmap, shm and malloc\n", methodstr);
  if (methodstr!=defmethodstr) free(methodstr);
  return(Err_ArgRange);
  }
if (methodstr!=defmethodstr) free(methodstr);

buffername=malloc(strlen(LOWBUFFER_NAME)+12);
snprintf(buffername, strlen(LOWBUFFER_NAME)+12, "%s_%05d", LOWBUFFER_NAME, getpid());
/*int_size(dataout_size);*/
/*int_size(out_size);*/
buffersize=dataout_size+out_size;
/* printf("dataout_size=%d, out_size=%d", dataout_size, out_size); */
#ifdef LOWBUFFER_EXTRA
/*int_size(extra_size);*/
buffersize+=extra_size;
/* printf(", extra_size=%d", extra_size); */
#endif
/* printf("\nbuffersize=%d\n", buffersize); */

switch (method)
  {
  case meth_malloc:
    /* printf("malloc %d ints\n", buffersize); */
    /* der Speicher MUSS benutzt worden sein, deshalb kein malloc */
    buffer=(int*)calloc(buffersize, sizeof(int));
    if (buffer==0)
      {
      printf("lowbuffer_init: init_datamod(...): %s\n", strerror(errno));
      return Err_System;
      }
    break;
  case meth_shm:
    printf("shm %d ints\n", buffersize);
    printf("ACHTUNG, Speicher muss vor backmap benutzt werden!\n");
    buffer=init_datamod(buffername, buffersize*sizeof(int), &modref);
    if (buffer==0)
      {
      printf("lowbuffer_init: init_datamod(...): %s\n", strerror(errno));
      return Err_System;
      }
    break;
  case meth_mmap:
    printf("lowbuffer_init: mmap not yet implemented\n");
    return Err_NotImpl;
    break;
  default:
    printf("fatal program error in lowbuffer_init: method=%d\n", (int)method);
    return Err_Program;
  }

out_buffer=buffer;
dataout_buffer=buffer+out_size;
#ifdef LOWBUFFER_EXTRA
extra_buffer=buffer+dataout_size+out_size;
#endif

/*
 * printf("lowbuffer_init:\n");
 * printf("  out_buffer    : first=%p, last=%p, size=%d\n",
 *     out_buffer, out_buffer+out_size-1, out_size);
 * printf("  dataout_buffer: first=%p, last=%p, size=%d\n",
 *     dataout_buffer, dataout_buffer+dataout_size-1, dataout_size);
 * printf("  extra_buffer  : first=%p, last=%p, size=%d\n",
 *     extra_buffer, extra_buffer+extra_size-1, extra_size);
 */
return OK;
}
/*****************************************************************************/
errcode lowbuffer_done()
{
switch (method)
  {
  case meth_malloc:
    if (buffer) free(buffer);
    break;
  case meth_shm:
    if (buffer) done_datamod(&modref);
    break;
  case meth_mmap:
    break;
  default:
    printf("fatal program error in lowbuffer_done: method=%d\n", (int)method);
    return Err_Program;
  }
buffer=0;
free(buffername);
return OK;
}
/*****************************************************************************/
errcode lowbuf_low_init(char* arg) /* already done in lowbuffer_init */
{
return OK;
}
/*****************************************************************************/
errcode lowbuf_low_done() /* to be done in lowbuffer_init */
{
return OK;
}
/*****************************************************************************/
/*****************************************************************************/
