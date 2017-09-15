/*
 * ems/support/compressed_io.cc
 * 
 * created 2004-12-04 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "compressed_io.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: compressed_io.cc,v 1.6 2010/10/13 14:15:52 wuestner Exp $")
#define XVERSION

#if 0
const compressed_input::compressmagic compressed_input::compressmagics[]={
    {"", {"cat", 0}},
    {"\037\235", {"uncompress", "-c", 0}}, // compress
    {"\037\213", {"gunzip",     "-c", 0}}, // gzip
    {"BZh",      {"bunzip2",          0}},  // bzip2
};
const compressed_output::compresschain compressed_output::compresschains[]={
    {{"cat", 0}},
    {{"compress", "-c", 0}},
    {{"gzip",     "-c", 0}},
    {{"bzip2",    "-9", 0}},
};
#endif
const int compressed_input::magiclen=3;
bool compressed_input::compressmagic_initialised=false;
bool compressed_output::compresschain_initialised=false;
compressed_input::compressmagic *compressed_input::compressmagics;
compressed_output::compresschain *compressed_output::compresschains;

void compressed_input::init_compressmagic(void)
{
    if (compressmagic_initialised)
        return;

    compressmagics=new compressmagic[4];
    for (int i=0; i<4; i++) {
        compressmagics[i].tool=new char*[3];
    }
    compressmagics[0].magic=strdup("");
    compressmagics[0].tool[0]=strdup("cat");
    compressmagics[0].tool[1]=0;
    compressmagics[0].tool[2]=0;
    compressmagics[1].magic=strdup("\037\235");
    compressmagics[1].tool[0]=strdup("uncompress");
    compressmagics[1].tool[1]=strdup("-c");
    compressmagics[1].tool[2]=0;
    compressmagics[2].magic=strdup("\037\213");
    compressmagics[2].tool[0]=strdup("gunzip");
    compressmagics[2].tool[1]=strdup("-c");
    compressmagics[2].tool[2]=0;
    compressmagics[3].magic=strdup("BZh");
    compressmagics[3].tool[0]=strdup("bunzip2");
    compressmagics[3].tool[1]=strdup("-c");
    compressmagics[3].tool[2]=0;

    compressmagic_initialised=true;
}

void compressed_output::init_compresschain(void)
{
    if (compresschain_initialised)
        return;

    compresschains=new compresschain[4];
    for (int i=0; i<4; i++) {
        compresschains[i].tool=new char*[3];
    }
    compresschains[0].tool[0]=strdup("cat");
    compresschains[0].tool[1]=0;
    compresschains[0].tool[2]=0;
    compresschains[1].tool[0]=strdup("compress");
    compresschains[1].tool[1]=strdup("-c");
    compresschains[1].tool[2]=0;
    compresschains[2].tool[0]=strdup("gzip");
    compresschains[2].tool[1]=strdup("-c");
    compresschains[2].tool[2]=0;
    compresschains[3].tool[0]=strdup("bzip2");
    compresschains[3].tool[1]=strdup("-9");
    compresschains[3].tool[2]=0;

    compresschain_initialised=true;
}


compressed_input::compressed_input()
{
    init_compressmagic();
}

compressed_input::compressed_input(string __name)
{
    init_compressmagic();
    if (__name=="-") {
        open_file(0);
    } else {
        _name=string(__name);
        open_file();
    }
}

compressed_input::compressed_input(int p)
{
    init_compressmagic();
    open_file(p);
}

int
compressed_input::open(string __name)
{
    if (_good) {
        cerr<<"compressed_input("<<__name<<") already open with "<<_name<<endl;
        return -1;
    }
    _name=__name;
    open_file();
    return 0;
}

compressed_output::compressed_output(void)
:_origpath(-1)
{
    init_compresschain();
}

compressed_output::compressed_output(string __name, compresstype __cpt, bool force)
:_origpath(-1)
{
    init_compresschain();
    _name=__name;
    _cpt=__cpt;
    open_file(force);
}

int
compressed_output::open(string __name, compresstype __cpt, bool force)
{
    if (_good) {
        cerr<<"compressed_output("<<__name<<") already open with "<<_name<<endl;
        return -1;
    }
    _name=__name;
    _cpt=__cpt;
    open_file(force);
    return 0;
}

compressed_input::~compressed_input()
{
    close();
}

int
compressed_input::close()
{
    if (_file)
        ::fclose(_file);
    else
        ::close(_path);
    _file=0;
    _path=-1;
    _good=false;
    wait_for_child(_child, "decompressor");
    _child=0;
    return 0;
}

compressed_output::~compressed_output()
{
    close();
}

int
compressed_output::close()
{
    if (_file)
        ::fclose(_file);
    else
        ::close(_path);
    _file=0;
    _path=-1;
    ::close(_origpath);
    _origpath=-1;
    _good=false;
    wait_for_child(_child, "compressor");
    _child=0;
    return 0;
}

int
compressed_output::flush()
{
    if (_origpath<0) {
        // it was output without compression
        // nothing to do
    } else {
        if (_file)
            ::fclose(_file);
        else
            ::close(_path);
        _file=0;
        wait_for_child(_child, "compressor");
        _child=0;
        _path=_origpath;
        _origpath=-1;
    }
    lseek(_path, 0, SEEK_SET);
    return 0;
}

int
compressed_io::wait_for_child(pid_t pid, const char* xname)
{
    int status;
    pid_t res;

    if (pid==0)
        return 0;

    //cerr<<"wait for "<<pid<<" ("<<xname<<")"<<endl;
    res=waitpid(pid, &status, 0);
    if (res==static_cast<pid_t>(-1)) {
        cerr<<"wait for "<<xname<<": "<<strerror(errno)<<endl;
        return 0;
    }
    if (WIFEXITED(status)) {
        int ex=WEXITSTATUS(status);
        if (ex!=0) {
            cerr<<"exit status ("<<xname<<"): "<<ex<<endl;
            return -1;
        }
    } else if (WIFSIGNALED(status)) {
        //cerr<<xname<<" received signal "<<WTERMSIG(status)<<endl;
        cerr<<xname<<" received signal: "<<strsignal(WTERMSIG(status))<<endl;
        return -1;
    } else {
        cerr<<"wait for "<<xname<<": strange status" <<hex<<status<<dec<<endl;
        return -1;
    }
    return 0;
}

void
compressed_input::open_file(int p)
{
    char str[magiclen];
    int res;

    if (p<0) {
        if (_name=="-") {
            p=dup(0);
        } else {
            p=::open(_name.c_str(), O_RDONLY, 0);
            if (p<0) {
                cerr<<"open "<<_name<<": "<<strerror(errno)<<endl;
                return;
            }
        }
    }
    res=::read(p, str, magiclen);
    if (res<0) {
        cerr<<"read "<<_name<<": "<<strerror(errno)<<endl; 
        ::close(p);
        return;
    }
    if (res<3) {// cpt_plain
        cerr<<"read magic: "<<strerror(errno)<<endl;
        return;
    }
    for (_cpt=static_cast<compresstype>(cpt_stop-1);
            _cpt>cpt_plain;
            _cpt=static_cast<compresstype>(_cpt-1)) {
        if (strncmp(str, compressmagics[_cpt].magic,
                strlen(compressmagics[_cpt].magic))==0) {
            start_pipe(compressmagics[_cpt].tool, p, str, magiclen);
            return;
        }
    }
    // cpt_plain
    start_plain(p, str, magiclen);
}

void
compressed_input::start_plain(int p, const char* str, int len)
{
    // try to seek back to origin of file
    off_t seek=lseek(p, 0, SEEK_SET);
    if (seek==static_cast<off_t>(-1)) {
        // seek failed; we have to fork a 'cat'
        cerr<<"seek failed: "<<strerror(errno)<<endl;
        start_pipe(compressmagics[cpt_plain].tool, p, str, len);
    } else {
        // we can just use p
        _path=p;
        _good=true;
    }
}

void
compressed_input::start_pipe(char* argv[], int p,
        const char* str, int len)
{
    int pp[2];

    if (pipe(pp)) {
        cerr<<"pipe failed: "<<strerror(errno)<<endl;
        return;
    }

    switch ((_child=fork())) {
    case static_cast<pid_t>(-1):
        cerr<<"fork failed: "<<strerror(errno)<<endl;
        return;
    case 0: // child
        {
            int rr[2];
            ::close(pp[0]);
            if (pipe(rr)) {
                cerr<<"pipe in child failed: "<<strerror(errno)<<endl;
                exit(1);
            }
            switch (fork()) {
            case static_cast<pid_t>(-1):
                cerr<<"forking helper failed: "<<strerror(errno)<<endl;
                exit(1);
            case 0:
                {
                    char s[4096];
                    int res;
                    
                    dup2(p, 0);
                    dup2(rr[1], 1);
                    for (int i=getdtablesize()-1; i>2; i--) ::close(i);
                    res=write(1, str, len);
                    if (res!=len) {
                        fprintf(stderr, "io_helper/write: %s\n", strerror(errno));
                        exit(255);
                    }
                    while ((res=read(0, s, 4096))>0) {
                        int wres;
                        wres=write(1, s, res);
                        if (wres!=res) {
                            fprintf(stderr, "io_helper/write: %s\n", strerror(errno));
                            exit(255);
                        }
                    }
                }
                exit(0);
            default:
                {
                    dup2(rr[0], 0);
                    dup2(pp[1], 1);
                    for (int i=getdtablesize()-1; i>2; i--) ::close(i);
                    execvp(argv[0], argv);
                    cerr<<"exec "<<argv[0]<<":  "<<strerror(errno)<<endl;
                }
                exit(255);
            }
        }
        exit(255);
    default: // parent
        _path=pp[0];
        ::close(pp[1]);
        ::close(p);
        _good=true;
    }
}

void
compressed_output::open_file(bool force)
{
    int p;
    int flags=O_RDWR|O_CREAT|LINUX_LARGEFILE|(force?O_TRUNC:O_EXCL);

    if (_name=="-") {
        p=dup(1);
    } else {
        p=::open(_name.c_str(), flags, 0666);
        if (p<0) {
            cerr<<"create file "<<_name<<": "<<strerror(errno)<<endl;
            return;
        }
    }

    if (_cpt==cpt_plain) {
        _path=p;
        _child=0;
        _good=true;
        return;
    }

    int pp[2];

    if (pipe(pp)<0) {
        cerr<<"compressed_output: pipe: "<<strerror(errno)<<endl;
        return;
    }

    switch((_child=fork())) {
    case static_cast<pid_t>(-1): /* error */
        cerr<<"compressed_output: fork: "<<strerror(errno)<<endl;
        ::close(p);
        return;
    case static_cast<pid_t>(0): /* child */
        ::close(pp[1]);
        dup2(pp[0], 0);
        dup2(p, 1);
        for (int i=getdtablesize()-1; i>2; i--) ::close(i);

        execvp(compresschains[_cpt].tool[0], compresschains[_cpt].tool);
        exit(255);
    default: /* parent */
        _path=pp[1];
        _origpath=p;
        ::close(pp[0]);
        _good=true;
    }
}

FILE*
compressed_input::fdopen(void)
{
    if (!_file) _file=::fdopen(_path, "r");
    return _file;
}

FILE*
compressed_output::fdopen(void)
{
    if (!_file) _file=::fdopen(_path, "w");
    return _file;
}
