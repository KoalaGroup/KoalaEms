


#include <module.h>
#define INSTCOM "instcom"
#define ERROR -1
typedef struct    dat_mod
          {
          struct modhcom _mh;         /* module header */
          long   _datastart;          /* pointer to data in module */
          } data_mod;
/**************************************************************/

link_instcom()
{
long      pva;
data_mod  *modlink(),*modptr ;
   if ((long)(modptr=modlink(INSTCOM,0))==ERROR) return(ERROR);
   pva=(long)modptr+modptr->_datastart;
   munlink(modptr);
   return(pva);
}
/**************************************************************/
