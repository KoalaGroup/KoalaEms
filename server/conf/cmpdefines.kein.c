/* Das ist KEIN "C" !!! */

#include "sconf.h"
#include "altdefines" /* mit definealt.kein.c erzeugt */

#ifdef DEBUG
#undef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif
#ifdef altDEBUG
#undef altDEBUG
#define altDEBUG 1
#else
#define altDEBUG 0
#endif

#if (DEBUG!=altDEBUG)
touch debug.stamp
#endif

#ifndef MAX_VAR
#define MAX_VAR -1
#endif
#ifndef altMAX_VAR
#define altMAX_VAR -1
#endif

#ifndef MAX_TRIGGER
#define MAX_TRIGGER -1
#endif
#ifndef altMAX_TRIGGER
#define altMAX_TRIGGER -1
#endif

#ifndef MAX_IS
#define MAX_IS -1
#endif
#ifndef altMAX_IS
#define altMAX_IS -1
#endif

#ifndef MAX_LAM
#define MAX_LAM -1
#endif
#ifndef altMAX_LAM
#define altMAX_LAM -1
#endif

#ifndef MAX_DATAIN
#define MAX_DATAIN -1
#endif
#ifndef altMAX_DATAIN
#define altMAX_DATAIN -1
#endif

#ifndef MAX_DATAOUT
#define MAX_DATAOUT -1
#endif
#ifndef altMAX_DATAOUT
#define altMAX_DATAOUT -1
#endif

#ifndef EVENT_MAX
#define EVENT_MAX -1
#endif
#ifndef altEVENT_MAX
#define altEVENT_MAX -1
#endif

#ifndef HANDLERCOMSIZE
#define HANDLERCOMSIZE -1
#endif
#ifndef altHANDLERCOMSIZE
#define altHANDLERCOMSIZE -1
#endif

#if (MAX_VAR!=altMAX_VAR)||(MAX_TRIGGER!=altMAX_TRIGGER)||(MAX_IS!=altMAX_IS)\
	||(MAX_LAM!=altMAX_LAM)||(MAX_DATAIN!=altMAX_DATAIN)||(MAX_DATAOUT!=altMAX_DATAOUT)\
	||(EVENT_MAX!=altEVENT_MAX)||(HANDLERCOMSIZE!=altHANDLERCOMSIZE)
touch limits.stamp
#endif
