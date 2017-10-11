/*
 * support/iopathes.hxx
 *
 * $ZEL: iopathes.hxx,v 1.14 2014/07/14 15:09:53 wuestner Exp $
 *
 */

#ifndef _iopathes_hxx_
#define _iopathes_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "nocopy.hxx"
#include "sockets.hxx"
#include "debuglevel.hxx"

/*****************************************************************************/

class C_iopath: public nocopy {
  public:
    typedef enum {iotype_none, iotype_file, iotype_tape, iotype_socket,
                  iotype_fifo} io_types;
    typedef enum {iodir_both, iodir_input, iodir_output} io_dir;
    C_iopath(const string& pathname, io_dir dir, int verbatim=0);
    C_iopath(int path, io_dir dir);
    C_iopath(const char* hostname, int port, io_dir dir);
    C_iopath();
    ~C_iopath();
  protected:
    string pathname_;
    int verbatim_;
    int path_;
    size_t wbuffersize_;
    ssize_t wbufferidx_;
    char* wbuffer_;
    sock* sockptr;
    sock* oldsockptr;
    io_types typ_;
    io_dir dir_;
    mode_t filemode_;
    typedef enum {advise_normal, advise_sequential, advise_random} io_advise;
    io_advise advise;
    off_t last_advise_offs;
    size_t fadvise_read;
    void open();
    int file_open(const string& pathname_, io_dir dir);
    int socket_open(const string& pathname_, io_dir dir);
    int socket_open(const char* hostname_, int port, io_dir dir);
    io_types detect_type(int path);
    mode_t get_mode(int path);
    void set_fadvise();
    void do_fadvise(size_t);
    ssize_t xsend(void*, size_t);
    void xread(void*, size_t);
    int is_socket(int path);
    int is_tape(int path);
    int is_fifo(int path);
    ostream& print_socket(ostream&) const;
  public:
    void init(const string& pathname, io_dir dir, int verbatim=0);
    io_types typ() const {return typ_;}
    io_dir dir() const {return dir_;}
    const string& pathname() const {return pathname_;}
    int verbatim() const {return verbatim_;}
    int path() const {return path_;}
    ssize_t write(void*, size_t, int);
    void write_buffered(void*, size_t, int);
    void flush_buffer();
    int wbuffersize() const {return wbuffersize_;}
    int wbuffersize(int);
    ssize_t read(void*, size_t);
    string readln();
    ssize_t write_noblock(void*, size_t);
    void noblock();
    void close();
    void reopen();
    off_t seek(ems_u64, int);
    void fadvise_sequential(bool);
    ostream& print(ostream&) const;
};

ostream& operator <<(ostream&, const C_iopath&);

#endif
/*****************************************************************************/
/*****************************************************************************/
