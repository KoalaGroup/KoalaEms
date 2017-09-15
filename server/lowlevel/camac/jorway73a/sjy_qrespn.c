/* sjy_qrespn.c				jms 28-sep-94 */

/* returns Q from last command */

#include "ieee_fun_types.h"

short qrespn(void)
{
  return(ieee_last_Q);

}
short qrespn_()
{
	return(qrespn());
}
