/*
 * iopathes.hxx
 *
 * $ZEL: iopathes.hxx,v 1.12 2004/11/26 14:45:37 wuestner Exp $
 *
 * created: 12.03.1998 PW
 * 16.03.1998 PW: adapted for <string>
 * 29.03.1998 PW: write, xsend, xwrite added
 * 02.04.1998 PW: noblock() and write_noblock added
 * 07.05.1998 PW: close() and reopen() added
 * 02.06.1998 PW: read() added
 * 11.07.1998 PW: seek() added
 * 28.07.1998 PW: write_buffered() and wbuffer_ added
 * 01.08.1998 PW: default constructor and init added
 * 19.06.1999 PW: C_iopath(const char* hostname,... added
 * 02.Jul.2001 PW: readln added
 * 30.Jul.2002 PW: read and write returning ssize_t instead of int
 */

#ifndef _iopathes_hxx_
#define _iopathes_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "nocopy.hxx"
#include "sockets.hxx"
#include "debuglevel.hxx"

/*****************************************************************************/

class C_iopath: public nocopy {
  public:
    typedef enum {iotype_none, iotype_file, iotype_tape, iotype_socket,
                  iotype_fifo} io_types;
    typedef enum {iodir_both, iodir_input, iodir_output} io_dir;
    C_iopath(const STRING& pathname, io_dir dir, int verbatim=0);
    C_iopath(int path, io_dir dir);
    C_iopath(const char* hostname, int port, io_dir dir);
    C_iopath();
    ~C_iopath();
  protected:
    STRING pathname_;
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
    void open();
    int file_open(const STRING& pathname_, io_dir dir);
    int socket_open(const STRING& pathname_, io_dir dir);
    int socket_open(const char* hostname_, int port, io_dir dir);
    io_types detect_type(int path);
    mode_t get_mode(int path);
    ssize_t xsend(void*, size_t);
    void xread(void*, size_t);
    int is_socket(int path);
    int is_tape(int path);
    int is_fifo(int path);
    ostream& print_socket(ostream&) const;
  public:
    void init(const STRING& pathname, io_dir dir, int verbatim=0);
    io_types typ() const {return typ_;}
    io_dir dir() const {return dir_;}
    const STRING& pathname() const {return pathname_;}
    int verbatim() const {return verbatim_;}
    int path() const {return path_;}
    ssize_t write(void*, size_t, int);
    void write_buffered(void*, size_t, int);
    void flush_buffer();
    int wbuffersize() const {return wbuffersize_;}
    int wbuffersize(int);
    ssize_t read(void*, size_t);
    STRING readln();
    ssize_t write_noblock(void*, size_t);
    void noblock();
    void close();
    void reopen();
    off_t seek(ems_u64, int);
    ostream& print(ostream&) const;
};

ostream& operator <<(ostream&, const C_iopath&);

#endif
/*****************************************************************************/
/*****************************************************************************/
