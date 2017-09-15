/******************************************************************************
*                                                                             *
* procedures.v.c                                                              *
*                                                                             *
* OS9/ULTRIX                                                                  *
*                                                                             *
* created: 11.09.94                                                           *
* last changed: 23.09.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include "../../main/requests.h"
#include "../../../common/requesttypes.h"
#include "../../procs/listprocs.h"
#include "names.h"

extern ems_u32* outptr;

/*****************************************************************************/

NORMAL_VARIABLE(proctest, forth, "proctest", 1)

/*****************************************************************************/

VOID dolocalprocedure()
{
int* oldoutptr;
plerrcode res;
int num, proc;

proc=spop(INT32);
oldoutptr=outptr;

if (!proctest.parameter || ((res=Proc[proc].test_proc(&sp->INT32))==plOK))
  {
  res=Proc[proc].proc(&sp->INT32);
  }

sp+=sp->INT32+1;
num=outptr-oldoutptr;
for (; outptr>oldoutptr;) (*(--sp)).INT32=*(--outptr);
spush(num, INT32);
spush(res, INT32);
}

NORMAL_CODE(localprocedure, proctest, "localprocedure", dolocalprocedure)

/*****************************************************************************/

VOID dotestprocedure()
{
int* oldoutptr;
plerrcode res;
int num, proc;

proc=spop(INT32);
oldoutptr=outptr;

res=Proc[proc].test_proc(&sp->INT32);

sp+=sp->INT32+1;
num=outptr-oldoutptr;
for (; outptr>oldoutptr;) (*(--sp)).INT32=*(--outptr);
spush(num, INT32);
spush(res, INT32);
}

NORMAL_CODE(testprocedure, localprocedure, "testprocedure", dotestprocedure)

/*****************************************************************************/

VOID dorequest()
{
int* oldoutptr;
errcode res;
int num, req;

req=spop(INT32);
num=spop(INT32);
oldoutptr=outptr;

res=DoRequest[req](&sp->INT32, num);

sp+=num;
num=outptr-oldoutptr;
for (; outptr>oldoutptr;) (*(--sp)).INT32=*(--outptr);
spush(num, INT32);
spush(res, INT32);
}

NORMAL_CODE(request, testprocedure, "request", dorequest)

/*****************************************************************************/

VOID doprocidx() /* ver (name) --> idx flag */
{
char* name;
int idx, ver;

ver=spop(INT32);
spush(' ', INT32);
doword();
name=spop(CSTR);

idx=0;
while ((idx<NrOfProcs)
    && ((strcmp(Proc[idx].name_proc, name)!=0) || (*Proc[idx].ver_proc!=ver))
    )
  idx++;

if (idx<NrOfProcs)
  {
  spush(idx, INT32);
  spush(1, INT32);
  }
else
  {
  spush(0, INT32);
  }
}

NORMAL_CODE(procidx, request, "procidx", doprocidx)

/*****************************************************************************/

VOID dolastprocidx() /* (name) --> idx flag */
{
char* name;
int idx, i, ver;

spush(' ', INT32);
doword();
name=spop(CSTR);

idx=0; ver=-1;
for (i=0; i<NrOfProcs; i++)
  {
  if ((strcmp(Proc[i].name_proc, name)==0) && (*Proc[i].ver_proc>ver))
      idx=i;
  }

if (idx>-1)
  {
  spush(idx, INT32);
  spush(1, INT32);
  }
else
  {
  spush(0, INT32);
  }
}

NORMAL_CODE(lastprocidx, procidx, "lastprocidx", dolastprocidx)

/*****************************************************************************/

VOID dorequestidx()
{
char* name;
int idx;

spush(' ', INT32);
doword();
name=spop(CSTR);

idx=0;
while ((idx<NrOfReqs) && (strcmp(reqstrs[idx], name)!=0)) idx++;

if (idx<NrOfReqs)
  {
  spush(idx, INT32);
  spush(1, INT32);
  }
else
  {
  spush(0, INT32);
  }
}

NORMAL_CODE(requestidx, lastprocidx, "requestidx", dorequestidx)

/*****************************************************************************/

VOID dolistreqs()
{
int req;

req=spop(INT32);

printf(">%s<\n", reqstrs[req]);
}

NORMAL_CODE(listreqs, requestidx, "listreqs", dolistreqs)

/*****************************************************************************/

VOID dodoterrcode()
{
errcode code;

code=spop(INT32);
if (code<NrOfErrs)
  emit_s(errstr[code]);
else
  {
  char s[256];
  sprintf(s, "unknown error %d", code);
  emit_s(s);
  }
}

/*NORMAL_CODE(doterrcode, requestidx, ".errcode", dodoterrcode)*/
NORMAL_CODE(doterrcode, listreqs, ".errcode", dodoterrcode)

/*****************************************************************************/
/*****************************************************************************/
