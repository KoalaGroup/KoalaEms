/************************************************************************
 *
 * S.Trusov: temporary for spectator tests
 *
 ************************************************************************/                           
#ifndef _v1290readout_HH_
#define _v1290readout_HH_

#include "../../../objects/domain/dom_ml.h"

#define V1290_STATUS_REG 0x1002
#define V1290_CLEAR_REG  0x1016

#define V1290_WTYPE_SHIFT    27
#define V1290_GLOBAL_HEADER  0x08
#define V1290_GLOBAL_TRAILER 0x10
#define V1290_FILLER         0x18
#define V1290_GLOBAL_TAG     0x11

#define V1290_TDC_HEADER     0x01
#define V1290_TDC_TRAILER    0x03
#define V1290_TDC_DATA       0x00
#define V1290_TDC_ERR        0x04


/* defines for Micro Controller */
/* defines for Micro Controller registers */
#define V1290_MicroReg       0x102e
#define V1290_MicroHandshake 0x1030

/* defines for Micro Controller bits */
#define V1290_WRITE_OK       0x1
#define V1290_READ_OK        0x2

#define V1290_LEAD_EDGE      0x2
#define V1290_TRAIL_EDGE     0x1
#define V1290_BOTH_EDGE      0x3
#define V1290_PAIR_EDGE      0x0

#define V1290_EDGE_025       0x3
#define V1290_EDGE_100       0x2
#define V1290_EDGE_200       0x1
#define V1290_EDGE_800       0x0


/* defines for Micro Controller operation codes */
#define V1290_OC_TRG_MATCH      0x0000
#define V1290_OC_CONT_STOR      0x0100
#define V1290_OC_LOAD_DEF_CFG   0x0500
#define V1290_OC_LOAD_USER_CFG  0x0700

#define V1290_OC_WIN_WIDTH      0x1000
#define V1290_OC_WIN_OFFS       0x1100
#define V1290_OC_SW_MARGIN      0x1200
#define V1290_OC_REJ_MARGIN     0x1300
#define V1290_OC_EN_SUB_TRG     0x1400
#define V1290_OC_DIS_SUB_TRG    0x1400

#define V1290_OC_EDGE_MODE      0x2200
#define V1290_OC_EDGE_RES       0x2400

#define V1290_OC_EN_HEAD        0x3000
#define V1290_OC_DIS_HEAD       0x3100

#define V1290_MASK_32           0x1f
#define V1290_OC_EN_CHAN        0x4000
#define V1290_OC_DIS_CHAN       0x4100



int v1290readout __P((ml_entry *,int));
int v1290readoutDSP __P((ml_entry *,int));
void v1290clear  __P((ml_entry * module));

#endif
