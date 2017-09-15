/*
 * lowlevel/lvd/lvd_access.c
 * created 25.Mar.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_access.c,v 1.11 2011/04/06 20:30:25 wuestner Exp $";

#include <rcs_ids.h>
#include "lvdbus.h"
#include "lvd_access.h"

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

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
int
lvd_c_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data)
{
    int res;

    res=dev->write(dev, ofs(struct lvd_bus1100, prim_controller)+offs,
            size, data);
    return res;
}
/*****************************************************************************/
int
lvd_c_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data)
{
    int res;
    res=dev->read(dev, ofs(struct lvd_bus1100, prim_controller)+offs,
            size, data);
    return res;
}
/*****************************************************************************/
#if 0
int
lvd_cb_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data)
{
    int res;
    res=dev->write(dev, ofs(struct lvd_bus1100, controller_bc)+offs, size, data);
    return res;
}
#endif
/*****************************************************************************/
#if 0
int
lvd_cb_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data)
{
    int res;
    res=dev->read(dev, ofs(struct lvd_bus1100, controller_bc)+offs, size, data);
    return res;
}
#endif
/*****************************************************************************/
int
lvd_i_w_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32 data)
{
    int res;
    res=dev->write(dev,
            ofs(struct lvd_bus1100, in_card[addr&0xf])+offs, size, data);
    return res;
}
/*****************************************************************************/
int
lvd_i_wblind_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32 data)
{
    int res;
    res=dev->write_blind(dev,
            ofs(struct lvd_bus1100, in_card[addr&0xf])+offs, size, data);
    return res;
}
/*****************************************************************************/
int
lvd_i_r_(struct lvd_dev* dev, int addr, int offs, int size, ems_u32* data)
{
    int res;
    res=dev->read(dev,
        ofs(struct lvd_bus1100, in_card[addr&0xf])+offs, size, data);
    return res;
}
/*****************************************************************************/
int
lvd_ib_w_(struct lvd_dev* dev, int offs, int size, ems_u32 data)
{
    int res;
    res=dev->write(dev, ofs(struct lvd_bus1100, in_card_bc)+offs, size, data);
    return res;
}
/*****************************************************************************/
int
lvd_ib_r_(struct lvd_dev* dev, int offs, int size, ems_u32* data)
{
    int res;
    res=dev->read(dev, ofs(struct lvd_bus1100, in_card_bc)+offs, size, data);
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
