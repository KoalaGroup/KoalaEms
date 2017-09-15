/*
 * events++/subevents.cc
 * 
 * created 28.01.95 PW
 */

#include <stdio.h>
#include <errors.hxx>
#include <nocopy.hxx>
#include <events.hxx>
#include <versions.hxx>

VERSION("Dec 08 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: subevents.cc,v 1.4 2004/11/26 14:40:20 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_subeventp::C_subeventp()
:valid(0), subevent(0), size_(0), trace(0)
{}

/*****************************************************************************/

C_subeventp::~C_subeventp()
{}

/*****************************************************************************/

void C_subeventp::store(const int* subevent_)
{
subevent=(int*)subevent_;
size_=subevent[1]+2;
valid=1;
}

/*****************************************************************************/

void C_subeventp::clear()
{
valid=0;
}

/*****************************************************************************/

void C_subeventp::id(int newid)
{
if (!valid) throw new C_program_error("C_subeventp::id: invalid");
subevent[0]=newid;
}

/*****************************************************************************/

C_subevent::C_subevent()
{}

/*****************************************************************************/

C_subevent::~C_subevent()
{
delete[] subevent;
}

/*****************************************************************************/

void C_subevent::store(const int* subevent_)
{
int i;

//cerr << "C_subevent::store called" << endl;
delete[] subevent;
size_=subevent_[1]+2;
subevent=new int[size_];
for (i=0; i<size_; i++) subevent[i]=subevent_[i];
valid=1;
}

/*****************************************************************************/

void C_subevent::clear()
{
delete[] subevent;
subevent=0;
valid=0;
}

/*****************************************************************************/
/*****************************************************************************/
