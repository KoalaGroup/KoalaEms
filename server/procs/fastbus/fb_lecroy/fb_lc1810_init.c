/*
 * procs/fastbus/fb_lecroy/fb_lc1810_init.c
 * created 29.12.94 MiZi/PeWue/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1810_init.c,v 1.16 2017/10/09 21:25:37 wuestner Exp $";

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
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "fb_lc_util.h"
#include "fb_lc1810_init.h"

#define SETUP_PAR  3                    /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_setup[]="fb_lc1810_setup";
int ver_proc_fb_lc1810_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1810_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_setup()
{
return(&fb_lc1810_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy CAT 1810
========================

parameters:
-----------
  p[ 1] : module idx

module specific parameters:
---------------------------
  p[ 2]: (DSR# 4< 0: 5>) FCW                       0..63- (n * 1024) + 100 ns
  p[ 3]: (DSR# 2< 0: 7>) TDC1879 frequency ref.    6..131- 1 / (1.6e-11*(256-n)*"tdc")
                                                      "tdc"= 2^(csr#0<1>)
  p[ 4]: (CSR# 0< 1>)    test selection:           0- no test
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_setup(ems_u32* p)
{
    ml_entry* module;
    struct fastbus_dev* dev;
    ems_u32 pa,sa,val,r_val;
    ems_u32 ssr;                         /* slave status response             */

    T(proc_fb_lc1810_setup)

    module=ModulEnt(p[1]);
    dev=module->address.fastbus.dev;
    pa=module->address.fastbus.pa;
    p++;

    sa= 4;                              /******           dsr#4          ******/
    val= p[1];                          /* fast clear window                  */
    dev->FWDa(dev, pa, sa, val, &ssr);  /* primary address cycle              */
    if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }
    dev->FCRW(dev, &r_val, &ssr);  r_val&=0x3f;
    if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=1;
        return(plErr_HW);
    }
    if (r_val^val) {
        dev->FCDISC(dev);
        *outptr++=pa; *outptr++=sa; *outptr++=r_val; *outptr++=2;
        return(plErr_HW);
     }

    sa= 2;                              /******           dsr#2          ******/
    val= p[2];                          /* tdc1879 frequency reference        */
    dev->FCWWS(dev, sa, val, &ssr);
    if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=3;
        return(plErr_HW);
    }
    dev->FCRW(dev, &r_val, &ssr); r_val&=0xff;
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=4;
        return(plErr_HW);
    }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=5;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    val= p[3];                          /* test selection                     */
    dev->FWCa(dev, pa, sa, val, &ssr);       /* primary address cycle              */
    if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=6;
        return(plErr_HW);
    }
    dev->FCRW(dev, &r_val, &ssr); r_val&=0x7;
    dev->FCDISC(dev);                        /* disconnect                         */
    if (ssr) {
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=7;
        return(plErr_HW);
    }
    if (r_val) {
        *outptr++=pa; *outptr++=sa; *outptr++=r_val; *outptr++=8;
        return(plErr_HW);
    }

    dev->FWCM(dev, 0x8d, 0, &ssr);           /* clear module                       */
    if (ssr) {
        *outptr++=pa; *outptr++=ssr; *outptr++=9;
        return(plErr_HW);
    }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    dev->FRC(dev, pa,sa, &r_val, &ssr); r_val&=0x38;
    if (ssr) {
        *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=10;
        return(plErr_HW);
    }
    if (r_val) {                        /* error bits not cleared             */
        *outptr++=pa; *outptr++=sa; *outptr++=r_val; *outptr++=11;
        return(plErr_HW);
    }

    D(D_USER, printf("CAT 1810(%2d): setup done\n",pa);)

    return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_setup(ems_u32* p)       
  /* test subroutine for "proc_fb_lc1810_setup" */
{
    ml_entry* module;
    int pa, i; 
    int msk_p[3]={0x3f,0xff,0x0};

    T(test_proc_fb_lc1810_setup)

    if (p[0]!=SETUP_PAR+1) {            /* check number of parameters         */
        *outptr++=0;
        return(plErr_ArgNum);
    }

    if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    pa=module->address.fastbus.pa;

    if (module->modultype!=LC_CAT_1810) {
        printf("test_proc_fb_lc1810_setup: invalid module 0x%08x\n",
                module->modultype);
        return plErr_BadISModList;
    }

    p+=2;
    for (i=0; i<SETUP_PAR; i++) {         /* check parameter ranges           */
        if (((unsigned int)p[i] & msk_p[i]) != (unsigned int)p[i]) {
            *outptr++=pa; *outptr++=i+1; *outptr++=0;
            return(plErr_ArgRange);
        }
    }
    if (((unsigned int)p[1]<6) || ((unsigned int)p[1]>131)) {
        *outptr++=pa; *outptr++=2; *outptr++=1;
        return(plErr_ArgRange);
    }

    wirbrauchen= 4;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

plerrcode lc1810_check_reset(struct fastbus_dev* dev, ems_u32 pa, int evt)
 /* check for multiple occurence of gate signals and reset lecroy cat1810 */
{
    ems_u32 err_flag;                   /* error flag bit pattern             */
    ems_u32 ssr;

T(lc1810_check_reset)

    /* check multiple gate occurence */
    dev->FRC(dev, pa,1, &err_flag, &ssr); err_flag&=0x38;
    if (ssr) {
        *outptr++=pa; *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }
    if (err_flag) {
/*
        int unsol_mes[6], unsol_num;
        unsol_num=5; unsol_mes[0]=5; unsol_mes[1]=3;
        unsol_mes[2]=evt; unsol_mes[3]=pa; unsol_mes[4]=err_flag;
        send_unsolicited(Unsol_Warning, unsol_mes, unsol_num);
*/
        send_unsol_warning(5, 3, evt, pa, err_flag);
    }

    dev->FWCM(dev, 0x8d, 0, &ssr);           /* clear module                       */
    if (ssr) {
        *outptr++=ssr; *outptr++=1;
        return(plErr_HW);
    }

    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check_start[]="fb_lc1810_check";
int ver_proc_fb_lc1810_check_start=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1810_check_start_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check_start()
{
return(&fb_lc1810_check_start_prop);
}
#endif
/******************************************************************************/
/*
Multiple Trigger Test of LeCroy CAT 1810
========================================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : module (index in memberlist)
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_check_start(ems_u32* p)
  /* check on multiple gate occurence for fastbus lecroy cat1810              */
  /* r/w pointer positions in multi--event modules are checked and corrected  */
  /*   at start of readout */
{
    ml_entry* module;
    ems_u32 ssr;
    ems_u32 err_flag;                   /* error flag bit pattern             */
    int pat;                            /* IS module pattern                  */
    plerrcode buf_check;
    struct fastbus_dev* dev;

T(proc_fb_lc1810_check_start)

    module=ModulEnt(p[1]);
    dev=module->address.fastbus.dev;

    /* check multiple gate occurence */
    dev->FRC(dev, module->address.fastbus.pa, 1, &err_flag, &ssr);
    err_flag&=0x38;
    if (ssr) {
        *outptr++=p[1]; *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }
    if (err_flag) {                     /* multiple gate occured              */
        if (trigger.eventcnt==1) {/* check and correct buffer ptrs at start ro */
            {
/*
            int unsol_mes[6], unsol_num;
            unsol_num=5; unsol_mes[0]=5; unsol_mes[1]=3;
            unsol_mes[2]=trigger.eventcnt; unsol_mes[3]=p[1]; unsol_mes[4]=err_flag;
            send_unsolicited(Unsol_Warning, unsol_mes, unsol_num);
*/
            send_unsol_warning(5, 3, trigger.eventcnt, p[1], err_flag);
            }
            if ((pat=lc_pat(LC_TDC_1876, "lc1810_check_start"))) {/* check r/w ptr for tdc1876        */
                if ((buf_check=lc1876_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                    *outptr++=LC_TDC_1876; *outptr++=1;
                    return(buf_check);
                }
            }
            if ((pat=lc_pat(LC_TDC_1877, "lc1810_check_start"))) {/* check r/w ptr for tdc1877        */
                if ((buf_check=lc1877_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                    *outptr++=LC_TDC_1877; *outptr++=2;
                    return(buf_check);
                }
            }
            if ((pat=lc_pat(LC_ADC_1881, "lc1810_check_start"))) {/* check r/w ptr for adc1881        */
                if ((buf_check=lc1881_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                    *outptr++=LC_ADC_1881; *outptr++=3;
                    return(buf_check);
                }
            }
            if ((pat=lc_pat(LC_ADC_1881M, "lc1810_check_start"))) {/* check r/w ptr for adc1881m      */
                if ((buf_check=lc1881_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                    *outptr++=LC_ADC_1881M; *outptr++=4;
                    return(buf_check);
                }
            }
        } else {                        /* multiple gate error                */
#if 1
            char s[1024];
            snprintf(s, 1024, "event %d error flag 0x%x", trigger.eventcnt, err_flag);
            send_unsol_text("multiple gate error", s, 0);
#else
            *outptr++=p[1]; *outptr++=err_flag; *outptr++=5;
            return(plErr_HW);
#endif
        }
    }

    return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check_start(ems_u32* p)
  /* test subroutine for "proc_fb_lc1810_check_start" */
{
    ml_entry* module;

    T(test_proc_fb_lc1810_check_start)

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if (p[0]!=1) {                      /* check number of parameters         */
        return(plErr_ArgNum);
    }
                                        /* check parameter values             */
    if ((unsigned int)p[1]>memberlist[0]) {
        *outptr++=p[1];
        return(plErr_ArgRange);
    }

    module=ModulEnt(p[1]);
    if ((module->modulclass!=modul_fastbus)||(module->modultype!=LC_CAT_1810)) {
        *outptr++=p[1];
        return(plErr_BadModTyp);
    }

    wirbrauchen= 5;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check[]="fb_lc1810_check";
int ver_proc_fb_lc1810_check=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1810_check_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check()
{
return(&fb_lc1810_check_prop);
}
#endif
/******************************************************************************/
/*
Multiple Trigger Test of LeCroy CAT 1810
========================================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : module (index in memberlist)
*/
/*****************************************************************************/

plerrcode proc_fb_lc1810_check(ems_u32* p)      
  /* check on multiple gate occurence for fastbus lecroy cat1810              */
  /* r/w pointer positions in multi--event modules are checked and corrected  */
{
    ml_entry* module;
    ems_u32 ssr;
    ems_u32 err_flag;                   /* error flag bit pattern             */
    int pat;                            /* IS module pattern                  */
    plerrcode buf_check;
    struct fastbus_dev* dev;

T(proc_fb_lc1810_check)

    module=ModulEnt(p[1]);
    dev=module->address.fastbus.dev;

    /* check multiple gate occurence */
    dev->FRC(dev, module->address.fastbus.pa, 1, &err_flag, &ssr);
    err_flag&=0x38;
    if (ssr) {
        *outptr++=p[1]; *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }
    if (err_flag) {                     /* multiple gate occured              */
        {
/*
            int unsol_mes[6],unsol_num;
            unsol_num=5;unsol_mes[0]=5;unsol_mes[1]=3;
            unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=p[1];unsol_mes[4]=err_flag;
            send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
*/
        send_unsol_warning(5, 3, trigger.eventcnt, p[1], err_flag);
        }
        if ((pat=lc_pat(LC_TDC_1876, "lc1810_check"))) {  /* check r/w ptr for tdc1876          */
            if ((buf_check=lc1876_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                *outptr++=LC_TDC_1876; *outptr++=1;
                return(buf_check);
            }
        }
        if ((pat=lc_pat(LC_TDC_1877, "lc1810_check"))) {  /* check r/w ptr for tdc1877          */
            if ((buf_check=lc1877_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                *outptr++=LC_TDC_1877; *outptr++=2;
                return(buf_check);
            }
        }
        if ((pat=lc_pat(LC_ADC_1881, "lc1810_check"))) {  /* check r/w ptr for adc1881          */
            if ((buf_check=lc1881_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                *outptr++=LC_ADC_1881; *outptr++=3;
                return(buf_check);
            }
        }
        if ((pat=lc_pat(LC_ADC_1881M, "lc1810_check"))) { /* check r/w ptr for adc1881          */
            if ((buf_check=lc1881_buf_converted(dev, pat, 2, trigger.eventcnt))!=plOK) {
                *outptr++=LC_ADC_1881; *outptr++=4;
                return(buf_check);
            }
        }
    }

    return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check(ems_u32* p)
  /* test subroutine for "proc_fb_lc1810_check" */
{
    ml_entry* module;

    T(test_proc_fb_lc1810_check)

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if (p[0]!=1) {                      /* check number of parameters         */
        return(plErr_ArgNum);
    }
                                        /* check parameter values             */
    if ((unsigned int)p[1]>memberlist[0]) {
        *outptr++=p[1];
        return(plErr_ArgRange);
    }

    module=ModulEnt(p[1]);
    if ((module->modulclass!=modul_fastbus)||(module->modultype!=LC_CAT_1810)) {
        *outptr++=p[1];
        return(plErr_BadModTyp);
    }

    wirbrauchen= 5;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check_reset[]="fb_lc1810_reset";
int ver_proc_fb_lc1810_check_reset=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1810_check_reset_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check_reset()
{
return(&fb_lc1810_check_reset_prop);
}
#endif
/******************************************************************************/
/*
Reset of LeCroy CAT 1810
========================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : module (index in memberlist)
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_check_reset(ems_u32* p)
  /* reset of fastbus lecroy cat1810 and check on multiple gate occurence     */
{
    ml_entry* module;
    ems_u32 ssr;
    struct fastbus_dev* dev;
    ems_u32 err_flag;                   /* error flag bit pattern             */

T(proc_fb_lc1810_check_reset)

    module=ModulEnt(p[1]);
    dev=module->address.fastbus.dev;

    /* check multiple gate occurence */
    dev->FRC(dev, module->address.fastbus.pa, 1, &err_flag, &ssr);
    err_flag&=0x38;
    if (ssr) {
        *outptr++=p[1]; *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }
    if (err_flag) {                     /* multiple gate occured              */
        *outptr++=p[1]; *outptr++=err_flag; *outptr++=1;
        return(plErr_HW);
    }

    dev->FWCM(dev, 0x8d, 0, &ssr); /* clear module         */
    if (ssr) {
        *outptr++=ssr; *outptr++=2;
        return(plErr_HW);
    }

    return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check_reset(ems_u32* p)
  /* test subroutine for "proc_fb_lc1810_check_reset" */
{
    ml_entry* module;

    T(test_proc_fb_lc1810_check_reset)

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if (p[0]!=1) {                      /* check number of parameters         */
        return(plErr_ArgNum);
    }
                                        /* check parameter values             */
    if ((unsigned int)p[1]>memberlist[0]) {
        *outptr++=p[1];
        return(plErr_ArgRange);
    }

    module=ModulEnt(p[1]);
    if ((module->modulclass!=modul_fastbus)||(module->modultype!=LC_CAT_1810)) {
        *outptr++=p[1];
        return(plErr_BadModTyp);
    }

    wirbrauchen= 3;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_reset[]="fb_lc1810_reset";
int ver_proc_fb_lc1810_reset=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1810_reset_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_reset()
{
return(&fb_lc1810_reset_prop);
}
#endif
/******************************************************************************/
/*
Reset of LeCroy CAT 1810
========================
parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : module (index in memberlist)
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_reset(ems_u32* p)      
{
    ml_entry* module;
    ems_u32 ssr;

    T(proc_fb_lc1810_reset)

    module=ModulEnt(p[1]);
    /* clear module */
    module->address.fastbus.dev->FWCM(module->address.fastbus.dev,
            0x8d, 0, &ssr);
    if (ssr) {
        *outptr++=ssr; *outptr++=0;
        return(plErr_HW);
    }

    return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_reset(ems_u32* p)       
  /* test subroutine for "proc_fb_lc1810_reset" */
{
    ml_entry* module;

    T(test_proc_fb_lc1810_reset)

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if (p[0]!=1) {                      /* check number of parameters         */
        return(plErr_ArgNum);
    }
                                        /* check parameter values             */
    if ((unsigned int)p[1]>memberlist[0]) {
        *outptr++=p[1];
        return(plErr_ArgRange);
    }

    module=ModulEnt(p[1]);
    if ((module->modulclass!=modul_fastbus)||(module->modultype!=LC_CAT_1810)) {
        *outptr++=p[1];
        return(plErr_BadModTyp);
    }

    wirbrauchen=2;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/
