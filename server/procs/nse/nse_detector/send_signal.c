#include <stdio.h>
#include <module.h>
#include <procid.h>
#define ERROR -1
/*****************************************************************************/
/*                                                                           */
/*                      Function send_signal                                 */
/*   Search process *name in process table                                   */
/*   If found: signal == 0: return its PID                                   */
/*             signal != 0: send signal, return kill return code             */
/*   If not found:          return -1
/*****************************************************************************/
send_signal(name,signal)
char *name;             /* name of process */
short signal;           /* signal number, or 0 */
{
extern int errno;
struct proctable {
        unsigned short
                _maxblock,
                _blocksize;
                procid *proc_ptr[127];
        }proctab;
procid   proc_desc;
mod_exec module_header,*modptr;
char     modname[40];
long     n,i;

_get_process_table( &proctab, sizeof(proctab) );

for(n=0; n<proctab._maxblock; ++n) 

 if (proctab.proc_ptr[n] != NULL) {

  _cpymem(0,sizeof(proc_desc),proctab.proc_ptr[n], &proc_desc);
  modptr = proc_desc._pmodul;
  _cpymem(0,sizeof(module_header),modptr,&module_header);
  _cpymem(0,sizeof(modname),(unsigned)modptr+module_header._mh._mname, modname);
  if( strcmp(modname,name) == 0) {

   if (signal >0) return (kill(proc_desc._id,signal)==ERROR ? errno:0);
   else if (signal == 0) return (proc_desc._id) ;
   else return (-1) ;
   }

 }
return -1;
}
