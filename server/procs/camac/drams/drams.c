/*
 * drams.c
 * 
 * created before 18.05.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: drams.c,v 1.14 2011/04/06 20:30:30 wuestner Exp $";

#include <math.h>
#include <stdlib.h>
#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include <xdrstring.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/is/is.h"
#include "../../../objects/is/isvar.h"
#include "../../../trigger/camac/gsi/gsitrigger.h"
#include "../../../trigger/camac/gsi/gsitrigmod.h"
#include "c219_help.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procprops.h"
#include "../../../main/timers.h"
#include "../../procs.h"

extern ems_u32* outptr;
extern int* memberlist;
extern ISV* isvar;

struct channel_pedestals {
    int pedestal;
    int threshold;
} channel_pedestals;

typedef struct channel_pedestals modul_pedestals[1024];

RCS_REGISTER(cvsid, "procs/camac/drams")

/*****************************************************************************/
static int to;
static void timeout(callbackdata arg)
{
    to=1;
}
/*****************************************************************************/
/*
struktur in is_var:
0: Modulindex des Controllers
1: Anzahl der Receiver
2: Modulindex des ersten Receivers
...
m: Modulindex des letzten Receivers
m+1: Anzahl der Hilfsmudule
m+2: Modulindex des ersten Hilfsmoduls
n: Modulindex des letzten Hilfsmoduls
*/

#define M_control (modullist->entry+is_var[0])
#define num_receiver is_var[1]
#define M_receiver(x) (modullist->entry+is_var[2+(x)])
#define num_auxmod is_var[2+num_receiver]
#define M_auxmod(x) (modullist->entry+is_var[3+num_receiver+(x)])

static plerrcode
test_is_drams(void)
{
    int i, modnum , anz_control, anz_event, anz_receiver, anz_aux, size;
    int i_rec, i_aux;
    plerrcode res;

    T(test_is_drams)
    anz_control=anz_event=anz_receiver=anz_aux=0;
    if (memberlist==(int*)0)
        return(plErr_NoISModulList);
    if ((modnum=memberlist[0])==0)
        return(plErr_BadISModList);
    for (i=1; i<=modnum; i++) {
        switch(ModulEnt_m(i)->modultype) {
            case DRAMS_RECEIVER: anz_receiver++; break;
            case DRAMS_CONTROL:  anz_control++; break;
            case DRAMS_EVENT:    anz_event++; break;
            default: anz_aux++; break;
        }
    }
    if (anz_receiver==0) return(plErr_BadISModList);
    if (anz_control!=1) return(plErr_BadISModList);
    if (anz_event>1) return(plErr_BadISModList);
    size=3+anz_receiver+anz_aux;
    if ((res=reallocisvar(size))!=OK)
        return(res);
    is_var[1]=anz_receiver;
    is_var[2+anz_receiver]=anz_aux;
    for (i=1, i_rec=0, i_aux=0; i<=modnum; i++) {
        switch (ModulEnt_m(i)->modultype) {
        case DRAMS_RECEIVER:
            is_var[2+i_rec++]=memberlist[i];
            break;
        case DRAMS_CONTROL:
            is_var[0]=memberlist[i];
            break;
        case DRAMS_EVENT:
            break;
        default:
            is_var[3+anz_receiver+i_aux++]=memberlist[i];
            break;
        }
    }
    return plOK;
}
/*****************************************************************************/
static void
reset_address(void)
{
    struct camac_dev* dev=M_control->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(M_control->address.camac.slot, 0, 24);
    ems_u32 stat;

    dev->CAMACcntl(dev, &addr, &stat);
}
/*****************************************************************************/
static void prepare(void)
{
    struct camac_dev* dev=M_control->address.camac.dev;
    camadr_t addr;
    ems_u32 stat;

    D(D_USER, printf("prepare()\n");)

    /* reset busy */
    addr=dev->CAMACaddr(M_control->address.camac.slot, 0, 11);
    dev->CAMACcntl(dev, &addr, &stat);

    /* CAMAC clear */
    dev->CCCC(dev);

    /* measurement mode */
    addr=dev->CAMACaddr(M_control->address.camac.slot, 0, 16);
    dev->CAMACwrite(dev, &addr, 1);
}
/*****************************************************************************/
typedef struct pedinf {
    int sum;
    int qsum;
} pedinf;

