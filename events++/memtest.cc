#include <iostream>
#include <iomanip>

#include <sys/types.h>
#include <stdlib.h>

using namespace std;

class memory
  {
#if (__DECCXX_VER > 60090010)
  friend void* operator new(size_t) throw(std::bad_alloc);
#else
  friend void* operator new(size_t);
#endif
  friend void operator delete(void*) throw();
  public:
    static int used_mem;
  };

int memory::used_mem=0;

#if (__DECCXX_VER > 60090010)
void* operator new(size_t s) throw(std::bad_alloc);
#else
void* operator new(size_t s);
#endif
void operator delete(void* p) throw();

/*****************************************************************************/
#if (__DECCXX_VER > 60090010)
void* operator new(size_t s) throw(std::bad_alloc)
#else
void* operator new(size_t s)
#endif
{
//fprintf(stderr, "operator new(%d)", s);
memory::used_mem+=s;
size_t* p=(size_t*)malloc(s+sizeof(size_t));
*p++=s;
return p;
}
/*****************************************************************************/
#if (__DECCXX_VER > 60090010)
void operator delete(void* p) throw()
#else
void operator delete(void* p)
#endif
{
if (p)
  {
  size_t* pp=((size_t*)p)-1;
  size_t s=*pp;
  //cerr<<"operator delete("<<p<<", "<<s<<"); used_mem="<<memory::used_mem<<endl;
  memory::used_mem-=s;
  free(pp);
  }
else
  cerr<<"operator delete("<<p<<")"<<endl;
}
/*****************************************************************************/

struct AA
  {
  int a, b, c, d;
  };

main()
{
AA* aa;
cerr<<"1 memory used: "<<memory::used_mem<<endl;
aa=new AA[10];
cerr<<"2 memory used: "<<memory::used_mem<<endl;
delete[] aa;
cerr<<"3 memory used: "<<memory::used_mem<<endl;
aa=new AA[11];
cerr<<"4 memory used: "<<memory::used_mem<<endl;
delete[] aa;
cerr<<"5 memory used: "<<memory::used_mem<<endl;
}
/*****************************************************************************/
/*****************************************************************************/
