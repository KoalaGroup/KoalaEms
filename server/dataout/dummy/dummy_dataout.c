/*
 * dataout/dummy/dummy_dataout.c
 * created before 22.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dummy_dataout.c,v 1.12 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../../objects/pi/readout.h"

int *next_databuf;
int buffer_free=1; /* immer frei */

#ifndef EVENT_BUFBEG
static int _databuf[2*event_max+1],*databuf;
#else
static int *databuf=(int*)EVENT_BUFBEG;
#endif

static int datalen,welcher;

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "dataout/dummy")

/*****************************************************************************/

int printuse_output(FILE* outfilepath)
{
return 0;
}

/*****************************************************************************/

errcode dataout_init(){
T(dataout_init)
#ifndef EVENT_BUFBEG
#ifdef OSK
databuf=(int*)(((int)_databuf+3)&0xfffffff0); /*4-Byte-Alignment erzwingen*/
#else
databuf=_databuf;
#endif
#else
#ifdef OSK
if (permit(EVENT_BUFBEG, (char*)EVENT_BUFEND-(char*)EVENT_BUFBEG, ssm_RW)!=1)
  {
  printf(
      "dataout_init: cannot permit eventbuf (start=0x%08x; size=0x%x)\n",
        EVENT_BUFBEG, (char*)EVENT_BUFEND-(char*)EVENT_BUFBEG);
  printf("errno=%d\n", errno);
  return(Err_System);
  }
#endif
#endif
datalen=0;
return(OK);
}

errcode dataout_done(){
T(dataout_done)
return(OK);
}

errcode start_dataout()
{
T(start_dataout)
datalen=0;
welcher=1;
next_databuf=databuf;
return(OK);
}

/*****************************************************************************/

errcode stop_dataout()
{
T(stop_dataout)
return(OK);
}

/*****************************************************************************/

void flush_databuf(p)
int *p;
{
T(flush_databuf)
datalen=p-next_databuf;
DV(D_TR,printf("Daten out: %d Bytes\n", datalen*sizeof(int));)
DV(D_DUMP, int i; for (i=0; i<datalen; i++) printf("\t%d\n", next_databuf[i]);)
if(welcher){
	next_databuf=databuf+event_max;
	welcher=0;
}else{
	next_databuf=databuf;
	welcher=1;
}
}

/*****************************************************************************/

errcode get_last_databuf()
{
register int len=datalen;
register int *ptr=(welcher?databuf+event_max:databuf);

T(get_last_databuf)
if (len)
  {
  while (len--)*outptr++= *ptr++;
  return(OK);
  }
else
  return(Err_NoDomain);
}

/*****************************************************************************/

void schau_nach_speicher()
{
T(schau_nach_speicher)
}

/*****************************************************************************/
/*****************************************************************************/