static plerrcode
measure_pedestal(pedinf** array, int nr_of_loops, int delay,
        int argc, int* argv)
{
    int typen[]={CAEN_PROG_IO, GSI_TRIGGER, 0};
    int T_aux=0, N_cont;
    ml_entry* M_aux, *M_cont;
    struct camac_dev* dev;
    ems_u32 stat;
    int n, i, j;
    int (*get_proc)(void);
    void (*reset_proc)(void);
    plerrcode (*done_proc)(void);
    int (*check_proc)(int);

    int nummod, numchan;
    int loop, base, x;
    int alm;
    int val, val1, val2;
    int feld[512];
    camadr_t cadr_read, cadr_res_busy, cadr_test, cadr_res_addr;
    pedinf *arr;
    int delinf;
    timeoutresc toresc;
    callbackdata cb;

    delinf=1000/delay;

    nummod=num_receiver;
    numchan=nummod*1024;

    /* Modul fuer Triggererzeugung suchen: */
    M_aux=0;
    j=0;
    while (!M_aux && (typen[j]!=0)) {
        for (i=num_auxmod-1; i>=0; i--) {
            if (M_auxmod(i)->modultype==typen[j]) {
                M_aux=M_auxmod(i);
                T_aux=typen[j];
            }
        }
        j++;
    }
    if (!M_aux) {
        printf("drams.c: measure_pedestal: no auxilary module found\n");
        return plErr_Other;
    }

    /* Trigger initialisieren */
    switch (T_aux) {
#if 0
    case GSI_TRIGGER: {
        plerrcode res;
        if ((res=trigmod_soft_init(M_aux, 1, 30, 10))!=plOK)
            return res;
        get_proc=trigmod_soft_get;
        reset_proc=trigmod_soft_reset;
        done_proc=trigmod_soft_done;
        check_proc=0;
        }
        break;
#endif
    case CAEN_PROG_IO: {
        plerrcode res;
        res=c219_init(M_aux, argc>3?3:argc, argv, &get_proc, &reset_proc,
                &done_proc, &check_proc);
        if (res!=plOK)
            return res;
        }
        break;
    default:
         printf("drams.c: measure_pedestal: program error");
         return plErr_Program;
    }

    /* DRAMS-Controller */
    M_cont=M_control;
    dev=M_cont->address.camac.dev;
    N_cont=M_cont->address.camac.slot;

    /* Adressen fuer schnellen Zugriff */
    cadr_res_busy=dev->CAMACaddr(N_cont, 0, 11);
    cadr_res_addr=dev->CAMACaddr(N_cont, 0, 24);
    cadr_test=dev->CAMACaddr(N_cont, 0, 0);

    /* Speicher belegen */
    arr=(pedinf*)calloc(numchan, sizeof(pedinf));
    if (arr==0) {
        *outptr++=numchan*sizeof(pedinf);
        return plErr_NoMem;
    }
    *array=arr;

    /* pedestal-mode einschalten */
    dev->CAMACwrite(dev, dev->CAMACaddrP(N_cont, 0, 16), 0);

    /* reset busy flipflop */
    dev->CAMACcntl(dev, &cadr_res_busy, &stat);

    dev->CCCC(dev);
    for (loop=0; loop<nr_of_loops; loop++) {
        if (loop && (loop%delinf==0)) {
            int x[3];
            x[0]=17;
            x[1]=0;
            x[2]=nr_of_loops-loop;
            send_unsolicited(Unsol_Patience, x, 3);
            /*send_unsol_patience(17, 2, 0, nr_of_loops-loop);*/
        }
        tsleep(delay);
        if (check_proc) check_proc(1);
        if (get_proc()==0) {
            printf("measure_pedestal: Fehler in get_proc()\n");
            free(arr);
            return plErr_Other;
        }
        to=0;
        cb.p=(void*)0;
        alm=install_timeout(200*TIMER_RES, timeout, cb, "DRAMS timeout", &toresc);
        if (alm==-1) {
            *outptr++=errno;
            free(arr);
            return plErr_Other;
        }
        /* Warten auf busy, Abbruch bei timeout */
        while (!to) {
            ems_u32 val;
            dev->CAMACread(dev, &cadr_test, &val);
            if ((dev->CAMACval(val)&2))
                break;
        }
        if (to) {
            *outptr++=1;
            *outptr++=loop;
            free(arr);
            return plErr_HW;
        }
        remove_timer(&toresc);
        reset_proc();
        /* Warten auf Ende der Conversion */
        x=0;
        while (++x<2000) {
            ems_u32 val;
            dev->CAMACread(dev, &cadr_test, &val);
            if (!(dev->CAMACval(val)&1))
                break;
        }
        if (x>=2000) {
            free(arr);
            *outptr++=2;
            *outptr++=loop;
            return(plErr_HW);
        }

        if (check_proc) {
            if (check_proc(0)==0) {
                D(D_USER, printf("muss daten vernichten\n");)
                loop--;
                /* reset address counter */
                dev->CAMACcntl(dev, &cadr_res_addr, &stat);
                for (n=0, base=0; n<nummod; n++, base+=1024) {
                    /* DRAMS-Receiver */
                    cadr_read=dev->CAMACaddr(M_receiver(n)->address.camac.slot, 0, 0);

                    /* auslesen und vergessen */
                    for (i=0; i<512; i++)
                        dev->CAMACread(dev, &cadr_read, &feld[i]);
                }
            }
        }

        /* reset address counter */
        dev->CAMACcntl(dev, &cadr_res_addr, &stat);

        for (n=0, base=0; n<nummod; n++, base+=1024) {
            /* DRAMS-Receiver */
            cadr_read=dev->CAMACaddr(M_receiver(n)->address.camac.slot, 0, 0);

            /* auslesen und addieren */
    /*      CAMACblread(feld, cadr_read, 512);*/
            for (i=0; i<512; i++)
                dev->CAMACread(dev, &cadr_read, &feld[i]);
            for (i=0; i<512; i++) {
                val=feld[i];
                DV(D_USER, if (i<50) printf("%3d: val=0x%08x, %3d, %3d\n", i, val,
                        val&0xff, (val>>8)&0xff);)
                val1=val&0xff;
                arr[base+i].sum+=val1; arr[base+i].qsum+=(val1*val1);
                val2=(val>>8)&0xff;
                arr[base+512+i].sum+=val2; arr[base+512+i].qsum+=(val2*val2);
            }
        }

        /* reset busy flipflop und clear fuer neachsten Zyklus */
        dev->CCCC(dev);
        dev->CAMACcntl(dev, &cadr_res_busy, &stat);
    /*  reset_proc();*/
    }
    done_proc();

/* nachfolgende Messung vorbereiten */
    prepare();
    return plOK;
}
/*****************************************************************************/
/*
DRAMS_Pedestal
p[0] : number of arguments (>=2)
p[1] : number of loops
p[2] : delay in 1/100s
[p[3] : Output-Kanal fuer CAEN C219]
*/

