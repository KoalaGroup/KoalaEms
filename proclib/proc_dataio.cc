/*
 * proc_dataio.cc
 * created: 10.06.97
 * 04.May.2001 PW: additional arguments added for dataout
 */

#include <errno.h>
#include <stdlib.h>
#include "proc_dataio.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include <versions.hxx>

VERSION("May 04 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_dataio.cc,v 2.7 2004/11/26 14:44:27 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_data_io::C_data_io(C_io_type* typ, C_io_addr* addr)
:typ(typ), addr(addr), args(0), numargs(0)
{
if (typ->uselists() || addr->uselists())
  {
  typ->forcelists(1);
  addr->forcelists(1);
  }
}

/*****************************************************************************/

C_data_io::~C_data_io()
{
delete typ;
delete addr;
delete[] args;
}

/*****************************************************************************/

void C_data_io::pruefen() const
{
cerr << "C_data_io::pruefen():" << endl;
typ->pruefen();
addr->pruefen();
}

/*****************************************************************************/

C_data_io* C_data_io::create(io_direction direction, const C_confirmation* conf)
{
C_data_io* io_addr;
C_io_type* typ;
C_io_addr* addr;

C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode

typ=C_io_type::create(direction, ib);
addr=C_io_addr::create(direction, ib, typ->uselists());

io_addr=new C_data_io(typ, addr);
if (!ib.empty())
  {
  ib>>io_addr->numargs;
  io_addr->args=new int[io_addr->numargs];
  for (int i=0; i<io_addr->numargs; i++) ib>>io_addr->args[i];
  }
return io_addr;
}

/*****************************************************************************/

int C_data_io::forcelists(int x)
{
int f=typ->uselists();
typ->forcelists(x);
addr->forcelists(x);
if (typ->uselists() || addr->uselists())
  {
  typ->forcelists(1);
  addr->forcelists(1);
  }
return f;
}

/*****************************************************************************/
void C_data_io::set_args(int* arg, int num)
{
delete[] args;
args=arg;
numargs=num;
if (num)
  {
  typ->forcelists(1);
  addr->forcelists(1);
  }
}
/*****************************************************************************/

C_outbuf& operator <<(C_outbuf& ob, const C_data_io& io)
{
ob << (*io.typ) << (*io.addr);
if (io.args)
  {
  ob<<io.numargs;
  for (int i=0; i<io.numargs; i++) ob<<io.args[i];
  }
return ob;
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_data_io& io)
{
os << (*io.typ) << ' ' << (*io.addr);
if (io.args)
  {
  os<<" {";
  for (int i=0; i<io.numargs; i++)
    {
    if (i>0) os<<' ';
    os<<io.args[i];
    }
  os<<'}';
  }
return os;
}

/*****************************************************************************/
/*****************************************************************************/
