#ifndef _koa_parsers_hxx_
#define _koa_parsers_hxx_

#include <cstdint>

class ems_private;
class mxdc32_private;

namespace DecodeUtil
{
  // subevent parser functionoids/functors (ref: ioscpp.org/wiki/faq/pointers-to-members)
  class ParserVirtual
  {
  public:
    ParserVirtual(): fName("Invalid Parser"),
                     fType("Invalid Parser"),
                     fIS_ID(0xffffffff) {}
    ParserVirtual(const char* name, const char* type, uint32_t id): fName(name),
                                                                    fType(type),
                                                                    fIS_ID(id)
    {}
    virtual int Parse(const uint32_t *buf, int size) = 0;

    const char* GetName();

  private:
    const char *fName;
    const char *fType;
    const uint32_t fIS_ID;
  };

  class ParserTime: public ParserVirtual
  {
  public:
    ParserTime(ems_private* pri, const char* name, uint32_t id): ParserVirtual(name,"Parser_Time",id),
                                                                 fPrivate(pri)
                                                                 
    {}
    virtual int Parse(const uint32_t *buf, int size);

  private:
    ems_private* fPrivate;
  };

  class ParserScalor: public ParserVirtual
  {
  public:
    ParserScalor(ems_private* pri, const char* name, uint32_t id): ParserVirtual(name,"Parser_Scalor",id),
                                                                   fPrivate(pri)
    {}
    virtual int Parse(const uint32_t *buf, int size);

  private:
    ems_private* fPrivate;
  };

  class ParserMxdc32: public ParserVirtual
  {
  public:
    ParserMxdc32(mxdc32_private* pri, const char* name, uint32_t id): ParserVirtual(name,"Parser_Mxdc32",id),
                                                                      fPrivate(pri)
    {}
    virtual int Parse(const uint32_t *buf, int size);
    
  private:
    mxdc32_private* fPrivate;

  };


}
#endif
