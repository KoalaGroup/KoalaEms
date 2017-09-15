/******************************************************************************
*                                                                             *
* fopengz.c                                                                   *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created 21.06.95                                                            *
* last changed 21.06.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <fopengz.h>
#include <stdio.h>

FILE* fopengz(const char* name, int* flag)
{
int compressed, l;
FILE* f;

l=strlen(name);
compressed=0;
if ((l>2) && (strcmp(name+l-2, ".Z")==0)) compressed=1;
if (compressed) printf("fopengz: .Z found\n");
if ((l>3) && (strcmp(name+l-3, ".gz")==0)) compressed=1;
if (compressed) printf("fopengz: .Z or .gz found\n");
*flag=compressed;
if (compressed)
  {
  char s[1200];
  strcpy(s, "zcat ");
  strcat(s, name);
  f=popen(s, "r");
  }
else
  f=fopen(name, "r");
}

int fclosegz(FILE* f, int flag)
{
if (flag)
  return(pclose(f));
else
  return(fclose(f));
}

/*****************************************************************************/
/*****************************************************************************/
