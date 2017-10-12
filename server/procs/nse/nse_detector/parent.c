#include <stdio.h>
#include <module.h>
#include <procid.h>


char *parent()
{
long     p_id;
procid   proc_desc;
mod_exec module_header,*modptr;
char     modname[40];

modname[0]=0;
p_id=getpid();
_get_process_desc(p_id,sizeof(proc_desc),&proc_desc );
if ( p_id=proc_desc._pid) 
   if (_get_process_desc(p_id,sizeof(proc_desc),&proc_desc )) {
      modptr = proc_desc._pmodul;
      _cpymem(0,sizeof(module_header),modptr,&module_header);
      _cpymem(0,sizeof(modname),(unsigned)modptr+module_header._mh._mname,
       modname);
   }
return modname ;
}
