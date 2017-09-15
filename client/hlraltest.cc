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

VERSION("May 14 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: hlraltest.cc,v 2.3 2004/11/26 22:20:43 wuestner Exp $")

C_VED* ved;
C_instr_system* is;
C_readargs* args;
const char* vedname;
bool old;

/*****************************************************************************/
static int
readargs()
{
    int res;
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("pgen_ort", "p", DEFISOCK,
        "pgen_ort fgen_or connection with commu", "pgen_ort");
    args->use_env("pgen_ort", "EMSPgen_orT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket fgen_or connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    args->addoption("board", "b", 0, "PCI board", "pciboard");
    args->addoption("old", "o", false, "old server", "old");
    args->addoption("vedname", 0, "", "name of VED", "vedname");
    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "pgen_ort", "sockname", 0);

    if ((res=args->interpret(1))!=0) return res;

    vedname=args->getstringopt("vedname");
    old=args->getboolopt("old");
    return 0;
}
/*****************************************************************************/
static int
open_ved()
{
    try {
        if (!args->isdefault("hostname") || !args->isdefault("pgen_ort")) {
            communication.init(args->getstringopt("hostname"),
                    args->getintopt("pgen_ort"));
        } else {
            communication.init(args->getstringopt("sockname"));
        }
        cout<<"default timeout was "<<communication.deftimeout()<<"s"<<endl;
        communication.deftimeout(30);
        cout<<"default timeout is "<<communication.deftimeout()<<"s"<<endl;
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
            //cout<<"NUll; col++ or RegMark"<<endl;
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
#if 0
static int
parity(int v)
{
    unsigned int p0, p1, p2, p3, p4;
    p0=(v^(v>>16))&0xffff;
    p1=(p0^(p0>>8))&0xff;
    p2=(p1^(p1>>4))&0xf;
    p3=(p2^(p2>>2))&0x3;
    p4=(p3^(p3>>1))&0x1;
    return p4;
}
#endif
/*****************************************************************************/
static void
ral_testreadout_matrix(int board, ral_matrix* matrix)
{
    int len, bytes, byte, *buf, i;
    int row, rowerror=0;
    int card, chan, crate;
    C_confirmation* conf;

    try {
        conf=is->command("HLRALtestreadout", 1, board);
    } catch (C_error* e) {
        cerr<<"error in HLRALtestreadout"<<endl;
        throw e;
    }
    buf=conf->buffer();
    len=conf->size();
    buf++; // skip errorcode
    bytes=*buf++;
    //cout<<"len="<<len<<" bytes="<<bytes<<endl;

    for (crate=0; crate<16; crate++) {
        matrix[crate].clear();
    }

    i=0;
    row=0; crate=0;
    while (bytes) {
        byte=(*buf>>(8*i))&0xff;
        i++;
        if (i==4) {
            i=0;
            buf++;
        }
        bytes--;

        //int p=parity(byte);
        //cout<<(p?'+':'-');
        switch ((byte>>4)&7) {
        case 0:
            //cout<<"Ende"<<endl;
            //cout<<"e ";
            goto ende;
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
            //cout<<hex<<byte<<dec<<"      ";
            //cout<<"Karte "<<row<<" Faser "<<(byte&0xf)<<endl;
            //cout<<"f"<<(byte&0xf)<<' ';
            if (row<matrix[crate].cards()) {
                matrix[crate](row, byte&0xf)++;
            } else {
                cout<<"crate "<<crate<<" row="<<row
                    <<" cards="<<matrix[crate].cards()<<endl;
                rowerror++;
            }
            break;
        case 5:
            //cout<<"NUll; col++ or RegMark"<<endl;
            //cout<<"c++ ";
            crate++; row=0;
            break;
        case 6:
            //cout<<hex<<byte<<dec<<"      ";
            //cout<<"Karte "<<row<<" Faser "<<(byte&0xf)<<"; row++"<<endl;
            //cout<<"f"<<(byte&0xf)<<"r++ ";
            if (row<matrix[crate].cards()) {
                matrix[crate](row, byte&0xf)++;
            } else {
                rowerror++;
            }
            row++;
            break;
        case 7:
            //cout<<"NUll; row++"<<endl;
            //cout<<"r++ ";
            row++;
            break;
        }
    }
ende:
    if (bytes) {
        cout<<"unused bytes="<<bytes<<endl;
    }


    //cout<<endl;
    //if (row!=matrix.cards) {
    if (rowerror) {
        cout<<"testreadout: rows="<<row<<endl;
        buf=conf->buffer();
        buf++; // skip errorcode
        bytes=*buf++;
        while (bytes) {
            byte=(*buf>>(8*i))&0xff;
            i++;
            if (i==4) {
                i=0;
                buf++;
            }
            bytes--;

            //int p=parity(byte);
            //cout<<(p?'+':'-');
            switch ((byte>>4)&7) {
            case 0:
                cout<<"e ";
                break;
            case 1:
                cout<<hex<<byte<<dec<<" ";
                break;
            case 2:
                cout<<hex<<byte<<dec<<" ";
                break;
            case 3:
                cout<<hex<<byte<<dec<<" ";
                break;
            case 4:
                cout<<"f"<<(byte&0xf)<<' ';
                break;
            case 5:
                cout<<"c++ ";
                break;
            case 6:
                cout<<"f"<<(byte&0xf)<<"r++ ";
                break;
            case 7:
                cout<<"r++ ";
                break;
            }
        }
        cout<<endl;
    }
        
    delete conf;
}
/*****************************************************************************/
static void
ral_set_testregs(int board, int crate, int num, int* data)
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
        for (i=0; i<num; i++) {
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
        cerr<<"error in HLRALloadtestregs"<<endl;
        throw e;
    }
}
/*****************************************************************************/
static void
ral_set_single_channel(int board, int crate, int cards, int card, int chan)
{
    C_confirmation* conf;
    int* data;
    int idx, i;

    cout<<endl<<"checking card "<<card<<" fibre "<<chan<<endl;
    data=new int[cards*4];
    for (i=0; i<cards*4; i++) data[i]=0;
    idx=4*card+3-(chan/4);
    data[idx]=1<<(chan%4);
    //cout<<"data["<<idx<<"]="<< data[idx]<<endl;
    ral_set_testregs(board, crate, cards*4, data);
    delete[] data;
}
/*****************************************************************************/
static void
ral_check_single_channel(int board, int crate, int cards, int card, int chan)
{
    ral_set_single_channel(board, crate, cards, card, chan);
    ral_testreadout(board);
}
/*****************************************************************************/
static void
print_matrix2(ral_matrix& matrix1, ral_matrix& matrix2)
{
    int card, chan;

    cout<<hex;
    for (chan=0; chan<16; chan++) {
        for (card=0; card<matrix1.cards(); card++) {
            cout<<matrix1(card, chan);
        }
        cout<<"  ";
        for (card=0; card<matrix2.cards(); card++) {
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
    int size=old?matrix.cards():matrix.cards()*4;

    if (!size) return; /* crate does not exist or is empty */

    data=new int[size];
    memset(data, 0, sizeof(int)*size);

    if (old) {
        for (card=0; card<matrix.cards(); card++) {
            for (chan=0; chan<16; chan++) {
                if (matrix(card, chan)) {
                    data[card]|=1<<(chan);
                }
            }
        }
    } else {
        for (card=0; card<matrix.cards(); card++) {
            for (chan=0; chan<16; chan++) {
                if (matrix(card, chan)) {
                    int idx=4*card+3-(chan/4);
                    data[idx]|=1<<(chan%4);
                }
            }
        }
    }
    ral_set_testregs(board, crate, size, data);
    delete[] data;
}
/*****************************************************************************/
static void
ral_read_testregs(int board, int crate, ral_matrix& matrix)
{
    int* data;
    int card, chan;

    data=new int[matrix.cards()*4];
    memset(data, 0, sizeof(int)*matrix.cards()*4);

    ral_set_testregs(board, crate, matrix.cards(), data);
    for (card=0; card<matrix.cards(); card++) {
        for (chan=0; chan<16; chan++) {
            int idx=4*card+3-(chan/4);
            matrix(card, chan)=!!(data[idx]&(1<<(chan%4)));
        }
    }
    delete[] data;
}
/*****************************************************************************/
static int
ral_check_matrix(int board, ral_testmatrix& testmatrix)
{
    int crate, errors=0;

    for (crate=0; crate<16; crate++) {
        ral_set_testregs(board, crate, testmatrix.matrix[crate]);
    }
    ral_testreadout_matrix(board, testmatrix.ematrix);

    for (crate=0; crate<16; crate++) {
        if (testmatrix.diff(crate)) {
            cout<<"crate "<<crate<<endl;
            testmatrix.print_generator(cout);
            cout<<endl;
            print_matrix2(testmatrix.matrix[crate], testmatrix.ematrix[crate]);
            cout<<endl;
            errors+=testmatrix.errors[crate];
        }
    }

    return errors;
}
/*****************************************************************************/
static int
ral_count_chips(int board, int* chips)
{
    C_confirmation* conf;
    int nchips=0, i;

    conf=is->command("HLRALconfig", 2, board, 0);
    cout<<(*conf)<<endl;

    for (i=1; i<=16; i++) {
        int c=conf->buffer(i);
        chips[i-1]=c;
        nchips+=c;
        if (c) {
            cout<<"Crate "<<i<<": "<<c<<" modules"<<endl;
        }
    }
    if (!nchips)
        cout<<"no modules found"<<endl;

    delete conf;
    return nchips;
}
/*****************************************************************************/
static void
ral_reset(int board)
{
    C_confirmation* conf;
    //C_confirmation* command(const char* proc, int argnum, ...);

    if (old) return;

    try {
        conf=is->command("HLRALreset", 1, board);
        delete conf;
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
    }
}
/*****************************************************************************/
static
int generate_and_test(int board, ral_testmatrix& matrix,
        int cardmask, int channelmask,
        enum ral_testmatrix::generatortype gentype)
{
    int res;

    matrix.generate(cardmask, channelmask, gentype);
    matrix.print_generator(cout); cout<<endl;
    res=ral_check_matrix(board, matrix);
    return res;
}
/*****************************************************************************/
struct special_cases {
    unsigned int cardmask;
    unsigned int channelmask;
    enum ral_testmatrix::generatortype gentype;
};

struct special_cases special_cases[]={
    {0, 0x0000, ral_testmatrix::gen_or},
    {0, 0xffff, ral_testmatrix::gen_or},
    {0, 0xaaaa, ral_testmatrix::gen_or},
    {0, 0x5555, ral_testmatrix::gen_or},
    {0, 0xa5a5, ral_testmatrix::gen_or},
    {0, 0x5a5a, ral_testmatrix::gen_or},

    {0x3c02, 0xf928, ral_testmatrix::gen_or},
    {0x261f, 0x96e8, ral_testmatrix::gen_or},
    {0x2bfe, 0xc0d8, ral_testmatrix::gen_xor},
    {0x1e76, 0x8790, ral_testmatrix::gen_and},
    {0x1fcd, 0x0588, ral_testmatrix::gen_xor},
    {0x2cb1, 0xa468, ral_testmatrix::gen_or},
    {0x22e0, 0xf1c7, ral_testmatrix::gen_or},
    {0x2f13, 0x0f0b, ral_testmatrix::gen_xor},
    {0x36fc, 0xa547, ral_testmatrix::gen_xor},
    {0x1e76, 0x8790, ral_testmatrix::gen_and},

    {0x340b, 0x7377, ral_testmatrix::gen_xor},
    {0x2f12, 0xe35a, ral_testmatrix::gen_xor},
    {0x3a3d, 0x0e7f, ral_testmatrix::gen_xor},
    {0x1340, 0x7880, ral_testmatrix::gen_xor},
    {0x33e0, 0xecc8, ral_testmatrix::gen_xor},
    {0x15bf, 0xa280, ral_testmatrix::gen_xor},
    {0x21c2, 0xbd80, ral_testmatrix::gen_xor},
    {0x3fe0, 0x6780, ral_testmatrix::gen_xor},
    {0x1c3c, 0x7e7f, ral_testmatrix::gen_xor}, /*!!*/
    {0x0160, 0x2380, ral_testmatrix::gen_xor},
    {0x3070, 0x7380, ral_testmatrix::gen_xor},
    {0x2e3e, 0xa780, ral_testmatrix::gen_and},
    {0x027e, 0xa380, ral_testmatrix::gen_xor},
    {0x1d7f, 0xe380, ral_testmatrix::gen_and},
    {0x3ebe, 0x0d7f, ral_testmatrix::gen_xor},
    {0x0841, 0xb380, ral_testmatrix::gen_or},
    {0x3f41, 0x9c80, ral_testmatrix::gen_xor},
    {0x6dc1, 0x9380, ral_testmatrix::gen_xor},
    {0x0, 0x67a, ral_testmatrix::gen_or},
    {0x0, 0x785, ral_testmatrix::gen_or},
    {0x0, 0x7d7, ral_testmatrix::gen_or},
    {0x0, 0x786, ral_testmatrix::gen_or},
    {0x161f, 0xb7f, ral_testmatrix::gen_xor},
    {0, 0, ral_testmatrix::none},
};

/*****************************************************************************/
static void
do_something()
{
    int ncards, cards[16], card, chan, i, j, loop;
    int board;
    int trials=0, errors=0;
    struct timeval tv;

    gettimeofday(&tv, 0);
    srandom(tv.tv_usec);

    board=args->getintopt("board");

    ral_reset(board);
    ncards=ral_count_chips(board, cards);
    cout<<ncards<<" cards found:"<<endl;
    for (i=0; i<16; i++) {
        if (cards[i])
                cout<<"  crate "<<i+1<<": "<<cards[i]<<endl;
    }

    ral_testmatrix matrix(cards);

    struct special_cases* cases=special_cases;
    while (cases->gentype!=ral_testmatrix::none) {
        generate_and_test(board, matrix, cases->cardmask, cases->channelmask,
                cases->gentype);
        cases++;
    }

    for (loop=0;; loop++) {
        /* random cross */
        int cardmask, channelmask;
        int res;
        int r=random();

        cardmask=(r>>16)&0xffff;
        channelmask=r&0xffff;

        matrix.cardmask=cardmask;
        matrix.channelmask=channelmask;

        matrix.gentype=ral_testmatrix::gen_or;
        matrix.generate();
        res=ral_check_matrix(board, matrix);
        trials++;
        if (res) {
            errors++;
            cout<<errors<<'/'<<trials<<endl;
        }

        matrix.gentype=ral_testmatrix::gen_and;
        matrix.generate();
        res=ral_check_matrix(board, matrix);
        trials++;
        if (res) {
            errors++;
            cout<<errors<<'/'<<trials<<endl;
        }

        matrix.gentype=ral_testmatrix::gen_xor;
        matrix.generate();
        res=ral_check_matrix(board, matrix);
        trials++;
        if (res) {
            errors++;
            cout<<errors<<'/'<<trials<<endl;
        }
        if (!(loop&0xff)) cout<<"loop "<<loop<<endl;
    }

/*
    HLRALconfig 0 0
    HLRALbuffermode 0 1
    HLRALloaddac 0 1 0 35
    HLRALloaddac2 0 0 -1 -1 255
    HLRALloadtestregs 0 0 1
    HLRALtestreadout 0
    HLRALstartreadout 0 0
    HLRALreadout 0
*/
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
        cerr<<"Wir sind rausgeflogen."<<endl;
        cerr << (*e) << endl;
        delete e;
        return 3;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
