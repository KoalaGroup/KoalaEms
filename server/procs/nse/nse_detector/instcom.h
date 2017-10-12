/***********************************************/
/*     Structure of instcom Data module        */
/*           for   I N 1 5                     */
/*     Use of control_log flag                 */
/*     Bit 0 : module control: print log info  */
/*     Bit 3 : module tof: print log info      */
/***********************************************/

#define VPB_SIZE 128
#define ORB_SIZE 128
typedef struct
  {
   union
    {long _l[VPB_SIZE];
     long _p[VPB_SIZE][1];} _vpb; /* VAX defined parameters,OS9 read only */
   union
    {long _l[ORB_SIZE];
     long _p[ORB_SIZE][1];} _orb; /* OS9 defined parameters,VAX read only */
  } instcom;

/***********************************************/
/*        Offsets in VAX parameter module      */
/***********************************************/


#define _T_RES            0
#define _N_DET            1
#define _CLOCK_DIVID      2
#define _TRIG_DELAY       3
#define _TRIG_READ_FACTOR 4
#define _TIME_BIT         5
#define _D_MASK           6
#define _GRP_SPCT        14
#define _PRESET          15
#define _PRESET_TYP      16
#define _SAVE_DATA       17

#define _ANGLES_CON      18
#define _END_SWITCH_LOW  23
#define _END_SWITCH_HIGH 28
#define _DECALAGE        33
#define _MOT_PAR_W       38

#define _BOBINES_CON     68
#define _B_DEB           96
#define _B_FIN           97
#define _B_STEP          98
#define _B_LOW_BUT       99
#define _B_HI_BUT       100

#define _PACK_DATA      124
#define _VME_NODE	125
#define _VAX_ACQ_STAT   127

/*********************************************************/
/*        Simplified Access to VAX parameter module      */
/*********************************************************/

#define t_res            _vpb._l[_T_RES]
#define n_det            _vpb._l[_N_DET]
#define clock_divid      _vpb._l[_CLOCK_DIVID]
#define trig_delay       _vpb._l[_TRIG_DELAY]
#define trig_read_factor _vpb._l[_TRIG_READ_FACTOR]
#define time_bit         _vpb._l[_TIME_BIT]
#define d_mask           _vpb._p[_D_MASK]
#define grp_spct         _vpb._l[_GRP_SPCT]
#define preset           _vpb._l[_PRESET]
#define preset_typ       _vpb._l[_PRESET_TYP]
#define save_data        _vpb._l[_SAVE_DATA]

#define angles_con       _vpb._p[_ANGLES_CON]
#define end_switch_low   _vpb._p[_END_SWITCH_LOW]
#define end_switch_high  _vpb._p[_END_SWITCH_HIGH]
#define decalage         _vpb._p[_DECALAGE]
#define mot_par_w        _vpb._p[_MOT_PAR_W]

#define bobines_con      _vpb._p[_BOBINES_CON]
#define b_deb            _vpb._l[_B_DEB]
#define b_fin            _vpb._l[_B_FIN]
#define b_step           _vpb._l[_B_STEP]
#define b_low_but        _vpb._l[_B_LOW_BUT]
#define b_hi_but         _vpb._l[_B_HI_BUT]
#define pack_data        _vpb._l[_PACK_DATA]
#define vme_node         _vpb._p[_VME_NODE]
#define vax_acq_stat     _vpb._l[_VAX_ACQ_STAT]
/***********************************************/
/*        Offsets in OS9 parameter module      */
/***********************************************/
#define _SS_TIMER	0 
#define _SS_MONITOR	1
#define _SS_C2          2
#define _SS_C3		3
#define _SS_STOP        4
#define _SS_CHOICE      5
#define _SS_COEF	6
#define _SS_DUREE	7
#define _SS_CONSIGNE    8
#define _SS_CU_TIM      9
#define _SS_CU_MON     10
#define _SS_CU_C2      11
#define _SS_CU_C3      12
#define _PID_SS        13

#define _ANGLES        14
#define _ANG_STAT      19
#define _MOT_PAR_R     24

#define _BOBINES       54
#define _BOB_STAT      82
#define _BOB_LOW      110
#define _BOB_HI       111
#define _BOB_ON       112

#define _TRIGGER      124
#define _HOST_NODE    125
#define _CONTROL_LOG  127
/*********************************************************/
/*        Simplified Access to OS9 parameter module      */
/*********************************************************/
#define ss_timer       _orb._l[_SS_TIMER]
#define ss_monitor     _orb._l[_SS_MONITOR]
#define ss_c2          _orb._l[_SS_C2]
#define ss_c3          _orb._l[_SS_C3]
#define ss_stop        _orb._l[_SS_STOP]
#define ss_choice      _orb._l[_SS_CHOICE]
#define ss_coef        _orb._l[_SS_COEF]
#define ss_duree       _orb._l[_SS_DUREE]
#define ss_consigne    _orb._l[_SS_CONSIGNE]
#define ss_cu_tim      _orb._l[_SS_CU_TIM]
#define ss_cu_mon      _orb._l[_SS_CU_MON]
#define ss_cu_c2       _orb._l[_SS_CU_C2]
#define ss_cu_c3       _orb._l[_SS_CU_C3]
#define pid_ss         _orb._l[_PID_SS]

#define angles         _orb._p[_ANGLES]
#define ang_stat       _orb._p[_ANG_STAT]
#define mot_par_r      _orb._p[_MOT_PAR_R]

#define bobines        _orb._p[_BOBINES]
#define bob_stat       _orb._p[_BOB_STAT]
#define bob_low	       _orb._l[_BOB_LOW]
#define bob_hi         _orb._l[_BOB_HI]
#define bob_on         _orb._l[_BOB_ON]

#define trigger        _orb._l[_TRIGGER]
#define host_node      _orb._p[_HOST_NODE]
#define control_log    _orb._l[_CONTROL_LOG]

