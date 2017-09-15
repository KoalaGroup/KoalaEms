/*
 * common/actiontypes.h
 * 
 * $ZEL: actiontypes.h,v 2.7 2011/04/12 21:06:25 wuestner Exp $
 */

#ifndef _actiontypes_h_
#define _actiontypes_h_

/* action types for unsol_StatusChanged */
typedef enum {
    status_action_none,    /* rather worthless */
    status_action_create,  /*  */
    status_action_delete,  /*  */
    status_action_change,  /* data, i.e. in domains */
    status_action_start,   /* readout */
    status_action_stop,    /* readout */
    status_action_reset,   /* readout, VED */
    status_action_resume,  /* readout */
    status_action_enable,  /* IS, dataout */
    status_action_disable, /* IS, dataout */
    status_action_finish,  /* readout */
    status_action_filename /* dataout */
} status_actions;

/* for ActionReady */
typedef enum {
    completion_none,
    completion_dataout
} completion_types;

typedef enum {
    dataout_completion_none,
    dataout_completion_filemark,
    dataout_completion_wind,
    dataout_completion_rewind,
    dataout_completion_seod,
    dataout_completion_unload
} dataout_completion_types;

#endif