plerrcode
proc_DRAMS_Pedestal(ems_u32* p)
{
    plerrcode res;
    pedinf* arr;
    int nummod, numloop, numchan, i;
    float average, sigma, loops, sum;

    T(proc_DRAMS_Pedestal)

    if ((res=measure_pedestal(&arr, p[1], p[2], p[0]-2, p+3))!=plOK)
        return res;

    nummod=num_receiver;
    numloop=p[1];
    numchan=nummod*1024;

    /* Auswertung */
    loops=(float)numloop;
    *outptr++=nummod;
    for (i=0; i<numchan; i++) {
        sum=(float)arr[i].sum;
        average=sum/loops;
        *(float*)(outptr++)=average;
        sigma=sqrt((float)arr[i].qsum/loops-average*average);
        *(float*)(outptr++)=sigma;
        if ((i>=1140)&&(i<=1151)) {
            printf("%04d: %.3f %.3f\n", i, average, sigma);
        }
    }

    free(arr);
    return plOK;
}

plerrcode
test_proc_DRAMS_Pedestal(ems_u32* p)
{
    plerrcode res;
    int i;

    T(test_proc_DRAMS_Pedestal)
    D(D_USER, printf("test:DRAMS_Pedestal()\n");)
    if (p[0]<2) return(plErr_ArgNum);
    if (p[1]<1) return(plErr_ArgRange);
    if ((unsigned int)p[2]>100) return(plErr_ArgRange);
    if ((res=test_is_drams())!=plOK) return(res);
    if (num_auxmod<1) return(plErr_BadISModList);
    for (i=0; i<num_auxmod; i++) {
        switch (M_auxmod(i)->modultype) {
        /*case GSI_TRIGGER: return(plOK);*/
        case CAEN_PROG_IO: return(plOK);
        }
    }
    return plErr_BadISModList;
}
#ifdef PROCPROPS
static procprop DRAMS_Pedestal_prop={
    1, -1, "int loops, int delay",
    "Misst Pedestals der koehlerschen DRAMS-Module \n"
    "und gibt sie als Funktionswerte zurueck."};

