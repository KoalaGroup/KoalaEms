/*
 * c219_help.c
 * 
 * created 05.12.94 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: c219_help.c,v 1.5 2011/04/06 20:30:30 wuestner Exp $";

#include <config.h>
#include <stdio.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"

#include "../../../lowlevel/camac/camac.h"
#include "c219_help.h"

RCS_REGISTER(cvsid, "procs/camac/drams")

/*
  !! Statische Variablen verhindern natuerlich reentrance, aber wir werden das
  auch nicht brauchen
  slot: CAMAC-Slot von C219-Modul
  tkanal: Kanal, ueber den der (pseudo)Trigger erzeugt wird
  fkanal: Kanal, an dem das Flat-Top-Signal anliegt
*/
static struct camac_dev* dev;
static int slot, tkanal, fkanal;

/*****************************************************************************/
static
plerrcode c219_done_old(void)
{
    ems_u32 stat;
    dev->CAMACcntl(dev, dev->CAMACaddrP(slot, 0, 9), &stat);
    return plOK;
}
/*****************************************************************************/
static
plerrcode c219_done(void)
{
    /* warum soll man alles zuruecksetzen? */
    return plOK;
}
/*****************************************************************************/
static
int c219_get_old(void)
{
    /* kann erst hier gemacht werden, da DRAMS dauernd CCCC verwenden */
    /* output, positive, normal, transparent */
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, tkanal, 17), 6);
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 0, 16), 1<<tkanal);
    return 1;
}
/*****************************************************************************/
static
int c219_get(void)
{
    ems_u32 val;
    dev->CAMACread(dev, dev->CAMACaddrP(slot, 0, 0), &val);
    val=dev->CAMACval(val)|(1<<tkanal);
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 0, 16), val);
    return 1;
}
/*****************************************************************************/
static
void c219_reset_old(void)
{
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 0, 16), 0);
}
/*****************************************************************************/
static
void c219_reset(void)
{
    ems_u32 val;
    dev->CAMACread(dev, dev->CAMACaddrP(slot, 0, 0), &val);
    val=dev->CAMACval(val)&~(1<<tkanal);
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 0, 16), val);
}
/*****************************************************************************/
static
int c219_check(int wait)
{
    ems_u32 val;
    dev->CAMACread(dev, dev->CAMACaddrP(slot, 0, 0), &val);
    val=dev->CAMACval(val);
    if (val&(1<<fkanal)) {
        D(D_USER, printf("kein flat top\n");)
        if (wait) {
            int x[3];
            x[0]=17;
            x[1]=1;
            x[2]=0;
            D(D_USER, printf("werde warten\n");)
            send_unsolicited(Unsol_Patience, x, 3);
            do {
                dev->CAMACread(dev, dev->CAMACaddrP(slot, 0, 0), &val);
                val=dev->CAMACval(val);
            } while (val&(1<<fkanal));
            D(D_USER, printf("flat top\n");)
            x[2]=1;
            send_unsolicited(Unsol_Patience, x, 3);
            return 1;
        }
        return 0;
    } else {
        return 1;
    }
}
/*****************************************************************************/
plerrcode
c219_init(ml_entry* module, int argc, int* argv,
    int (**get_proc)(void),
    void (**reset_proc)(void),
    plerrcode (**done_proc)(void),
    int (**check_proc)(int))
{
/*
  argc==0: Kanal fuer Triggeroutput =0
  argc==1: Kanal fuer Triggeroutput =argv[0]
  argc==3: Kanal fuer Flat-Top-input=argv[1]
           Polaritaet von Flat-Top  =argv[2]
*/
    int check_flat, fpol=0;
    ems_u32 val;
    D(D_USER, printf("c219_init:\n");)
    switch (argc) {
    case 0:
        tkanal=0;
        check_flat=0;
        break;
    case 1:
        tkanal=argv[0];
        check_flat=0;
        break;
    case 3:
        tkanal=argv[0];
        fkanal=argv[1];
        fpol=argv[2];
        check_flat=1;
        break;
    default:
        return plErr_ArgNum;
        break;
    }

    dev=module->address.camac.dev;
    slot=module->address.camac.slot;

    if ((unsigned int)slot>30)
        return plErr_BadHWAddr;
    if (module->modultype!=CAEN_PROG_IO)
        return plErr_BadModTyp;
    if ((unsigned int)tkanal>15)
        return plErr_ArgRange ;
    if (check_flat && ((unsigned int)fkanal>15))
        return plErr_ArgRange;
    if (check_flat && ((unsigned int)fpol>1))
        return plErr_ArgRange;

    /* mal sehen, ob CAMAC clear "totgemacht wurde" */
    dev->CAMACwrite(dev, dev->CAMACaddrP(slot, tkanal, 17), 6);
    dev->CAMACread(dev, dev->CAMACaddrP(slot, tkanal, 1), &val);
    val=dev->CAMACval(val);
    if (val!=6) {
        printf("schlimmer Fehler in c219_init\n");
        return(plErr_HW);
    }
    dev->CCCC(dev);
    dev->CAMACread(dev, dev->CAMACaddrP(slot, tkanal, 1), &val);
    val=dev->CAMACval(val);
    switch (val) {
    case 6: /* Clear ist gekappt */
        D(D_USER, printf("CCCC wirkt nicht\n");)
        if (argc==0) {
        /* Fehlende Argumente (Watzlawik) ergaenzen */
            /*
            Kanal 0: Trigger
            Kanal 1: Flat_top
            Kanal 12: 1, wenn check_flat gesetzt werden soll
            Kanal 13: 1, wenn Kanal 1 andere Polaritaet haben soll
            Kanal 14, 15: Ausgang, wird auf 1 gesetzt
            */
            ems_u32 val1;
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 15, 17), 6);
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 14, 17), 6);
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 13, 17), 7);
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 12, 17), 7);
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, 0, 16), 0xc000);
            dev->CAMACread(dev, dev->CAMACaddrP(slot, 0, 0), &val1);
            val1=dev->CAMACval(val1);
            check_flat=((val1&0x2000)!=0);
            fpol=((val1&0x1000)!=0);
            D(D_USER, printf("val=0x%x; check_flat=%d; fpol=%d\n",
                val1, check_flat, fpol);)
            if (check_flat) fkanal=1;
        }
        *get_proc=c219_get;
        *reset_proc=c219_reset;
        *done_proc=c219_done;
        dev->CAMACwrite(dev, dev->CAMACaddrP(slot, tkanal, 17), 6);
        /* output, positive, normal, transparent */
        if (check_flat) {
            *check_proc=c219_check;
            dev->CAMACwrite(dev, dev->CAMACaddrP(slot, fkanal, 17), 5|(fpol?2:0));
            /* input, positive|negative, normal, transparent */
        } else {
            *check_proc=0;
        }
        c219_reset();
        break;
    case 7:
        D(D_USER, printf("CCCC wirkt\n");)
        if (check_flat) { /* das passt nicht zusammen */
            D(D_USER, printf("Clear nicht gekappt aber check_flat gesetzt.\n");)
            return(plErr_HW);
        }
        *get_proc=c219_get_old;
        *reset_proc=c219_reset_old;
        *done_proc=c219_done_old;
        *check_proc=0;
        c219_done_old();
        break;
    default:
        D(D_USER, printf("CCCC wirkt falsch\n");)
        return(plErr_HW);
        break;
    }
    return(plOK);
}
/*****************************************************************************/
/*****************************************************************************/
