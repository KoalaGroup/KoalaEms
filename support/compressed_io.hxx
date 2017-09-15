/*
 * ems/support/compressed_io.hxx
 *
 * $ZEL: compressed_io.hxx,v 1.5 2010/02/02 23:47:33 wuestner Exp $
 *
 * created 2004-11-24 PW
 */

#ifndef _compressed_io_hxx_
#define _compressed_io_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <cstdio>

class compressed_io {
  public:
    enum compresstype {cpt_plain, cpt_compress, cpt_gzip, cpt_bzip2, cpt_stop};

  protected:
    compressed_io() :_path(-1), _file(0), _good(false), _cpt(cpt_stop), _child(0) {}
    int wait_for_child(pid_t pid, const char* name);
    string _name;
    int _path;
    FILE* _file;
    bool _good;
    compresstype _cpt;
    pid_t _child;

  public:
    string name(void) const {return _name;}
    int path(void) const {return _path;}
    FILE* file(void) const {return _file;}
    bool good(void) const {return _good;}
    int close(void);
    compresstype get_compresstype(void) const {return _cpt;}
};

class compressed_input: public compressed_io {
  protected:
    struct compressmagic {
        char *magic;
        char **tool;
    };
    static compressmagic *compressmagics;
    static const int magiclen;
    void open_file(int p=-1);
    void start_plain(int _p, const char* str, int len);
    void start_pipe(char* argv[], int p, const char* str, int len);
  private:
    static bool compressmagic_initialised;
    void init_compressmagic(void);

  public:
    compressed_input(void);
    compressed_input(const string name);
    compressed_input(int path);
    ~compressed_input();
    int open(const string name);
    int close();
    FILE* fdopen(void);
    istream sdopen(void);
};

class compressed_output: public compressed_io {
  protected:
    struct compresschain {
        char **tool;
    };
    static compresschain *compresschains;
    void open_file(bool force);
    void start_plain(int _p, const char* str, int len);
    //void start_pipe(const char* argv[], int _p, const char* str, int len);
    void start_pipe(const char*[], int&, char[], const int&);
    int _origpath;
  private:
    static bool compresschain_initialised;
    void init_compresschain(void);

  public:
    compressed_output(void);
    compressed_output(const string name, compresstype cpt, bool force=false);
    ~compressed_output();
    int open(const string name, compresstype cpt, bool force=false);
    int close();
    int flush();
    FILE* fdopen(void);
    ostream sdopen(void);
};

#endif