procprop* prop_proc_DRAMS_Pedestal()
{
    return &DRAMS_Pedestal_prop;
}
#endif
char name_proc_DRAMS_Pedestal[]="DRAMS_Pedestal";
int ver_proc_DRAMS_Pedestal=1;
/*****************************************************************************/
/*
DRAMS_mess_ped
p[0] : number of arguments (>=3)
p[1] : variable
p[2] : number of loops
p[3] : delay in 1/100s
[p[4] : Output-Kanal fuer CAEN C219]
*/

plerrcode
proc_DRAMS_mess_ped(ems_u32* p)
{
    plerrcode res;
    pedinf* arr;
    int nummod, numloop, numchan, i;
    float loops;

    typedef struct {
        int pedestal;  /* *1000 */
        int sigma;     /* *1000 */
    } pedestals;
    pedestals *pedestalvar;

    T(proc_mess_ped)
    D(D_USER, printf("DRAMS_mess_ped()\n");)

    nummod=num_receiver;
    numloop=p[2];
    numchan=nummod*1024;

    /* ems-variable vorbereiten */
    var_delete(p[1]);
    if ((res=var_create(p[1], numchan*2))!=plOK) return(res);
    pedestalvar=(pedestals*)var_list[p[1]].var.ptr;

    if ((res=measure_pedestal(&arr, p[2], p[3], p[0]-3, p+4))!=plOK)
        return(res);

    /* Auswertung */
    loops=(float)numloop;
    for (i=0; i<numchan; i++) {
        float sum, qsum, average, sigma;
        sum=(float)arr[i].sum;
        qsum=(float)arr[i].qsum;
        average=sum/loops;
        pedestalvar[i].pedestal=(int)(average*1000+0.5);
        sigma=sqrt(qsum/loops-average*average);
        pedestalvar[i].sigma=(int)(sigma*1000+0.5);
        if ((i>=1140)&&(i<=1151)) {
            printf("%04d: %d %d\n", i, pedestalvar[i].pedestal, pedestalvar[i].sigma);
            printf("      %.3f %.3f\n", average, sigma);
        }
    }

    free(arr);
    return(plOK);
}

