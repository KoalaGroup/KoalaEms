/*
 * objects/var/variables.h
 * 
 * 27.01.93
 * 20.Jun.2002 PW: var_read
 */
/*
 * static char *rcsid="$ZEL: variables.h,v 1.10 2008/01/15 14:10:46 wuestner Exp $";
 */

#ifndef _variables_h_
#define _variables_h_
#include <errorcodes.h>
#include <sconf.h>
#include "varobj.h"

struct Var {
    int len;
    union {
        ems_u32 val;
        ems_u32 *ptr;
    } var;
};

extern struct Var var_list[MAX_VAR]; /* definiert in variables.c */

errcode CreateVariable(ems_u32* p, unsigned int num);
errcode DeleteVariable(ems_u32* p, unsigned int num);
errcode WriteVariable(ems_u32* p, unsigned int num);
errcode ReadVariable(ems_u32* p, unsigned int num);
errcode GetVariableAttributes(ems_u32* p, unsigned int num);

plerrcode var_create(unsigned int index, unsigned int size);
plerrcode var_delete(unsigned int index);
plerrcode var_clear(unsigned int index);
plerrcode var_read_int(unsigned int index, ems_u32* val);
plerrcode var_write_int(unsigned int index, ems_u32 val);
plerrcode var_get_ptr(unsigned int index, ems_u32** ptr);
plerrcode var_attrib(unsigned int index, unsigned int* size);
objectcommon* lookup_var(ems_u32* id, unsigned int idlen, unsigned int* remlen);
ems_u32* dir_var(ems_u32* ptr);
errcode var_init(void);
errcode var_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
