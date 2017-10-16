#ifndef _gnuthrow_
#define _gnuthrow_

#ifndef __GNUC_OLD__
#define THROW(x) throw x
#define C_Error C_error
#else
#define THROW(x) throw new C_errorbox(x)
#define C_Error C_errorbox
#endif

#endif
