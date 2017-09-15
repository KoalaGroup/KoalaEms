/******************************************************************************
*                                                                             *
* getsubevent.c                                                               *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 23.03.94                                                                    *
*                                                                             *
******************************************************************************/

#include <stdio.h>

/******************************************************************************

Subeventformat:
0: IS-id
1: size
2: Beginn der Daten

/*****************************************************************************/

int* getsubevent(int* event, int is_id)
{
int isnum, isidx, *isptr;

isnum=event[3];
isptr=event+4;

for (isidx=0; isidx<isnum; isidx++)
  {
  if (isptr[0]==is_id) return(isptr);
  isptr+=isptr[1]+2;
  }
return(0);
}

/*****************************************************************************/

int* getsubevent_s(int* event, int is_id)
{
int isnum, isidx, *isptr;

isnum=event[3];
isptr=event+4;
if ((isnum<0) || (isnum>100))
  {
  printf("getsubevent: is_num=%d\n", isnum);
  return(0);
  }

for (isidx=0; isidx<isnum; isidx++)
  {
  if ((isptr[0]<0) || (isptr[0]>100))
    {
    printf("getsubevent: is_num=%d; is_id=%d\n", isnum, isptr[0]);
    return(0);
    }
  if (isptr[0]==is_id) return(isptr);
  isptr+=isptr[1]+2;
  if ((isptr<=event) || (isptr>=event+event[0]))
    {
    printf("getsubevent: is_num=%d; verwirrt\n", isnum);
    return(0);
    }
  }
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
