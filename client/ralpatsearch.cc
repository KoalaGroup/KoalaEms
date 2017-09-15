/*
 * hlraltest.cc
 * created 12.May.2004 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <errorcodes.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "client_defaults.hxx"

#include "ral_testmatrix.hxx"

#include <versions.hxx>

VERSION("May 17 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: ralpatsearch.cc,v 2.2 2004/11/26 22:20:49 wuestner Exp $")

C_VED* ved;
C_instr_system* is;
C_readargs* args;
const char* vedname;

/*****************************************************************************/
static int
readargs()
{
    int res;
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", DEFISOCK,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    args->addoption("board", "b", 0, "PCI board", "pciboard");
    args->addoption("cardmask", "card", 0, "card bit mask", "card_mask");
    args->addoption("channelmask", "chan", 0, "channel bit mask", "channel_mask");
    args->addoption("operator", "op", "", "operator", "operator");
    args->addoption("vedname", 0, "", "name of VED", "vedname");
    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);

    if ((res=args->interpret(1))!=0) return res;

    vedname=args->getstringopt("vedname");
    return 0;
}
/*****************************************************************************/
static int
open_ved()
{
    try {
        if (!args->isdefault("hostname") || !args->isdefault("port")) {
            communication.init(args->getstringopt("hostname"),
                    args->getintopt("port"));
        } else {
            communication.init(args->getstringopt("sockname"));
        }
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        return -1;
    }
    try {
        ved=new C_VED(vedname);
        is=ved->is0();
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        communication.done();
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static void
ral_testreadout(int board)
{
    int len, bytes, byte, *buf, i;
    int col, row;
    C_confirmation* conf;

    conf=is->command("HLRALtestreadout", 1, board);
    buf=conf->buffer();
    len=conf->size();
    buf++; // skip errorcode
    bytes=*buf++;
    //cout<<"len="<<len<<" bytes="<<bytes<<endl;
    i=0;
    col=0; row=0;
    while (bytes) {
        byte=(*buf>>(8*i))&0xff;
        i++;
        if (i==4) {
            i=0;
            buf++;
        }
        bytes--;

        switch ((byte>>4)&7) {
        case 0:
            //cout<<"Ende"<<endl;
            break;
        case 1:
            cout<<hex<<byte<<dec<<" ";
            cout<<"unknown"<<endl;
            break;
        case 2:
            cout<<hex<<byte<<dec<<" ";
            cout<<"Status"<<endl;
            break;
        case 3:
            cout<<hex<<byte<<dec<<" ";
            cout<<"Clock+Daten"<<endl;
            break;
        case 4:
            cout<<hex<<byte<<dec<<"      ";
            cout<<"Karte "<<row<<" Faser "<<(byte&0xf)<<endl;
            break;
        case 5:
            //cout<<"NUll; col++ OR RegMark"<<endl;
            col++;
            break;
        case 6:
            cout<<hex<<byte<<dec<<"      ";
            cout<<"Karte "<<row<<" Faser "<<(byte&0xf)<<"; row++"<<endl;
            row++;
            break;
        case 7:
            //cout<<"NUll; row++"<<endl;
            row++;
            break;
        }
    }

    delete conf;
}
/*****************************************************************************/
static void
ral_testreadout_matrix(int board, ral_matrix& matrix, int verbose)
{
    int len, bytes, byte, *buf, i;
    int col, row;
    int card, chan;
    C_confirmation* conf;

    try {
        conf=is->command("HLRALtestreadout", 1, board);
    } catch (C_error* e) {
        cerr<<"Error in HLRALtestreadout"<<endl;
        throw e;
    }
    //if (verbose) cout<<(*conf)<<endl;
    buf=conf->buffer();
    len=conf->size();
    buf++; // skip errorcode
    bytes=*buf++;

    matrix.clear();

    if (verbose) cout<<hex;
    i=0;
    col=0; row=0;
    while (bytes) {
        byte=(*buf>>(8*i))&0xff;
        i++;
        if (i==4) {
            i=0;
            buf++;
        }
        bytes--;
        if (verbose) cout<<hex<<byte<<"  ";
        switch ((byte>>4)&7) {
        case 0:
            if (verbose) cout<<"Ende";
            break;
        case 1:
            //cout<<hex<<byte<<dec<<" ";
            if (verbose) cout<<"unknown";
            break;
        case 2:
            //cout<<hex<<byte<<dec<<" ";
            if (verbose) cout<<"Status";
            break;
        case 3:
            //cout<<hex<<byte<<dec<<" ";
            if (verbose) cout<<"Clock+Daten";
            break;
        case 4:
            //cout<<hex<<byte<<dec<<"      ";
            if (verbose) cout<<"Karte "<<row<<" Faser "<<(byte&0xf);
            matrix(row, byte&0xf)++;
            break;
        case 5:
            if (verbose) cout<<"NUll; col++ OR RegMark";
            col++;
            break;
        case 6:
            //cout<<hex<<byte<<dec<<"      ";
            if (verbose) cout<<"Karte "<<row<<" Faser "<<(byte&0xf)<<"; row++";
            matrix(row, byte&0xf)++;
            row++;
            break;
        case 7:
            if (verbose) cout<<"NUll; row++";
            row++;
            break;
        }
        if (verbose) cout<<endl;
    }
    if (verbose) cout<<dec<<endl;

    delete conf;
}
/*****************************************************************************/
static void
ral_set_testregs(int board, int crate, int cards, int* data)
{
    C_confirmation* conf;
    int idx, i;

    try {
        is->execution_mode(delayed);
        is->clear_command();
        is->command("HLRALloadtestregs");
        is->add_param(board);
        is->add_param(crate);
        is->add_param(3);
        //cout<<"sending:"<<endl;
        for (i=0; i<cards*4; i++) {
            //cout<<data[i]<<' ';
            is->add_param(data[i]);
        }
        //cout<<endl;
        conf=is->execute();
        //cout<<(*conf)<<endl;
        for (i=1; i<conf->size(); i++) {
             data[i-1]=conf->buffer(i);
        }
        delete conf;
        is->execution_mode(immediate);
    } catch (C_error* e) {
        cerr<<"Error in HLRALloadtestregs"<<endl;
        throw e;
    }
    //sleep(1);
}
/*****************************************************************************/
static void
print_matrix2(ral_matrix& matrix1, ral_matrix& matrix2)
{
    int card, chan;

    cout<<hex;
    for (chan=0; chan<16; chan++) {
        for (card=0; card<matrix1.cards; card++) {
            cout<<matrix1(card, chan);
        }
        cout<<"  ";
        for (card=0; card<matrix2.cards; card++) {
            cout<<matrix2(card, chan);
        }
        cout<<endl;
    }
    cout<<dec;
}
/*****************************************************************************/
static void
ral_set_testregs(int board, int crate, ral_matrix& matrix)
{
    int* data;
    int card, chan;

    data=new int[matrix.cards*4];
    memset(data, 0, sizeof(int)*matrix.cards*4);

    for (card=0; card<matrix.cards; card++) {
        for (chan=0; chan<16; chan++) {
            if (matrix(card, chan)) {
                int idx=4*card+3-(chan/4);
                data[idx]|=1<<(chan%4);
            }
        }
    }
    ral_set_testregs(board, crate, matrix.cards, data);
    delete[] data;
}
/*****************************************************************************/
static void
ral_read_testregs(int board, int crate, ral_matrix& matrix)
{
    int* data;
    int card, chan;

    data=new int[matrix.cards*4];
    memset(data, 0, sizeof(int)*matrix.cards*4);

    ral_set_testregs(board, crate, matrix.cards, data);
    for (card=0; card<matrix.cards; card++) {
        for (chan=0; chan<16; chan++) {
            int idx=4*card+3-(chan/4);
            matrix(card, chan)=!!(data[idx]&(1<<(chan%4)));
        }
    }
    delete[] data;
}
/*****************************************************************************/
static int
ral_check_matrix(int board, int crate, ral_testmatrix& testmatrix)
{
    testmatrix.print_generator(cout);
    cout<<endl;

    ral_set_testregs(board, crate, testmatrix.matrix);

    ral_testreadout_matrix(board, testmatrix.ematrix, 0);

    if (testmatrix.diff()==0)
        return 0;

    ral_matrix xmatrix(testmatrix.matrix.cards);
    ral_matrix ymatrix(testmatrix.matrix.cards);
    int loop, res;
    int errors=0, eerrors=0;

    ral_read_testregs(board, crate, ymatrix);
    if (testmatrix.diff(ymatrix, 0)) {
        cout<<"error in testpattern:"<<endl;
        print_matrix2(testmatrix.matrix, ymatrix);
        cout<<"data:"<<endl;
    }
    print_matrix2(testmatrix.matrix, testmatrix.ematrix);
    cout<<endl;

    for (loop=0; loop<100; loop++) {
        //cout<<"loop "<<loop<<endl;
        ral_set_testregs(board, crate, testmatrix.matrix);
        ral_testreadout_matrix(board, xmatrix, 0);
        if (testmatrix.diff(xmatrix, 0)) {
            errors++;
            res=testmatrix.diff(xmatrix, 1);
            if (res && !eerrors) {
                cout<<"New Error:"<<endl;
                print_matrix2(testmatrix.matrix, xmatrix);
                eerrors++;
            }
        }
    }
    testmatrix.percentage=errors;
    testmatrix.print_statistics(cout);
    if (eerrors) cout<<"different errors: "<<eerrors<<endl;
    cout<<endl;

    return testmatrix.errors;
}
/*****************************************************************************/
static int
ral_check_matrix_light(int board, int crate, ral_testmatrix& testmatrix,
    int olderrors)
{
    ral_set_testregs(board, crate, testmatrix.matrix);

    ral_testreadout_matrix(board, testmatrix.ematrix, 0);

    if (testmatrix.diff()<=olderrors)
        return -1;

    ral_matrix xmatrix(testmatrix.matrix.cards);
    ral_matrix ymatrix(testmatrix.matrix.cards);
    int loop;
    int errors=0;

    ral_read_testregs(board, crate, ymatrix);
    if (testmatrix.diff(ymatrix, 0))
        return -1;

    for (loop=0; loop<100; loop++) {
        ral_testreadout_matrix(board, xmatrix, 0);
        if (testmatrix.diff(xmatrix, 0)) {
            errors++;
        }
    }
    testmatrix.percentage=errors;
    //print_matrix2(testmatrix.matrix, testmatrix.ematrix);
    //testmatrix.print_statistics(cout);

    return testmatrix.errors;
}
/*****************************************************************************/
static int
ral_count_chips(int board)
{
    C_confirmation* conf;
    int chips;

    conf=is->command("HLRALconfig", 2, board, 0);
    chips=conf->buffer(1);
    delete conf;
    return chips;
}
/*****************************************************************************/
static void
ral_reset(int board)
{
    C_confirmation* conf;
    //C_confirmation* command(const char* proc, int argnum, ...);

    conf=is->command("HLRALreset", 1, board);
    delete conf;
}
/*****************************************************************************/
static
int generate_and_test(int board, int crate, ral_testmatrix& matrix,
        int cardmask, int channelmask,
        enum ral_testmatrix::generatortype gentype)
{
    int res;

    matrix.generate(cardmask, channelmask, gentype);
    res=ral_check_matrix(board, crate, matrix);
    return res;
}
/*****************************************************************************/
static void
do_something()
{
    ral_testmatrix* matrix;
    int cards, card, chan, i, j, loop;
    int board, cardmask, channelmask;
    int crate=0;
    const char* oper;
    ral_testmatrix::generatortype generator;
    struct timeval tv;

    gettimeofday(&tv, 0);
    srandom(tv.tv_usec);

    board=args->getintopt("board");

    ral_reset(board);
    cards=ral_count_chips(board);
    cout<<cards<<" cards for board "<<board<<" found."<<endl;

    matrix=new ral_testmatrix(cards);

    cardmask=args->getintopt("cardmask");
    channelmask=args->getintopt("channelmask");
    oper=args->getstringopt("operator");
    switch (oper[0]) {
        case 'a': generator=ral_testmatrix::and; break;
        case 'o': generator=ral_testmatrix::or; break;
        case 'x': generator=ral_testmatrix::xor; break;
        default:
            cout<<"unknown operator '"<<oper<<"'"<<endl;
            cout<<"only 'or', 'and' and 'xor' are allowed"<<endl;
            return;
    }

    generate_and_test(board, crate, *matrix, cardmask, channelmask, generator);
    while (1) {
        int res;
        ral_matrix oldmatrix=matrix->matrix;
        int olderrors=matrix->errors;
        matrix->mutate();
        res=ral_check_matrix_light(board, crate, *matrix, olderrors);
        
        if (res<0)
            goto zurueck;
        if (matrix->percentage<90)
            goto zurueck;
        cout<<"!!!!!!!!!!!!!!"<<endl;
        print_matrix2(matrix->matrix, matrix->ematrix);
        matrix->print_statistics(cout);
        continue;
zurueck:
        matrix->matrix=oldmatrix;
        matrix->errors=olderrors;
    }
}
/*****************************************************************************/
main(int argc, char* argv[])
{
    try {
        args=new C_readargs(argc, argv);
        if (readargs()!=0) return 1;

        if (open_ved()<0) return 2;

        do_something();

        delete ved;
        communication.done();
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        return 3;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
