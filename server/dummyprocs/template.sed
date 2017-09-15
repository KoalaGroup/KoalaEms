#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>

errcode DUMMYPROC(void);
errcode DUMMYPROC(void)
{
T(DUMMYPROC)
return(Err_NotImpl);
}
