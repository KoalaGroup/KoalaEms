/*
 * lowlevel/lvd/lvd_access.h
 * created 14.Dec.2003 PW
 * $ZEL: lvd_access.h,v 1.10 2009/12/03 00:04:53 wuestner Exp $
 */

#ifndef _lvd_access_h_
#define _lvd_access_h_

#include "lvd_map.h"

#define ofs(what, elem) ((size_t)&(((what*)0)->elem))
#define siz(what, elem) (sizeof(((what*)0)->elem))

/*
 * old: (with slave controllers)
 * addressing the cards:
 * ccccc|iiii
 * iiii: address of the card in the crate (ignored if broadcast)
 * ccccc: address of the controller; primary controller has 0x10
 */
/*
 * new: (without slave controllers)
 * addressing the cards:
 * iiii
 * iiii: address of the card in the crate (ignored if broadcast)
 * iiii+0x100 is converted to iiii
 */

int lvd_c_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data);

#define lvd_cc_w(dev, elem, data) \
    lvd_c_w_(dev, ofs(struct lvd_controller_common, elem), \
    siz(struct lvd_controller_common, elem), data)
#define lvd_c1_w(dev, elem, data) \
    lvd_c_w_(dev, ofs(struct lvd_controller_1, elem), \
    siz(struct lvd_controller_1, elem), data)
#define lvd_c2_w(dev, elem, data) \
    lvd_c_w_(dev, ofs(struct lvd_controller_2, elem), \
    siz(struct lvd_controller_2, elem), data)
#define lvd_c3_w(dev, elem, data) \
    lvd_c_w_(dev, ofs(struct lvd_controller_3, elem), \
    siz(struct lvd_controller_3, elem), data)


int lvd_c_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data);

#define lvd_cc_r(dev, elem, data) \
    lvd_c_r_(dev, ofs(struct lvd_controller_common, elem), \
    siz(struct lvd_controller_common, elem), data)
#define lvd_c1_r(dev, elem, data) \
    lvd_c_r_(dev, ofs(struct lvd_controller_1, elem), \
    siz(struct lvd_controller_1, elem), data)
#define lvd_c2_r(dev, elem, data) \
    lvd_c_r_(dev, ofs(struct lvd_controller_2, elem), \
    siz(struct lvd_controller_2, elem), data)
#define lvd_c3_r(dev, elem, data) \
    lvd_c_r_(dev, ofs(struct lvd_controller_3, elem), \
    siz(struct lvd_controller_3, elem), data)

#if 0
int lvd_cb_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data);

#define lvd_cb1_w(dev, elem, data) \
    lvd_cb_w_(dev, ofs(struct lvd_controller_bc_1, elem), \
    siz(struct lvd_controller_bc_1, elem), data)


int lvd_cb_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data);

#define lvd_cb1_r(dev, elem, data) \
    lvd_cb_r_(dev, ofs(struct lvd_controller_bc_1, elem), \
    siz(struct lvd_controller_bc_1, elem), data)
#endif

int lvd_i_w_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32 data);

#define lvd_i_w(dev, addr, elem, data) \
    lvd_i_w_(dev, addr, ofs(union lvd_in_card, elem), \
    siz(union lvd_in_card, elem), data)


int lvd_i_wblind_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32 data);

#define lvd_i_wblind(dev, addr, elem, data) \
    lvd_i_wblind_(dev, addr, ofs(union lvd_in_card, elem), \
    siz(union lvd_in_card, elem), data)


int lvd_i_r_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32* data);

#define lvd_i_r(dev, addr, elem, data) \
    lvd_i_r_(dev, addr, ofs(union lvd_in_card, elem), \
    siz(union lvd_in_card, elem), data)


int lvd_ib_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data);

#define lvd_ib_w(dev, elem, data) \
    lvd_ib_w_(dev, ofs(struct lvd_in_card_bc, elem), \
    siz(struct lvd_in_card_bc, elem), data)


int lvd_ib_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data);

#define lvd_ib_r(dev, elem, data) \
    lvd_ib_r_(dev, ofs(struct lvd_in_card_bc, elem), \
    siz(struct lvd_in_card_bc, elem), data)

#endif
