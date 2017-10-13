/*
 * common/objecttypes.h
 * created before 03.05.93
 */

/* $ZEL: objecttypes.h,v 2.25 2015/04/24 20:55:39 wuestner Exp $ */

#ifndef _objecttypes_h_
#define _objecttypes_h_

typedef enum
  {
  Object_null, Object_ved, Object_domain, Object_is, Object_var, Object_pi,
  Object_do
  } Object;

typedef enum
  {
  Dom_null, Dom_Modullist, Dom_LAMproclist, Dom_Trigger, Dom_Event, Dom_Datain,
  Dom_Dataout
  } Domain;

typedef enum
  {
  Invocation_null, Invocation_readout, Invocation_LAM
  } Invocation;

typedef enum{
  Invoc_error= -3,
  Invoc_alldone,   /* -2 */
  Invoc_stopped,   /* -1 */
  Invoc_notactive, /*  0 */
  Invoc_active     /*  1 */
}InvocStatus;

/*****************************************************************************/

typedef enum
  {
  Capab_listproc, Capab_trigproc
  } Capabtyp;

typedef enum {
    InOut_Ringbuffer, InOut_Stream, InOut_Cluster, InOut_Filebuffer,
    InOut_Selected_Events, InOut_Opaque, InOut_MQTT
} InOutTyp;

typedef enum
  {
  Addr_Raw, Addr_Modul, Addr_Driver_mapped, Addr_Driver_mixed,
  Addr_Driver_syscall, Addr_Socket, Addr_LocalSocket, Addr_File, Addr_Tape,
  Addr_Null, Addr_AsynchFile, Addr_Autosocket, Addr_V6Socket
  } IOAddr;

typedef enum {
    ip_default=0,
    ip_v4only=1,
    ip_v6only=2,
    ip_passive=4
} ip_flags;

typedef enum{
  Do_running=0,
  Do_neverstarted,
  Do_done,
  Do_error,
  Do_flushing
}DoRunStatus;

typedef enum{
  Do_noswitch= -1,
  Do_enabled,
  Do_disabled
}DoSwitch;

/*
 * modul_unspec must not be used any longer,
 * therefore remaned to modul_unspec_
 */
// rename modul_unspec_ back, by yong
enum Modulclass {
    modul_none, modul_unspec, modul_generic, modul_camac,
    modul_fastbus, modul_vme, modul_lvd, modul_perf, modul_can, modul_caenet,
    modul_sync, modul_pcihl, modul_ip, modul_invalid
};
typedef enum Modulclass Modulclass;

enum errpairflags {
    errfl_none=0, /* to be misused later */
    errfl_text=1,
    errfl_sound=2,
    errfl_picture=3,
    errfl_program=4,
    errfl_mail=5,
    errfl_addressee=6
};
enum errseverity {
    errsev_info=0,
    errsev_warning=1,
    errsev_error=2,
    errsev_catastrophic=3
};


extern const char* Modulclass_names[];

#endif

/*****************************************************************************/
/*****************************************************************************/

