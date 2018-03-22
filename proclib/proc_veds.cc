/*
 * proc_veds.cc
 * 
 * created: 11.06.97
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <errno.h>
#include <proc_veds.hxx>
#include <ved_errors.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_veds.cc,v 2.9 2016/05/10 16:24:46 wuestner Exp $")
#define XVERSION

C_veds veds;

using namespace std;

/*****************************************************************************/

C_veds::C_veds()
:confmode(synchron)
{
listsize=10;
list_idx=0;
list=new entry[listsize];
if (list==0) throw new C_unix_error(errno, "C_veds::C_veds()");
}

/*****************************************************************************/

C_veds::~C_veds()
{
delete[] list;
}

/*****************************************************************************/

void C_veds::add(C_VED* ved, C_VED::VED_prior prior)
{
if (list_idx==listsize)
  {
  entry* help;
  int i;
  listsize+=10;
  help=new entry[listsize];
  if (help==0) throw new C_unix_error(errno, "C_veds::add");
  for (i=0; i<list_idx; i++) help[i]=list[i];
  delete[] list;
  list=help;
  }

int i, idx;

for (idx=0; (idx<list_idx) && (list[idx].prior<=prior); idx++);
for (i=list_idx; i>idx; i--) list[i]=list[i-1];
list[idx].ved=ved;
list[idx].prior=prior;
list_idx++;
}

/*****************************************************************************/

void C_veds::remove(const C_VED* ved)
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].ved!=ved); idx++);
if (list[idx].ved==ved)
  {
  int i;
  for (i=idx; i<list_idx-1; i++) list[i]=list[i+1];
  list_idx--;
  }
else
  throw new C_program_error("C_veds::remove: ved existiert nicht.");
}

/*****************************************************************************/

void C_veds::close()
{
while (list_idx>0) delete list[0].ved;
}

/*****************************************************************************/

C_VED::VED_prior C_veds::getprior(const C_VED* ved)
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].ved!=ved); idx++);
if (list[idx].ved==ved)
  return(list[idx].prior);
else
  throw new C_program_error("C_veds::getprior: ved existiert nicht.");
}

/*****************************************************************************/

const char* C_veds::operator [] (int idx) const
{
if (idx<list_idx)
  return(list[idx].ved->name().c_str());
else
  return(0);
}

/*****************************************************************************/

void C_veds::printlist() const
{
int i;

cout << "List of VEDs:" << endl;
for (i=0; i<list_idx; i++)
  {
  cout << list[i].ved->name() << ": " << list[i].prior << endl;
  }
}

/*****************************************************************************/

C_VED* C_veds::find(int ved) const
{
int i;

for (i=0; i<list_idx; i++)
  {
  if (list[i].ved->ved_id()==ved) return list[i].ved;
  }
return 0;
}

/*****************************************************************************/

void C_veds::setprior(C_VED* ved, C_VED::VED_prior prior)
{
remove(ved);
add(ved, prior);
}

/*****************************************************************************/

void C_veds::changeprior(C_VED* ved, int offs)
{
C_VED::VED_prior prior;
prior=getprior(ved);
remove(ved);
prior=static_cast<C_VED::VED_prior>(prior+offs);
add(ved, prior);
}

/*****************************************************************************/

void C_veds::ResetVED_all()
{
int idx;

for (idx=0; idx<list_idx; idx++) list[idx].ved->ResetVED();
}

/*****************************************************************************/

void C_veds::ResetVED()
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->ResetVED();
}

/*****************************************************************************/

void C_veds::ResetVED_R(void)
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++)
  {
  try
    {
    list[idx].ved->ResetVED();
    }
  catch (C_ved_error* error)
    {
    if ((error->errtype()==C_ved_error::req_err) &&
        (error->errconf().buffer(0)==Err_PIActive))
      {
      list[idx].ved->ResetReadout();
      list[idx].ved->ResetVED();
      }
    delete error;
    }
  }
}

/*****************************************************************************/

void C_veds::StartReadout()
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->StartReadout();
}

/*****************************************************************************/

void C_veds::ResetReadout()
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->ResetReadout();
}

/*****************************************************************************/

void C_veds::StopReadout()
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->StopReadout();
}

/*****************************************************************************/

void C_veds::ResumeReadout()
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->ResumeReadout();
}

/*****************************************************************************/

void C_veds::SetUnsol(int val)
{
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
for (; idx<list_idx; idx++) list[idx].ved->SetUnsol(val);
}

/*****************************************************************************/

int C_veds::GetReadoutStatus(ems_u32* mineventcount, ems_u32* maxeventcount)
{
const InvocStatus Invoc_10=static_cast<InvocStatus>(-10);
static InvocStatus matrix[5][5]=
  {{Invoc_error, Invoc_error,   Invoc_error,   Invoc_error,     Invoc_error},
   {Invoc_error, Invoc_alldone, Invoc_10,      Invoc_10,        Invoc_active},
   {Invoc_error, Invoc_10,      Invoc_stopped, Invoc_10,        Invoc_10},
   {Invoc_error, Invoc_10,      Invoc_10,      Invoc_notactive, Invoc_10},
   {Invoc_error, Invoc_active,  Invoc_10,      Invoc_10,        Invoc_active}};
ems_u32 count;
int res;
InvocStatus status;
int idx;

for (idx=0; (idx<list_idx) && (list[idx].prior==0); idx++);
if (idx==list_idx)
  {
  *mineventcount=0;
  *maxeventcount=0;
  return(0);
  }

status=list[idx++].ved->GetReadoutStatus(mineventcount);
*maxeventcount=*mineventcount;

for (; idx<list_idx; idx++)
  {
  res=list[idx].ved->GetReadoutStatus(&count);
  if (count<*mineventcount) *mineventcount=count;
  if (count>*maxeventcount) *maxeventcount=count;
  if (status!=-10) status=matrix[res+3][status+3];
  }
return(status);
}

/*****************************************************************************/

conf_mode C_veds::def_confmode(conf_mode mode)
{
conf_mode old=confmode;
confmode=mode;
return old;
}

/*****************************************************************************/

void C_veds::set_confmode(conf_mode mode)
{
for (int idx=0; idx<list_idx; idx++) list[idx].ved->confmode(mode);
}

/*****************************************************************************/
/*****************************************************************************/
