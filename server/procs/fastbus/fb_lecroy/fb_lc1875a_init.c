/*
 * procs/fastbus/fb_lecroy/fb_lc1875a_init.c
 * 
 * 28.12.94 MaWo PeWue MiZi
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1875a_init.c,v 1.13 2017/10/09 21:25:37 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit+16)))

#define SETUP_PAR  6                    /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
static void
dump_proc_args(const char *name, ems_u32 *p)
{
    int i;

    printf("%u args for procedure %s\n", p[0], name);
    for (i=1; i<=p[0]; i++) {
        printf("0x%x ", p[i]);
    }
    printf("\n");
}
/******************************************************************************/

char name_proc_fb_lc1875a_setup[]="fb_lc1875a_setup";
int ver_proc_fb_lc1875a_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875a_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1875a_setup()
{
return(&fb_lc1875a_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy TDC 1875A
========================

parameters:
-----------
  p[ 1] : number of tdc1875a modules

module specific parameters:
---------------------------
  p[ 2]:                 operation mode:           0- F mode, 1- N mode
  p[ 3]: (CSR# 0<24| 8>) common start source:      0- front panel, 1- backplane
  p[ 4]: (CSR# 0< 9|25>) FCW source:               0- internal, 1- backplane
  p[ 5]: (CSR# 1<24:30>) active channel depth:     1..64- range of active channels
                                                      0..(n-1)
  p[ 6]: (CSR# 0<28|12>) autorange:                0- disable, 1- enable
  p[ 7]: (CSR# 0<29|13>) range:                    0- low, 1- high
         (ignored if parameter p[ 6]=1)
*/
/******************************************************************************/

plerrcode proc_fb_lc1875a_setup(ems_u32* p)      
  /* setup of fastbus lecroy tdc1875a */
{
    ems_u32 sa, val, r_val, csr_val;
    ems_u32 csr_mask;
    int i;

    T(proc_fb_lc1875a_setup)

    dump_proc_args("fb_lc1875a_setup", p);
    p++;
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=modullist->entry+memberlist[i];
        if (module->modultype==LC_TDC_1875A) {
            struct fastbus_dev* dev=module->address.fastbus.dev;
            int pa=module->address.fastbus.pa;
            ems_u32 ssr;

            D(D_USER,printf("TDC 1875A: pa= %d\n",pa);)
            printf("lc1875a_setup; pa=%d\n", pa);

            sa=0;
            dev->FWCa(dev, pa, sa, 1<<27, &ssr); /* clear */
            if (ssr) {
                dev->FCDISC(dev);
                printf("lc1875a_setup: pa=%d sa=%d FWCa: ss=%d\n", pa, sa, ssr);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=0;
                return plErr_HW;
            }
            tsleep(2);
            dev->FCWW(dev, 1<<30, &ssr); /* reset */
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=1;
                return plErr_HW;
            }

            /* test of operation mode */
            dev->FCWW(dev, 1<<11, &ssr); /* COM */
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=0; *outptr++=ssr; *outptr++=2;
                return plErr_HW;
            }
            tsleep(2);

            sa=1;
            /* check conversion event counter */
            dev->FCRWS(dev, sa, &val, &ssr);
            val&=0xfff;
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=3;
                return plErr_HW;
            }
            switch (val) {
            case 0x000:
                D(D_USER, printf("TDC 1875A(%2d): N mode\n",pa);)
                if (!p[1]) {
                    dev->FCDISC(dev);
                    *outptr++=pa; *outptr++=sa; *outptr++=4;
                    return plErr_HWTestFailed;
                }
                break;
            case 0x100:
                D(D_USER, printf("TDC 1875A(%2d): F mode\n",pa);)
                if (p[1]) {
                    dev->FCDISC(dev);
                    *outptr++=pa; *outptr++=sa; *outptr++=5;
                    return plErr_HWTestFailed;
                }
                break;
            default:
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=6;
                return plErr_HW;
                break;
            }

            sa=0;
            dev->FCWWS(dev, sa, 1<<27, &ssr); /* clear */
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=7;
                return plErr_HW;
            }
            tsleep(2);
            dev->FCWW(dev, 1<<30, &ssr); /* reset */
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=8;
                return plErr_HW;
            }

            val=set_ctrl_bit(1,6) |      /* disable wait */
                set_ctrl_bit(0,10) |     /* disable test pulser */
                set_ctrl_bit(p[2],8) |   /* common start source */
                set_ctrl_bit(1-p[3],9) | /* FCW source */
                set_ctrl_bit(p[5],12);   /* autorange */
            csr_mask= 0x1740;
            if (!p[5]) { /* autorange disabled */
                val= val | set_ctrl_bit(p[6],13);  /* range */
                csr_mask= csr_mask | 0x2000;
            }
            csr_val= (val^set_ctrl_bit(1,6))&csr_mask; /* wait status */
            dev->FCWW(dev, val, &ssr);
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=9;
                return plErr_HW;
            }
            dev->FCRW(dev, &r_val, &ssr);
            r_val&=csr_mask;
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=10;
                return plErr_HW ;
            }
            if (r_val^csr_val) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=r_val; *outptr++=11;
                return plErr_HW;
            }

            sa=1;
            dev->FCRWS(dev, sa, &val, &ssr);
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=12;
                return plErr_HW;
            }
            r_val=(val&0x80ffffff)|(p[4]<<24);
            if (r_val!=(p[4]<<24)) {
                printf("lc1875a: csr1=%08x rval=%08x p[4]=%d\n", val, r_val, p[4]);
                send_unsol_warning(6, 3, pa, sa, r_val);
            }
            dev->FCWW(dev, val, &ssr);
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa;*outptr++=sa; *outptr++=ssr; *outptr++=13;
                return(plErr_HW);
            }
            dev->FCRW(dev, &r_val, &ssr);
            dev->FCDISC(dev);
            if (ssr) {
                *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=14;
                return(plErr_HW);
            }
            if (r_val^val) {
                *outptr++=pa; *outptr++=sa; *outptr++=r_val; *outptr++=15;
                return(plErr_HW);
            }

            D(D_USER, printf("TDC 1875A(%2d): setup done\n",pa);)
            p+= SETUP_PAR;
        }
    }

    return plOK;
}
/******************************************************************************/
plerrcode
test_proc_fb_lc1875a_setup(ems_u32* p)       
{
    int n_p, unused;
    int i, i_p;
    int msk_p[6]={0x1, 0x1, 0x1, 0x7f, 0x1, 0x1};

    T(test_proc_fb_lc1875a_setup)

    if (memberlist==0) {
        return(plErr_NoISModulList);
    }
    if (memberlist[0]==0) {
        return(plErr_BadISModList);
    }

    unused=p[0];
    D(D_USER,printf("TDC 1875A: number of parameters for all modules= %d\n",
            unused);)

    unused-=1;
    n_p=1;
    if (unused%SETUP_PAR) { /* check number of parameters */
        *outptr++=0;
        return plErr_ArgNum;
    }
    if ((unused/SETUP_PAR)!=p[1]) {
        *outptr++=1;
        return plErr_ArgNum;
    }

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=modullist->entry+memberlist[i];
        if (module->modultype==LC_TDC_1875A) {
            D(D_USER, printf("TDC 1875A(%2d): "
                    "number of parameters not used= %d\n",i,unused);)
            if (unused<SETUP_PAR) {
                /* not enough parameters for modules defined in IS*/
                *outptr++=i;
                *outptr++=unused;
                return plErr_ArgNum;
            }
            if (p[n_p+5]) /* autorange enabled */
                msk_p[5]= 0xffffffff;
            /* check parameter ranges */
            for (i_p=1; i_p<=SETUP_PAR; i_p++) {
                if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=
                        (unsigned int)p[n_p+i_p]) {
                    *outptr++=i; *outptr++=i_p; *outptr++=0;
                    return plErr_ArgRange;
                }
            }
            /* add. check for edge detection */
            if (((unsigned int)p[n_p+4]<1) || ((unsigned int)p[n_p+4]>64)) {
                *outptr++=i; *outptr++=4; *outptr++=1;
                return(plErr_ArgRange);
            }
            n_p+=SETUP_PAR;
            unused-=SETUP_PAR;
        }
    }

    if (unused) { /* unused parameters left */
        *outptr++=2;
        return plErr_ArgNum;
    }

    wirbrauchen= 4;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
