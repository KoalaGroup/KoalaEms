/* sjy_xrespn.c				jms 28-sep-94 */

/* returns X from last command */

#include "ieee_fun_types.h"

short xrespn(void)
{
	return(ieee_last_X);
}
short xrespn_(void)
{
	return(xrespn());
}
