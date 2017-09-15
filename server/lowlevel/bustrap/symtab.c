/******************************************************************************
*                                                                             *
* symtab.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 11.10.94                                                           *
* last changed: 20.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <errorcodes.h>
#include <module.h>
#include <procid.h>
#include "bustrap.h"

static int pid;              /* own process id */
static mod_exec* pmodul;     /* primary module */
static struct modhcom* stbptr;
static procid pd;            /* own process descriptor */
static char* pname;          /* name of primary module */
static char stbname[128];    /* name of symbol table module */

/*****************************************************************************/

int get_process_info()
{
int name_offs;

pid=getpid();
if (_get_process_desc(pid, sizeof(procid), &pd)==-1)
  {
  printf("cannot get process descriptor: errno=%d\n", errno);
  return(-1);
  }

pmodul=pd._pmodul;

name_offs=pmodul->_mh._mname;
pname=(char*)pmodul+name_offs;
strcpy(stbname, pname);
strcat(stbname, ".stb");

stbptr=(struct modhcom*)modlink(stbname, mktypelang(MT_DATA, ML_ANY));
if (stbptr==(struct modhcom*)-1)
    stbptr=(struct modhcom*)modloadp(stbname, 0, (char*)0);

if (stbptr==(struct modhcom*)-1)
  {
  printf("cannot link module \"%s\"; errno=%d\n", stbname, errno);
  return(-1);
  }

return(0);
}

/*****************************************************************************/

void printsym(trappc abs_pc)
{
typedef struct symtab
  {
  short format;
  int crc;
  int symofs;
  int symanz;
  } symtab;

typedef struct symentry
  {
  int val;
  short type;
  int ofs;
  } symentry;


symtab* tab;
symentry* syms;
int symanz, i;
char *base;
int pc;
int stbcrc;
int pcrc;

if (get_process_info()!=0) return;

tab=(symtab*)((char*)stbptr+stbptr->_msymbol);
stbcrc=tab->crc;
pcrc=*(int*)((char*)pmodul+pmodul->_mh._msize-4);
if (stbcrc!=pcrc)
  {
printf("Warning: CRC in stb module and CRC of primary module are different!\n");
  printf("stb: 0x%06x; pm: 0x%06x\n", stbcrc, pcrc);
  }
else
  printf("CRC: 0x%06x\n", stbcrc);

syms=(symentry*)((char*)stbptr+tab->symofs);
symanz=tab->symanz;
base=(char*)stbptr;
pc=(char*)abs_pc-(char*)pmodul;

i=symanz-1;
while ((i>=0) && (((syms[i].type&7)!=4) || (syms[i].val>pc))) i--;
if ((syms[i].type&7)!=4)
  {
  printf("found no possible procedure name\n");
  }
else
  {
  printf("0x%08x  %s\n", syms[i].val, base+syms[i].ofs);
  printf("0x%08x  pc (relative)\n", pc);
  do i++; while((i<symanz) && ((syms[i].type&7)!=4));
  if ((syms[i].type&7)==4)
    printf("0x%08x  %s\n", syms[i].val, base+syms[i].ofs);
  }

munlink(stbptr);
}

/*****************************************************************************/
/*****************************************************************************/
