/* Genauigkeitstoleranz fuer die Motoren */
#define EPS 4
#define VOLT_min (-3.48)
#define VOLT_max  3.14
#define CM_min    1.1
#define CM_max   36.6

/* sleep fuer das Fahren einzelner Schritte */
#define s_leep ((1<<5)|0x80000000) /* hier 1/16 Sekunde */

/* Synchronisation */   
#define CNTR_master    0xc0
#define CNTR_slave     0xc1

/* Register select */
#define CNTR_pulse     0x80

#define CNTR_stop      0xf1
#define CNTR_decel     0x14
#define CNTR_decelf2   0x13

/* Control word */
#define CNTR_slow      0x40
#define CNTR_slowpos   0x40
#define CNTR_slowneg   0x48
#define CNTR_step      0x44
#define CNTR_steppos   0x44
#define CNTR_stepneg   0x4c
#define CNTR_stepposAv 0x46
#define CNTR_stepnegAv 0x4e

/* Mode of operation */
#define CNTR_reset     0x08
#define GO_slow        0x10
#define GO_slowr2      0x11
#define GO_quickbyfreq 0x14
#define GO_quick       0x15
#define GO_quickf2     0x17

/* Offsets fuer SSI-Encode */
#define S_GET_    0x00
#define S_PUT_    0x01
#define T_LOW_    0x04
#define T_HIGH_   0x05
#define P_LOW_    0x06
#define P_HIGH_   0x07
#define M_OK_     0x08
#define M_NB_     0x09   
#define PARM_NEW_ 0x0A

/* Aktionen zum SSI-Encoder */
#define READ_SSI_PARM  0x100
#define READ_SSI_VAL   0x200
#define WRITE_SSI_PARM 0x300

/* SSI-Encoder Returncodes */
#define SEMAOK 0x00
#define NOSEMA 0x01

/* Aktionen zum M_362-Motor */
#define SETALARM  0x400
#define GETSIGNAL 0x500
#define GOREF     0x600
#define GOLIM_R   0x700
#define GOLIM_L   0x800

/* M_E362_Motor Returncodes */
#define CANTSETALARM  0x02
#define CANTGETSIGNAL 0x03
#define REF_DISABLE   0x04
#define LIM_R_DISABLE 0x05
#define LIM_L_DISABLE 0x06



/* maximale Anzahl von Versuchen um Sempaphor zu belegen */