plerrcode
test_proc_DRAMS_mess_ped(ems_u32* p)
{
    plerrcode res;
    int i;

    T(test_proc_DRAMS_mess_ped)
    D(D_USER, printf("test:DRAMS_mess_ped()\n");)
    if (p[0]<3) return(plErr_ArgNum);
    if (p[1]>MAX_VAR) return(plErr_IllVar);
    if ((unsigned int)p[3]>100) return(plErr_ArgRange);
    if ((res=test_is_drams())!=plOK) return(res);
    if (num_auxmod<1) return(plErr_BadISModList);
    for (i=0; i<num_auxmod; i++) {
        switch (M_auxmod(i)->modultype) {
        /*case GSI_TRIGGER: return(plOK);*/
        case CAEN_PROG_IO: return(plOK);
        }
    }
    return(plErr_BadISModList);
}
#ifdef PROCPROPS
static procprop DRAMS_mess_ped_prop={1, -1,
    "int var, int loops, int delay[, int kanal]",
    "Misst Pedestals der koehlerschen DRAMS-Module "
    "und schreibt sie in die Variable var."};

procprop* prop_proc_DRAMS_mess_ped()
{
    return(&DRAMS_mess_ped_prop);
}
#endif
char name_proc_DRAMS_mess_ped[]="DRAMS_mess_ped";
int ver_proc_DRAMS_mess_ped=1;
/*****************************************************************************/
/*
DRAMS_set_ped
p[0] : number of arguments (==2)
p[1] : variable
p[2] : factor (*1000)
*/

plerrcode proc_DRAMS_set_ped(ems_u32* p)
{
    int nummod, numchan;
    int base, n;
    struct camac_dev* dev;
    ml_entry* M_cont;
    int N_cont, val, i;
    camadr_t cadr_res_addr, cadr_write;
    float fact;
    ems_u32 stat;
    typedef struct {
        int thresh;
        int ped;
    } pedinf;
    pedinf *arr;
    typedef struct {
        int pedestal;
        int sigma;
    } pedestals;
    pedestals *pedestalvar;

    nummod=num_receiver;
    numchan=nummod*1024;

    /* ems-variable vorbereiten */
    if (var_list[p[1]].len!=numchan*2)
        return(plErr_IllVarSize);
    pedestalvar=(pedestals*)var_list[p[1]].var.ptr;

    /* DRAMS-Controller */
    M_cont=M_control;
    dev=M_cont->address.camac.dev;
    N_cont=M_cont->address.camac.slot;

    /* Adressen fuer schnellen Zugriff */
    cadr_res_addr=dev->CAMACaddr(N_cont, 0, 24);

    /* Speicher belegen */
    arr=(pedinf*)calloc(numchan, sizeof(pedinf));
    if (arr==0) {
        *outptr++=numchan*sizeof(pedinf);
        return(plErr_NoMem);
    }

    /* Auswertung */
    fact=(float)p[2]/1000;
    for (i=0; i<numchan; i++) {
        float fthresh, pedestal, sigma;

        pedestal=(float)pedestalvar[i].pedestal/1000;
        sigma=(float)pedestalvar[i].sigma/1000;
        arr[i].ped=(int)(pedestal+0.5);
        fthresh=pedestal+fact*sigma;
        arr[i].thresh=(int)(fthresh+0.5);
        if (arr[i].thresh>255) arr[i].thresh=255;
    }

    dev->CAMACcntl(dev, &cadr_res_addr, &stat);

    for (n=0, base=0; n<nummod; n++, base+=1024) {
        cadr_write=dev->CAMACaddr(M_receiver(n)->address.camac.slot, 0, 16);
        for (i=0; i<512; i++) {
            val=arr[base+i].ped | arr[base+i+512].ped<<8;
            dev->CAMACwrite(dev, &cadr_write, val);
        }
        cadr_write=dev->CAMACaddr(M_receiver(n)->address.camac.slot, 0, 17);
        for (i=0; i<512; i++) {
            val=arr[base+i].thresh | arr[base+i+512].thresh<<8;
            dev->CAMACwrite(dev, &cadr_write, val);
        }
    }

    free(arr);
    return(plOK);
}

