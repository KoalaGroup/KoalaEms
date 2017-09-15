/*
 * procs/unixvme/vme_verify.c
 * created 06.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_verify.c,v 1.20 2011/08/13 20:22:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../lowlevel/devices.h"
#include "vme_verify.h"

enum ident_type {id_none, id_faf5, id_configrom, id_sis, id_stop};
enum ident_ind {oui_ind, ver_ind, board_ind, rev_ind, ser_ind, last_ind};

RCS_REGISTER(cvsid, "procs/unixvme")

struct ident_faf5 {
    ems_u32 am;
    ems_u32 offs;
    ems_u32 manu_code;
    ems_u32 type_code;
};
struct ident_configrom {
    ems_u32 am;
    ems_u32 offs;
    ems_u32 oui;
    ems_u32 board;
};

struct ident_sis {
    ems_u32 am;
    ems_u32 offs;
    ems_u32 ident;
};
union ident_descr {
    struct ident_faf5 faf5;
    struct ident_configrom configrom;
    struct ident_sis sis;
};
struct vme_info {
    int ems_type;
    char* name;
    char* descr;
    enum ident_type id_type;
    union ident_descr idd;
    ems_u32 address_bits; /* size of used address space */
};

struct vme_info vme_types[]={
    {CAEN_V259, "V259", "multihit pattern/NIM",
        id_faf5,
        {.faf5={0x39, 0xfa, 0x2, 0x3}},
        8
    },
    {CAEN_V259E, "V259E", "multihit pattern/ECL",
        id_faf5,
        {.faf5={0x39, 0xfa, 0x2, 0x4}},
        8
    },
    {CAEN_V262, "V262", "IO register",
        id_faf5,
        {.faf5={0x39, 0xfa, 0x2, 0x1}},
        8
    },
    {CAEN_V265, "V265", "8-channel charge integrating ADC",
        id_faf5,
        {.faf5={0x39, 0xfa, 0x2, 0x12}},
        8
    },
    {CAEN_V462, "V462", "Gate Generator",
        id_faf5,
        {.faf5={0x39, 0xfa, 0x2, 0xa}},
        8
    },
    {CAEN_V512, "V512", "Logic",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x1a}},
        8
    },
    {CAEN_V550, "V550", "C-RAMS",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x34}},
        16
    },
    {CAEN_V551B, "V551B", "C-RAMS Sequencer",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x3c}},
        16
    },
    {CAEN_V556, "V556", "8-channel peak sensing ADC",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x36}},
        8
    },
    {CAEN_V560, "V560", "counter",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x18}},
        16
    },
    {CAEN_V673, "V673", "128 channel KLOE TDC",
        id_faf5,
        {.faf5={0x9, 0x1fa, 0x2, 0x12c}},
        16
    },
    {CAEN_V693, "V693", "Multihit TDC",
        id_faf5,
        {.faf5={0x9, 0x1fa, 0x2, 0x12e}},
    },
    {CAEN_V729A, "V729A", "4 channel 40 MHz flash ADC",
        id_faf5,
        {.faf5={0x9, 0xfa, 0x2, 0x48}},
        16
    },
    {CAEN_V767, "V767", "128 channel TDC",
        id_configrom,
        {.configrom={0x9, 0x1000, 0x0040e6, 0x2ff}},
        16
    },
    {CAEN_V775, "V775", "tdc",
        id_configrom,
        {.configrom={0x9, 0x8000, 0x0040e6, 0x307}},
        16
    },
    {CAEN_V785, "V785", "Peak Sensing ADC",
        id_configrom,
        {.configrom={0x9, 0x8000, 0x0040e6, 0x311}},
        16
    },
    {mesytec_madc32, "madc32", "Peak Sensing ADC",
        id_configrom,
        {.configrom={0x0, 0x8000, 0xd, 0xd}},
        16
    },
 //   {CAEN_V792, "V792", "qdc",
 //       id_configrom,
 //       {.configrom={0x9, 0x8000, 0x0040e6, 0x318}},
 //       16
 //   },
    {CAEN_V792, "V792", "qdc", /* geographical addresses */
        id_configrom,
        {.configrom={0x2f, 0x8000, 0x0040e6, 0x318}},
        16
    },
    {CAEN_V820, "V820", "32 channel latching scaler",
        id_configrom,
        {.configrom={0x9, 0x8000, 0x0040e6, 0x334}},
        16
    },
    {CAEN_V830, "V830", "32 channel latching scaler block mode",
        id_configrom,
        {.configrom={0x9, 0x8000, 0x0040e6, 0x33e}},
        16
    },
    {CAEN_V1190, "V1190", "128 channel multihit TDC",
        id_configrom,
        {.configrom={0x9, 0x4000, 0x0040e6, 0x0004a6}},
        16
    },
    {CAEN_V1290, "V1290", "32 channel multihit TDC",
        id_configrom,
        {.configrom={0x9, 0x4000, 0x0040e6, 0x050a}},
        16
    },
    {CAEN_VN1488, "VN1488", "tdc",
        id_faf5,
        {.faf5={0x09, 0xfa, 0x2, 0x37}},
        16
    },
    {SIS_3300, "SIS3300", "8 channel 100 MHz flash ADC",
        id_sis,
        {.sis={0x09, 0x4, 0x3300}},
        16
    },
    {SIS_3400, "SIS3400", "time stamper",
        id_sis,
        {.sis={0x09, 0x4, 0x3400}},
        16
    },
    {SIS_3600, "SIS3600", "multi event latch",
        id_sis,
        {.sis={0x09, 0x4, 0x3600}},
        16
    },
    {SIS_3800, "SIS3800", "scaler/counter",
        id_sis,
        {.sis={0x09, 0x4, 0x3800}},
        16
    },
    {SIS_3801, "SIS3801", "multiscaler/counter",
        id_sis,
        {.sis={0x09, 0x4, 0x3801}},
        16
    },
    {0, 0, 0, id_stop} 
};
/*****************************************************************************/
static int
read_ident_faf5(struct vme_dev* dev, int am, ems_u32 addr, ems_u16* ident)
{
    int res;
    if ((am&0x30) && (addr&0xff000000)) return -1; /* addr too large for A24 */
    res=dev->read(dev, addr, am, (ems_u32*)ident, 6, 2, 0);
    if (res!=6) {
/*
        printf("read_ident_faf5: addr=%x res=%d\n", addr, res);
*/
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
verify_faf5(struct vme_dev* dev, ems_u32 addr, struct vme_info* info,
        int verbose)
{
    ems_u16 v[3];
    int ver, ser, man, mod;
    struct ident_faf5* ident=&info->idd.faf5;

    if (verbose) printf("verify_faf5: checking %s addr=0x%08x\n",
            info->name, addr);
    if (read_ident_faf5(dev, ident->am, addr+ident->offs, v)<0) {
        printf("verify_faf5 %s: read failed\n", info->name);
        return plErr_HWTestFailed;
    }

    if (v[0]!=0xfaf5) {
        printf("verify_faf5 %s: magic=0x%04x\n", info->name, v[0]);
        return plErr_HWTestFailed;
    }
    ver=(v[2]>>12)&0xf;
    ser=v[2]&0xfff;
    man=(v[1]>>10)&0x3f;
    mod=v[1]&0x3ff;

    if (man!=ident->manu_code) {
        printf("verify_faf5 %s: manufacturer=0x%x (0x%x expected)\n",
            info->name, man, ident->manu_code);
        return plErr_HWTestFailed;
    }
    if (mod!=ident->type_code) {
        printf("verify_faf5 %s: type=0x%x (0x%x expected)\n",
            info->name, mod, ident->type_code);
        return plErr_HWTestFailed;
    }
    if (verbose) {
        printf("found %s at 0x%x; version %d, serial %d\n",
                info->name, addr,
                ver, ser);
    }
    return plOK;
}

/*************** new configrom  **********************************************/
/* here ident_32[] contains: 
 *    0: oui
 *    1: ver
 *    2: board
 *    3: rev
 *    4: ser
 */
static int
read_ident_configrom1(struct vme_dev* dev, int am, ems_u32 addr, ems_u32* ident_32)
{
    int res, i;
    ems_u8  ident[10];
    ems_u32 off[10]={
            0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e, 0x4e, 0xf02, 0xf06};
    ems_u16 val;
  //  printf("vme_verify: read_ident_configrom1()!\n");
    if ((am&0x30) && (addr&0xff000000)) return -1; /* addr too large for A24 */
  //  printf("vme_verify: read_ident_configrom1(), doesn't return -1\n");
    for (i=0; i<10; i++) {
        res=dev->read(dev, addr+off[i], am, (ems_u32*)&val, 2, 2, 0);
//printf("vme_verify: read_ident_configrom1(): check res value!\n");
        if (res!=2) return -1;
//printf("vme_verify: read_ident_configrom1(): res=2!\n");
        ident[i]=val&0xff;
    }
    ident_32[oui_ind]   = (ident[0]<<16)|(ident[1]<<8)|ident[2];
    ident_32[ver_ind]   = ident[3]&0xff; 
    ident_32[board_ind] = (ident[4]<<16)|(ident[5]<<8)|ident[6];
    ident_32[rev_ind]   = ident[7];
    ident_32[ser_ind]   = (ident[8]<<8)|ident[9];
//printf("vme_verify: read_ident_configrom1(): return 0!\n");
    return 0;
}
/*****************************************************************************/
/* for CAEN_1290 and like it */
static int
read_ident_configrom2(struct vme_dev* dev, int am, ems_u32 addr, ems_u32* ident_32)
{
    int res, i;
    ems_u8  ident[13];
    ems_u32 off[13]={
            0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x40, 0x44, 0x048, 0x4c, 0x80, 0x84};
    ems_u16 val;
//printf("vme_verify: read_ident_configrom2()!\n");
    if ((am&0x30) && (addr&0xff000000)) return -1; /* addr too large for A24 */
//printf("vme_verify: read_ident_configrom2(), return -1!\n");
    for (i=0; i<13; i++) {
        res=dev->read(dev, addr+off[i], am, (ems_u32*)&val, 2, 2, 0);
        if (res!=2) return -1;
        ident[i]=val&0xff;
    }
    ident_32[oui_ind]   = (ident[0]<<16)|(ident[1]<<8)|ident[2];
    ident_32[ver_ind]   = ident[3];
    ident_32[board_ind] = (ident[4]<<16)|(ident[5]<<8)|ident[6];
    ident_32[rev_ind]   = (ident[7]<<24)|(ident[8]<<16)|(ident[9]<<8)|(ident[10]);
    ident_32[ser_ind]   = (ident[11]<<8)|ident[12];

    return 0;
}
/*****************************************************************************/
static struct vme_info*
find_vme_info_configrom(int id)
{
    struct vme_info* types=vme_types;
    for (types=vme_types; types->id_type!=id_stop; types++) {
        if ((types->id_type==id_configrom) && (types->idd.configrom.board==id))
            return types;
    }
    return 0;
}
/*****************************************************************************/
static void
print_ident_configrom(struct vme_dev* dev, int am, ems_u32 addr,
        ems_u32* ident_32, struct vme_info* info)
{
    ems_u32 vendor=ident_32[oui_ind];
    ems_u32 id=ident_32[board_ind];
    printf("0x%08x/%02x: ", addr, am);
    printf("vendor 0x%x, id 0x%x,", vendor, id);
    if (info)
        printf(" %s,", info->name);
    printf(" revision %d, version %d, ser. %d\n",
            ident_32 [rev_ind], ident_32[ver_ind], ident_32[ser_ind]);
    if (vendor==0x40e6 && id==0x311) { /* CAEN V785 */
        ems_u32 geo, status1;
        int amnesia, slot;
        dev->read(dev, addr+0x1002, am, &geo, 2, 2, 0);
        dev->read(dev, addr+0x100e, am, &status1, 2, 2, 0);
        amnesia=!!(status1&0x10);
        slot=(geo&0x1f);
        if (0&amnesia)
            printf("    AMNESIA\n");
        else
            printf("    slot %d   geo=0x%04x status1=0x%04x\n", slot, geo, status1);
    }
}
/*****************************************************************************/
static plerrcode
verify_configrom(struct vme_dev* dev, ems_u32 addr, struct vme_info* info,
        int verbose)
{ printf("verify_configrom!\n");
    ems_u32 ident_32[last_ind];
    struct ident_configrom* ident=&info->idd.configrom;
    if (verbose) {
        printf("verify_configrom: checking %s addr=0x%08x\n", info->name, addr);
    }
printf("verify_configrom1!\n");
    if (read_ident_configrom1(dev, ident->am, addr+ident->offs, ident_32)<0) {
printf("verify_configrom2!\n");
      if(read_ident_configrom2(dev, ident->am, addr+ident->offs, ident_32)<0) {
printf("verify_configrom3!\n");
        if (verbose) {
            printf("read_ident_configrom failed (am=0x%x)\n", ident->am);
        }
     printf("HW test Failed!\n");
        return plErr_HWTestFailed;
      }
    }
  printf("vme_verify: verify_configrom(): ident->oui is %oxd\n",ident->oui);
  printf("vme_verify: verify_configrom(): ident_32[oui_ind] is %oxd\n",ident_32[oui_ind]);
printf("vme_verify: verify_configrom(): ident_32[ver_ind] is %oxd\n",ident_32[ver_ind]);
printf("vme_verify: verify_configrom(): ident_32[board_ind] is %oxd\n",ident_32[board_ind]);
printf("vme_verify: verify_configrom(): ident_32[rev_ind] is %oxd\n",ident_32[rev_ind]);
    if (ident_32[oui_ind]!=ident->oui) {
      if (verbose) {
        printf("oui mismatch\n");
        print_ident_configrom(dev, ident->am, addr, ident_32, 0);
      }
printf("vme_verify: verify_configrom(): oui mismatch, so HW test Failed!\n");
      return plErr_HWTestFailed;
    }
    if (ident_32[board_ind] != ident->board) {
      if (verbose) {
	printf("board mismatch\n");
	print_ident_configrom(dev, ident->am, addr, ident_32, 0);
      }
printf("HWTest Failed2!\n");
      return plErr_HWTestFailed;
    }
    if (verbose) {
        printf("found %s at 0x%x; revision %d version %d, serial %d\n",
            info->name, addr, ident_32[rev_ind], ident_32[ver_ind], ident_32[ser_ind]);
    }
printf("HWTest successful!\n");
    return plOK;
}
/*****************************************************************************/
static int
read_ident_sis(struct vme_dev* dev, int am, ems_u32 addr, ems_u32* ident)
{
    int res;
    if ((am&0x30) && (addr&0xff000000)) return -1; /* addr too large for A24 */

    res=dev->read(dev, addr, am, ident, 4, 4, 0);
    if (res!=4) return -1;
    return 0;
}
/*****************************************************************************/
static plerrcode
verify_sis(struct vme_dev* dev, ems_u32 addr, struct vme_info* info,
        int verbose)
{
    ems_u32 v;
    struct ident_sis* ident=&info->idd.sis;
    int board;

    if (verbose)
        printf("verify_sis: checking %s addr=0x%08x\n", info->name, addr);
    if (read_ident_sis(dev, 0x9, addr+4, &v)<0) {
        printf("verify_sis %s: read failed\n", info->name);
        return plErr_HWTestFailed;
    }
    /* ver=(v>>12)&0xf; */
    board=(v>>16)&0xffff;

    if (board!=ident->ident) {
        printf("verify_sis(%s 0x%08x): found 0x%08x\n", info->name, addr, v);
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
#if 0
static struct vme_info*
find_vme_info(int modultype)
{
    struct vme_info* types=vme_types;
    while (types->ems_typ && types->ems_typ!=modultype) types++;
    return types->ems_typ==modultype?types:0;
}
#endif
/*****************************************************************************/
static plerrcode
verify_vme(struct vme_dev* dev, ems_u32 addr, struct vme_info* info,
        int verbose)
{
/*
 * return:
 *     0: not verified but not fatal
 *    -1: fatal error (crate offline ...)
 *     1: verified
 */
    if (!dev->generic.online) {
        printf("verify_vme: crate offline\n");
        return plErr_HWTestFailed;
    }
printf("verify_vme\n");
    switch (info->id_type) {
        case id_none:
            printf("verify_vme: id_none\n");
            return plErr_HWTestFailed;
        case id_faf5:
            return verify_faf5(dev, addr, info, verbose);
        case id_configrom:
printf("verify_vme: id_configrom\n");
            return verify_configrom(dev, addr, info, verbose);
        case id_sis:
            return verify_sis(dev, addr, info, verbose);
        default:
            printf("verify_vme: id_type %d unknown\n", info->id_type);
            return plErr_HWTestFailed;
    }
}
/*****************************************************************************/
#if 0
plerrcode verify_vme_id(struct vme_dev* dev, ems_u32 addr, ems_u32 modultype)
{
    struct vme_info* info=find_vme_info(modultype);
    if (!info) return plErr_Program;

    return verify_vme(dev, addr, info);
}
#else
plerrcode verify_vme_id(struct vme_dev* dev, ems_u32 addr, ems_u32 modultype,
        int verbose)
{
printf("verify_vme_id\n");
    struct vme_info* info=vme_types;
    plerrcode pres;
    int n=0;

    do {
        if (info->ems_type==modultype) {
            pres=verify_vme(dev, addr, info, verbose);
            if (pres==plOK)
                return plOK;
            n++;
	}
        info++;
    } while (info->ems_type);
    return n?plErr_HWTestFailed:plErr_Program;
}
#endif
/*****************************************************************************/
plerrcode verify_vme_module(ml_entry* module, int verbose)
{
    return verify_vme_id(module->address.vme.dev, module->address.vme.addr,
            module->modultype, verbose);
}
/*****************************************************************************/
/*
 * checking whether all modulues of IS have the correct type
 */
plerrcode test_proc_vme(int* list, ems_u32* module_types)
{
    int i, res;
    plerrcode pres;
    if (!list) return plErr_NoISModulList;
printf("test_proc_vme: OK1\n");

    for (i=list[0]; i>0; i--) {
        ml_entry* module=&modullist->entry[list[i]];
        int j;
printf("modullist i is: %d\n",i);
printf("test_proc_vme: OK2\n");

        if (module->modulclass!=modul_vme) {
            printf("test_proc_vme[%d]: not vme\n", i);
            return plErr_BadModTyp;
        }
        res=-1;
printf("test_proc_vme: OK3! res = %d\n", res);

        for (j=0; module_types[j] && res; j++) {
printf("j is :%d\n", j);
printf("module->modultype is %d\n",module->modultype);
printf("module_type[j] is %d\n",module_types[j]);
            if (module->modultype==module_types[j]) res=0;
printf("res is : %d\n", res);
        }
printf("test_proc_vme: OK4, res is : %d\n", res);
        if (res) {
            printf("test_proc_vme[%d]: type in modullist is %08x\n",
                    i, module->modultype);
            printf("looked for");
            for (j=0; module_types[j]; j++) {
                printf(" %08x", module_types[j]);
            }
            printf("\n");
            return plErr_BadModTyp;
        }
printf("test_proc_vme: OK5\n");
        pres=verify_vme_module(module, 1);
printf("test_proc_vme: OK6\n");
printf("test_proc_vme: pres=%d\n",pres);

        if (pres!=plOK)
//printf("test_proc_vme: OK7\n");
            return pres;
//printf("test_proc_vme: OK72\n");
    }
//printf("test_proc_vme: OK8\n");
    return plOK;
}
/*****************************************************************************/
/*
 * checking whether one modulue has the correct type
 */
plerrcode test_proc_vmemodule(ml_entry* module, ems_u32* module_types)
{
    int j, res;
    plerrcode pres;

#if 0
    printf("test_proc_vmemodule:\n");
    dump_modent(module, 0);
#endif

    if (module->modulclass!=modul_vme) {
        printf("test_proc_vmemodule: not vme\n");
        return plErr_BadModTyp;
    }
    res=-1;
    for (j=0; module_types[j] && res; j++) {
        if (module->modultype==module_types[j])
            res=0;
    }
    if (res) {
        printf("test_proc_vmemodule: type in modullist is %08x\n",
                module->modultype);
        printf("looked for");
        for (j=0; module_types[j]; j++) {
            printf(" %08x", module_types[j]);
        }
        printf("\n");
        return plErr_BadModTyp;
    }

    pres=verify_vme_module(module, 0);
    if (pres!=plOK)
        return pres;

    return plOK;
}
/*****************************************************************************/
static struct vme_info*
find_vme_info_faf5(int id)
{
    struct vme_info* types=vme_types;
    for (types=vme_types; types->id_type!=id_stop; types++) {
        if ((types->id_type==id_faf5) && (types->idd.faf5.type_code==id))
            return types;
    }
    return 0;
}
/*****************************************************************************/
static void
print_ident_faf5(int am, ems_u32 addr, ems_u16* ident, struct vme_info* info)
{
    int ver, ser, man, mod;
    if (ident[0]!=0xfaf5) printf("faf5-->%04x\n", ident[0]);
    /*
    printf("ident_faf5: addr=%08x [0]=%04x [1]=%04x [2]=%04x\n",
            addr, ident[0], ident[1], ident[2]);
    */
    ver=(ident[2]>>12)&0xf;
    ser=ident[2]&0xfff;
    man=(ident[1]>>10)&0x3f;
    mod=ident[1]&0x3ff;
    printf("0x%08x/%02x: ", addr, am);
    if (info)
        printf("%s", info->name);
    else
        printf("vendor 0x%x id 0x%x", man, mod);
    printf(" version %d ser. %d\n", ver, ser);
}

static int
scan_vme_module_faf5(struct vme_dev* dev, ems_u32 addr,
        ems_u32* ems_type, int* serial)
{
    int i, j, am_list[2]={0x9, 0x39};
    ems_u32 offs_list[2]={0xfa, 0x1fa};
    ems_u16 ident[3];
    for (i=0; i<2; i++)
        for (j=0; j<2; j++) {
            if (read_ident_faf5(dev, am_list[i], addr+offs_list[j], ident)==0) {
                struct vme_info* info;
                int mod;
#if 0
                printf("scan_vme_module_faf5: addr=%x am=%x offs=%x\n",
                        addr, am_list[i], offs_list[j]);
#endif
                mod=ident[1]&0x3ff;
                info=find_vme_info_faf5(mod);
                print_ident_faf5(am_list[i], addr, ident, info);
                if (info && ems_type)
                    *ems_type=info->ems_type;
                if (serial)
                    *serial=ident[2]&0xfff;
                return 1;
            }
        }
    return 0;
}
/*****************************************************************************/
static int
scan_vme_module_configrom(struct vme_dev* dev, ems_u32 addr,
        ems_u32* ems_type, int* serial)
{
  int i;
  ems_u32 offs_list[3]={0x1000, 0x8000, 0x4000};
  ems_u32 ident[last_ind];

  /*printf("scan_vme_module_configrom\n");*/
  for (i=0; i<2; i++)
    if (read_ident_configrom1(dev, 0x9, addr+offs_list[i], ident)==0) {
      struct vme_info* info;
      info=find_vme_info_configrom(ident[board_ind]);
      print_ident_configrom(dev, 0x9, addr, ident, info);
      if (info && ems_type)
	*ems_type=info->ems_type;
      if (serial)
	*serial=ident[ser_ind];
      return 1;
    }
  if (read_ident_configrom2(dev, 0x9, addr+offs_list[2], ident)==0) {
    struct vme_info* info;
    info=find_vme_info_configrom(ident[board_ind]);
    print_ident_configrom(dev, 0x9, addr, ident, info);
    if (info && ems_type)
      *ems_type=info->ems_type;
    if (serial)
      *serial=ident[ser_ind];
    return 1;
  }
  
  return 0;
}
/*****************************************************************************/
/*
static struct vme_info*
find_vme_info_sis(int id)
{
    struct vme_info* types=vme_types;
    for (types=vme_types; types->id_type!=id_stop; types++) {
        if ((types->id_type==id_sis) && (types->idd.sis.ident==id))
            return types;
    }
    return 0;
}
*/
/*
static void
print_ident_sis(int am, ems_u32 addr, ems_u32 ident)
{
    struct vme_info* info;
    int ver, board;

    ver=(ident>>12)&0xf;
    board=(ident>>16)&0xffff;

    info=find_vme_info_sis(board);
    if (!info) return;
    printf("0x%08x/%02x: ", addr, am);
    if (info)
        printf("%s", info->name);
    else
        printf("id 0x%x", board);
    printf(" version %d\n", ver);
}
*/
/*****************************************************************************/
/*
static int
scan_vme_module_sis(struct vme_dev* dev, ems_u32 addr, ems_u32* outptr)
{
    ems_u32 ident;
    if (read_ident_sis(dev, 0x9, addr+4, &ident)==0) {
        print_ident_sis(0x9, addr, ident);
        return 1;
    }
    return 0;
}
*/
static int
scan_vme_module(struct vme_dev* dev, ems_u32 addr,
        ems_u32* ems_type, int* serial)
{
    if (scan_vme_module_faf5(dev, addr, ems_type, serial)>0)
        return 1;
    if (scan_vme_module_configrom(dev, addr, ems_type, serial)>0)
        return 1;
#if 0
    if (scan_vme_module_sis(dev, addr, ems_type, serial)>0)
        return 1;
#endif
    return 0;
}
/*****************************************************************************/
int
scan_vme_crate(struct vme_dev* dev, ems_u32 addr, ems_u32 incr, int num,
    ems_u32* outptr)
{
    ems_u32 adr;
    int i, berr, n=0;

    berr=dev->berrtimer(dev, 5000);

    for (adr=addr, i=0; (adr>=addr)&&(i<num); adr+=incr, i++) {
        int res, serial=-1;
        ems_u32 ems_type=-1;
        res=scan_vme_module(dev, adr, &ems_type, &serial);
        if (res>0) {
            n++;
            if (outptr) {
                *outptr++=ems_type;
                *outptr++=serial;
                *outptr++=adr;
            }
        }
    }
    if (berr>=0)
        dev->berrtimer(dev, berr);
    return n;
}
/*****************************************************************************/
ems_u32 vme_module_id(struct vme_dev* dev, ems_u32 addr)
{
    struct vme_info* info=vme_types;
    int res;

    while (info->id_type!=id_stop) {
        res=verify_vme(dev, addr, info, 0);
        if (res) {
            if (res>0) {
                printf("vme_module_id: found %s at 0x%08x\n",
                    info->name, addr);
                return info->ems_type;
            }
            return 0;
        }
        info++;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
