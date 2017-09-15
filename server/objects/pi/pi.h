#ifndef _pi_h_
#define _pi_h_

#include <errorcodes.h>

errcode pi_init(void);
errcode pi_done(void);

errcode CreateProgramInvocation(ems_u32* p, unsigned int num);
errcode DeleteProgramInvocation(ems_u32* p, unsigned int num);
errcode StartProgramInvocation(ems_u32* p, unsigned int num);
errcode ResetProgramInvocation(ems_u32* p, unsigned int num);
errcode StopProgramInvocation(ems_u32* p, unsigned int num);
errcode ResumeProgramInvocation(ems_u32* p, unsigned int num);
errcode GetProgramInvocationAttr(ems_u32* p, unsigned int num);
errcode GetProgramInvocationParams(ems_u32* p, unsigned int num);

#endif