plerrcode
test_proc_DRAMS_set_ped(ems_u32* p)
{
    plerrcode res;

    T(test_proc_DRAMS_set_ped)
    D(D_USER, printf("test:DRAMS_set_ped()\n");)
    if (p[0]!=2) return(plErr_ArgNum);
    if (p[2]<0) return(plErr_ArgRange);
    if (p[1]>MAX_VAR) return(plErr_IllVar);
    if (!var_list[p[1]].len) return(plErr_NoVar);
    if ((res=test_is_drams())!=plOK) return(res);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_set_ped_prop={
    1, -1, "int var, int factor",
    "Entnimmt der Variablen var Pedestals und schreibt sie in die "
    "koehlerschen DRAMS-Module"};

procprop* prop_proc_DRAMS_set_ped()
{
    return(&DRAMS_set_ped_prop);
}
#endif
char name_proc_DRAMS_set_ped[]="DRAMS_set_ped";
int ver_proc_DRAMS_set_ped=1;

/*****************************************************************************/
/*
DRAMS_prepare
p[0] : number of arguments (==0)
*/

plerrcode
proc_DRAMS_prepare(ems_u32* p)
{
    prepare();
    return(plOK);
}

plerrcode
test_proc_DRAMS_prepare(ems_u32* p)
{
    plerrcode res;

    T(test_proc_DRAMS_prepare)
    D(D_USER, printf("test:DRAMS_prepare()\n");)
    if (p[0]!=0) return(plErr_ArgNum);
    if ((res=test_is_drams())!=plOK) return(res);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_prepare_prop={
    0, 0, "void",
    "schaltet DRAMS in Mess-Modus"};

procprop* prop_proc_DRAMS_prepare()
{
    return(&DRAMS_prepare_prop);
}
#endif
char name_proc_DRAMS_prepare[]="DRAMS_prepare";
int ver_proc_DRAMS_prepare=1;
/*****************************************************************************/
/*
DRAMS_Set
p[0] : No. of parameters
p[1] : IS_pedestals
*/
plerrcode
proc_DRAMS_Set(ems_u32* p)
{
    typedef struct {
        int pedestal;
        int threshold;
    } channel_pedestals;

    channel_pedestals *ptr;
    int anz, i, j, val;
    camadr_t cadr;

    anz=num_receiver;
    reset_address();
    ptr=(channel_pedestals*)(p+2);
    for (i=0; i<anz; i++) {
        ml_entry* M_rec=M_receiver(i);
        struct camac_dev* dev=M_rec->address.camac.dev;
        int slot=M_rec->address.camac.slot;
        cadr=dev->CAMACaddr(slot, 0, 16);
        for (j=0; j<512; j++) {
            val=(ptr[j].pedestal & 0xff)|((ptr[j+512].pedestal & 0xff)<<8);
            dev->CAMACwrite(dev, &cadr, val);
        }
        cadr=dev->CAMACaddr(slot, 0, 17);
        for (j=0; j<512; j++) {
            val=(ptr[j].threshold&0xff)|((ptr[j+512].threshold&0xff)<<8);
            dev->CAMACwrite(dev, &cadr, val);
        }
        ptr++;
    }

/* nachfolgende Messung vorbereiten */
    prepare();
    return plOK;
}

plerrcode
test_proc_DRAMS_Set(ems_u32* p)
{
    int num;
    plerrcode res;

    T(test_proc_DRAMS_Set)
    num=p[0];
    D(D_USER, printf("test_proc_DRAMS_Set: %d args\n", num);)
    if (num<1) return(plErr_ArgNum);
    if ((res=test_is_drams())!=plOK) return(res);
    if (p[1]!=num_receiver) return(plErr_ArgRange);
    if (num!=1+num_receiver*2048) return(plErr_ArgNum);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_Set_prop={
    0, 0, "pedestals",
    "Akzeptiert Pedestals als Argumente und schreibt sie in die "
    "koehlerschen DRAMS-Module"
};

procprop* prop_proc_DRAMS_Set()
{
    return(&DRAMS_Set_prop);
}
#endif
char name_proc_DRAMS_Set[]="DRAMS_Set";
int ver_proc_DRAMS_Set=1;
/*****************************************************************************/
/*
DRAMS_Readout
*/
plerrcode
proc_DRAMS_Readout(ems_u32* p)
{
    int n, anz, i;
    camadr_t cadr_read, cadr_test;
    ml_entry* M_cont=M_control;
    int N_cont=M_cont->address.camac.slot;
    struct camac_dev* dev=M_cont->address.camac.dev;
    ems_u32 stat;

/* Anzahl der auszulesenden Module */
    anz=num_receiver;
/* Adresse fuer Hardwarezugriff vorbereiten */
    cadr_test=dev->CAMACaddr(N_cont, 0, 0);

    DV(D_USER, printf("%d receiver\n", anz);)
    for (n=0; n<anz; n++) {
        ems_u32 val;
        /* DRAMS-Receiver */
        int N_rec=M_receiver(n)->address.camac.slot;
        cadr_read=dev->CAMACaddr(N_rec, 0, 1);

        /* Conversion abwarten */
        /* bei Fehlern sollte er hier haengenbleiben (timeout?)*/
        do {
            dev->CAMACread(dev, &cadr_test, &val);
        } while (!(val&2));
        do {
            dev->CAMACread(dev, &cadr_test, &val);
        } while (val&1);
        /* Daten auslesen */
        i=0;
        while (i<1024) {
            dev->CAMACread(dev, &cadr_read, &val);
            if (!dev->CAMACgotQ(val))
                break;
            i++;
            *outptr++=dev->CAMACval(val);
        };
        DV(D_USER, printf("%d words\n", i);)
    }

    /* Clear fuer naechsten Zyklus */
    dev->CCCC(dev);

    /* reset busy flipflop */
    dev->CAMACcntl(dev, dev->CAMACaddrP(N_cont, 0, 11), &stat);

    return plOK;
}

plerrcode
test_proc_DRAMS_Readout(ems_u32* p)
{
    plerrcode res;

    T(test_proc_DRAMS_Readout)
    if (p[0]!=0) return(plErr_ArgNum);
    if ((res=test_is_drams())!=plOK) return(res);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_Readout_prop={
    0, 0, "void",
    "Readout der DRAMS-Module"
};

procprop* prop_proc_DRAMS_Readout()
{
    return(&DRAMS_Readout_prop);
}
#endif
char name_proc_DRAMS_Readout[]="DRAMS_Readout";
int ver_proc_DRAMS_Readout=1;
/*****************************************************************************/
/*
save_ped
p[0] : number of arguments (==1)
p[1] : variable
*/
plerrcode
proc_DRAMS_save_ped(ems_u32* p)
{
    plerrcode res;
    int nummod, numchan, n, base, val;
    typedef struct {
        int pedestal;
        int thresh;
    } pedestals;
    pedestals *pedestalvar;
    camadr_t cadr_read;

    nummod=num_receiver;
    numchan=nummod*1024;

    /* ems-variable vorbereiten */
    var_delete(p[1]);
    if ((res=var_create(p[1], numchan*2))!=plOK)
        return(res);
    pedestalvar=(pedestals*)var_list[p[1]].var.ptr;

    for (n=0, base=0; n<nummod; n++, base+=1024) {
        struct camac_dev* dev=M_receiver(n)->address.camac.dev;
        int slot=M_receiver(n)->address.camac.slot;
        int i;
        cadr_read=dev->CAMACaddr(slot, 0, 0);
        for (i=0; i<512; i++) {
            dev->CAMACread(dev, &cadr_read, &val);
            pedestalvar[base+i].pedestal=val&0xff;
            pedestalvar[base+i+512].pedestal=(val>>8)&0xff;
        }
        cadr_read=dev->CAMACaddr(slot, 0, 2);
        for (i=0; i<512; i++) {
            dev->CAMACread(dev, &cadr_read, &val);
            pedestalvar[base+i].thresh=val&0xff;
            pedestalvar[base+i+512].thresh=(val>>8)&0xff;
        }
    }
    return(plOK);
}

plerrcode
test_proc_DRAMS_save_ped(ems_u32* p)
{
    if (p[0]!=1) return(plErr_ArgNum);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_save_ped_prop={
    0, 0, "int var",
    "Liest Pedestals und Thresholds aus DRAMS aus und speichert sie \n"
    "in der Variablen var."
};

procprop* prop_proc_DRAMS_save_ped()
{
    return(&DRAMS_save_ped_prop);
}
#endif
char name_proc_DRAMS_save_ped[]="DRAMS_save_ped";
int ver_proc_DRAMS_save_ped=1;

/*****************************************************************************/
/*
DRAMS_zero_ped
setzt alle Pedestals in allen Receiver-Modulen des IS auf 0
p[0] : number of arguments (==0)
*/
plerrcode
proc_DRAMS_zero_ped(ems_u32* p)
{
    int i, j, N;
    camadr_t cadr;

    T(proc_DRAMS_zero_ped)
    D(D_USER, printf("DRAMS_zero_ped()\n");)
    for (i=0; i<num_receiver; i++) {
        struct camac_dev* dev=M_receiver(i)->address.camac.dev;
        ems_u32 stat;
        N=M_receiver(i)->address.camac.slot;
        /* reset address */
        dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 24), &stat);

        /* Schreiben vorbereiten */
        cadr=dev->CAMACaddr(N, 0, 16);
        /* 512 Nullen schreiben */
        for (j=0; j<512; j++) dev->CAMACwrite(dev, &cadr, 0);
    }
    /* nachfolgende Messung vorbereiten */
    prepare();
    return(plOK);
}

plerrcode
test_proc_DRAMS_zero_ped(ems_u32* p)
{
    plerrcode res;

    T(test_proc_DRAMS_zero_ped)
    if (p[0]!=0) return(plErr_ArgNum);
    if ((res=test_is_drams())!=plOK) return(res);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_zero_ped_prop={
    0, 0, "void",
    "setzt alle Pedestals auf 0"
};

procprop* prop_proc_DRAMS_zero_ped()
{
    return(&DRAMS_zero_ped_prop);
}
#endif
char name_proc_DRAMS_zero_ped[]="DRAMS_zero_ped";
int ver_proc_DRAMS_zero_ped=1;
/*****************************************************************************/
/*
DRAMS_zero_thresh
setzt alle Thresholds in allen Receiver-Modulen des IS auf 0
p[0] : number of arguments (==0)
*/
plerrcode
proc_DRAMS_zero_thresh(ems_u32* p)
{
    int i, j, N;
    camadr_t cadr;

    T(proc_DRAMS_zero_thresh)
    D(D_USER, printf("DRAMS_zero_thresh()\n");)
    for (i=0; i<num_receiver; i++) {
        struct camac_dev* dev=M_receiver(i)->address.camac.dev;
        ems_u32 stat;
        N=M_receiver(i)->address.camac.slot;
        /* reset address */
        dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 24), &stat);

        /* Schreiben vorbereiten */
        cadr=dev->CAMACaddr(N, 0, 17);
        /* 512 Nullen schreiben */
        for (j=0; j<512; j++) dev->CAMACwrite(dev, &cadr, 0);
    }
    /* nachfolgende Messung vorbereiten */
    prepare();
    return(plOK);
}

plerrcode
test_proc_DRAMS_zero_thresh(ems_u32* p)
{
    plerrcode res;

    T(test_proc_DRAMS_zero_thresh)
    if (p[0]!=0) return(plErr_ArgNum);
    if ((res=test_is_drams())!=plOK) return(res);
    return(plOK);
}
#ifdef PROCPROPS
static procprop DRAMS_zero_thresh_prop={
    0, 0, "void",
    "setzt alle Schwellen auf 0"
};

procprop* prop_proc_DRAMS_zero_thresh()
{
    return(&DRAMS_zero_thresh_prop);
}
#endif
char name_proc_DRAMS_zero_thresh[]="DRAMS_zero_thresh";
int ver_proc_DRAMS_zero_thresh=1;
/*****************************************************************************/
/*****************************************************************************/
