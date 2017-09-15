/*
 * dump_scaler.cc
 * $ZEL: dump_scaler.cc,v 1.2 2007/04/10 01:10:13 wuestner Exp $
 * created: 2006-Nov-08 PW
 */

#include "config.h"
#include <proc_communicator.hxx>
#include <proc_is.hxx>

const char *hostname = "wasa01-a";
const int port       = 4096;
const char *vedname  = "scaler";
const int isidx      = 1;
const int ems_var    = 1;
const int latch_mask = 0x0f;

C_VED* ved;
C_instr_system *is;

ems_u64 timestamp; /* usec */
ems_u32 nr_modules;
ems_u32 nr_channels[32]; /* no more than 32 modules...*/
ems_u64 val[32][32]; /* val[module][channel] */

/*****************************************************************************/
static void
start_communication(void)
{
    communication.init(hostname, port);
}
/*****************************************************************************/
static void
open_ved(void)
{
    ved=new C_VED(vedname);
}
/*****************************************************************************/
static void
open_is(void)
{
    is=new C_instr_system(ved, isidx, C_instr_system::open);
    is->execution_mode(delayed);
}
/*****************************************************************************/
static int
read_scaler(void)
{
    C_confirmation* conf;
    ems_u32* buf;

    is->clear_command();
    is->command("Timestamp");
    is->add_param(1, 1);
    is->command("sis3800ShadowUpdateAll");
    is->add_param(2, ems_var, latch_mask);
    conf=is->execute();
    buf=reinterpret_cast<ems_u32*>(conf->buffer());

    if (buf[0]) {
        cerr<<"error in EMS procedure: "<<buf[0]<<endl;
        delete conf;
        return -1;
    }
    buf++;

    timestamp=*buf++;   /* sec */
    timestamp*=1000000; /* converted to usec */
    timestamp+=*buf++;  /* usec */
    nr_modules=*buf++;
    for (unsigned int mod=0; mod<nr_modules; mod++) {
        nr_channels[mod]=*buf++;
        for (unsigned int chan=0; chan<nr_channels[mod]; chan++) {
            ems_u64 v;
            v=*buf++;
            v<<=32;
            v+=*buf++;
            val[mod][chan]=v;
        }
    }
    delete conf;
    return 0;
}
/*****************************************************************************/
static void
dump_scaler(void)
{
    cout<<"timestamp: "<<timestamp<<" us"<<endl;
    cout<<nr_modules<<" modules"<<endl;
    for (unsigned int i=0; i<nr_modules; i++) {
        cout<<i<<":";
        for (unsigned int j=0; j<nr_channels[i]; j++) {
            cout<<" "<<val[i][j];
        }
        cout<<endl;
    }
    cout<<endl;
}
/*****************************************************************************/
static void
mainloop(void)
{
    while (1) {
        if (read_scaler()==0) {
            dump_scaler();
        }
        sleep(10);
    }
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    try {
        start_communication();
        open_ved();
        open_is();

        mainloop();

    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return 2;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
