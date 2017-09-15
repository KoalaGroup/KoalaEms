/* Das ist nicht als "C"-Quelle gedacht !!! */

#include "sconf.h.alt"

#ifdef DEBUG
%define altDEBUG
#endif

#ifdef EVENT_MAX
%define altEVENT_MAX EVENT_MAX
#endif
#ifdef HANDLERCOMSIZE
%define altHANDLERCOMSIZE HANDLERCOMSIZE
#endif
#ifdef MAX_VAR
%define altMAX_VAR MAX_VAR
#endif
#ifdef MAX_TRIGGER
%define altMAX_TRIGGER MAX_TRIGGER
#endif
#ifdef MAX_IS
%define altMAX_IS MAX_IS
#endif
#ifdef MAX_LAM
%define altMAX_LAM MAX_LAM
#endif
#ifdef MAX_DATAIN
%define altMAX_DATAIN MAX_DATAIN
#endif
#ifdef MAX_DATAOUT
%define altMAX_DATAOUT MAX_DATAOUT
#endif
