/*
 * commu_io.hxx
 * 
 * $ZEL: commu_io.hxx,v 2.4 2009/08/21 21:50:51 wuestner Exp $
 * 
 * created 29.07.94 PW
 */

#ifndef _commu_io_hxx_
#define _commu_io_hxx_

#include <commu_sock.hxx>
#include <commu_message.hxx>

/*****************************************************************************/

class C_io
  {
  private:
    int size;
    int idx;
    int read_it(int, int, char*);
    int write_it(int, int, char*);
    const char* statstr();
  public:
    C_io();
    ~C_io();
    typedef enum
      {
      iost_idle, iost_ready, iost_header, iost_body, iost_error, iost_broken
      } stat;
    stat status;
    int error;
    C_message* message;
    void read_(C_socket &);
    void write_(C_socket &, C_messagelist &);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
