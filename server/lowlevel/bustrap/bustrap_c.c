/******************************************************************************
*                                                                             *
* bustrap.c                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 07.10.94                                                           *
* last changed: 23.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <setjmp.h>
#include <ssm.h>
#include "bustrap.h"

extern int init_bustrap();
extern int done_bustrap();
extern void btrap0();
extern void* trapproc;

extern void cleanup();
extern void done_comm();

static jmp_buf jmpbuf;
static int jmpset;
static trapfunc bustrapfuncs[10];
static int numtrapfuncs;
static int maxtrapfuncs=10;

/*****************************************************************************/

errcode bustrap_low_init(char* arg)
{
T(bustrap_low_init)
if (init_bustrap()!=0)
  {
  printf("bustrap_low_init: Fehler %d\n", errno);
  return(Err_System);
  }
trapproc=btrap0;
jmpset=0;
numtrapfuncs=0;
return(OK);
}

/*****************************************************************************/

errcode bustrap_low_done()
{
T(bustrap_low_done)
if (done_bustrap()!=0)
  {
  printf("bustrap_low_done: Fehler %d\n", errno);
  return(Err_System);
  }
return(OK);
}

/*****************************************************************************/

void add_bustrapfunc(void (*func)(short*))
{
T(add_bustrapfunc)

if (numtrapfuncs==maxtrapfuncs)
  printf("add_bustrapfunc: no more trapfuncs.\n");
else
  bustrapfuncs[numtrapfuncs++]=func;
}

/*****************************************************************************/

void change_bustrap(void *trapfunc)
{
if (trapfunc)
  trapproc=trapfunc;
else
  trapproc=btrap0;
}

/*****************************************************************************/

void unregister_jmp()
{
jmpset=0;
}

/*****************************************************************************/

void register_jmp(jmp_buf buf)
{
memcpy(jmpbuf, buf, sizeof(jmp_buf));
jmpset=1;
}

/*****************************************************************************/

char* check_access(char* addr, int size, int needpermit)
{
int res;
char* pres;

T(check_access)
if (needpermit)
  {
  if (permit(addr, size, ssm_RW)!=1)
    {
    printf("check_access: cannot get permission for addr 0x%08x, size 0x%x\n",
        addr, size);
    return((char*)-1);
    }
  }
D(D_MEM, printf("check_access(0x%08x, 0x%x, %d)\n", addr, size, needpermit);)

jmpset=1;
if ((res=setjmp(jmpbuf))!=0)
  {
  pres=(char*)res;
  }
else
  {
  chk_acc(addr, size); /* hier kann er 'rausfliegen */
  pres=0;
  }

jmpset=0;
if (needpermit) protect(addr, size);
return(pres);
}

/*****************************************************************************/

void btrap_hand(trappc pc, char* a0)
{
T(btrap_hand)
if (jmpset)
  {
  D(D_MEM, printf("\nBusError exception at PC 0x%08x, a0=0x%08x; ", pc, a0);
      printf("wird abgefangen\n");)
  longjmp(jmpbuf, a0);
  }
else
  {
  int i;
  printf("\nBusError exception at PC 0x%08x\n", pc);
  printsym(pc);
  for (i=0; i<numtrapfuncs; i++) bustrapfuncs[i](pc);
  shutdown_server();
  exit(E_BUSERR);
  }
}

/*****************************************************************************/
/*****************************************************************************/
