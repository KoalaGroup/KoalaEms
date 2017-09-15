#ifndef OSK
#include <stdlib.h>
#endif
#include <strings.h>

#error not to be used

char *strdup(char* s)
{
  char *help;
  help=(char*)malloc(strlen(s)+1);
  if(help)strcpy(help,s);
  return(help);
}
