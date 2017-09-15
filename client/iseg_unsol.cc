/*
 * iseg_unsol.cc
 * 
 * created: 2006-Sep-27 PW
 */

#include "config.h"
#include <iostream>
#include <cerrno>
//#include <sys/stat.h>
//#include <sys/types.h>
//#include <fcntl.h>

#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <xdrstring.h>

const int bus=0;
const char *hostname="zelas3";
const int port=4096;
const char *vedname="q20";

C_VED* ved;
int commpath;

/*****************************************************************************/
static C_instr_system*
open_is0(void)
{
    C_instr_system *is0;
    is0=ved->is0();
    if (is0==0) {
        cerr<<"VED "<<ved->name()<<" has no procedures."<<endl;
        return 0;
    }
    is0->execution_mode(delayed);
    return is0;
}
/*****************************************************************************/
static void
print_message(C_confirmation* message)
{
    //cout<<(*message)<<endl;
    if (!(message->header()->flags&Flag_Unsolicited))
        return;
#if 0
    if (message->header()->type.unsoltype!=Unsol_HeartBeat)
        return;
    if (message->size()<5) // too short
        return;
    if (message->buffer(0)!=1) // not a CanBus message
        return;
#endif

    int can_id=message->buffer(1);
    int type=message->buffer(2);
    // buffer(3) is byte counter
    int data_id=message->buffer(4);

    // decode ID
    int ID10=(can_id>>10)&1;
#if 0
    if (ID10) // not ISEG HV
        return;
#endif

    cout<<hex<<setfill('0');
    for (int i=1; i<message->size(); i++)
        cout<<" "<<setw(2)<<message->buffer(i);
    cout<<setfill(' ')<<dec;

    int ID9=(can_id>>9)&1;
    int addr=(can_id>>3)&0x3f;
    int nmt=(can_id>>2)&1;
    int ext=(can_id>>1)&1;
    int dir=can_id&1;
    if (ext)
        data_id+=256;

    cout<<" addr 0x"<<hex<<addr<<dec;
    if (ID9)
        cout<<" passive";
    if (nmt)
        cout<<" MNT";
    cout<<" id=0x"<<hex<<setfill('0')<<setw(2)<<data_id<<setfill(' ')<<dec;

    if (message->size()>5) {
        cout<<" data:"<<hex<<setfill('0');
        for (int i=5; i<message->size(); i++)
            cout<<" "<<setw(2)<<message->buffer(i);
        cout<<setfill(' ')<<dec;
    } else {
        cout<<" no data";
    }
    cout<<endl;
}
/*****************************************************************************/
static int
do_unsol(void)
{
    C_confirmation* message;
    struct timeval tv;
    fd_set readfd;
    int res;

    FD_ZERO(&readfd);
    FD_SET(commpath, &readfd);
    res=select(commpath+1, &readfd, 0, 0, 0);
    if ((res<0)&&(errno==EINTR))
        return 0;
    if (res<0) {
        cout<<"select: "<<strerror(errno)<<endl;
        return -1;
    }
    if (res==0) // impossible
        return 0;

    tv.tv_sec=0; tv.tv_usec=0;
    message=communication.GetConf(&tv);

    if (message!=0)
        print_message(message);
    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    try {
        C_instr_system *is0;

        communication.init(hostname, port);
        commpath=communication.path();
        
        ved=new C_VED(vedname);
        if ((is0=open_is0())==0)
            return 2;
        ved->SetUnsol(1);

        is0->clear_command();
        is0->command("canbus_unsol");
        is0->add_param(2, bus, 1);
        delete is0->execute();

        while (!do_unsol());
        delete ved;
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return 3;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
