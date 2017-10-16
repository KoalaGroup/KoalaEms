#include <iostream.h>

unsigned int* iptr[]={
    0xb0,
    0xf070f070,
    0xcf70f070,
    0xcbcccdce,
    0x70e7c8c9,
    0xcecf70f0,
    0x43444ded,
    0xc6604142,
    0x70e0c2c4,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0xe06d4ff0,
    0xf070f070,
    0xf070f070,
    0xf070f070,
    0xf070f070,
    0xf070f070,
    0xf070f070,
    0x7050f070,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x6ff070f0,
    0x70e0c2c3,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0xf07050ef,
    0xf070f070,
    0xf070f070,
    0x70e06e4f,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x70f070f0,
    0x494a4bf0,
    0x44464748,
    0x70f06243,
    0xf070f0
};

/*****************************************************************************/
int scan_subevent(int* subevent)
{
int num=subevent[0];
unsigned char* data=(unsigned char*)(subevent+1);
int col=0, row=0, chan, slice=0;
int count=0;

for (int i=0; i<num; i++)
  {
  int d=data[i];
  int code=(d>>4)&0x7;
  int chan=d&0xf;
  switch (code)
    {
    case 0:
      if (i+1<num) {
        cerr<<"endekennung: i="<<i<<"; num="<<num<<endl;
      }
      return count;
      break;
    case 4: incr(col, row, chan, slice); count++; break;
    case 5:
      if (chan)
        {row=0; col=0; slice++;} // Zeitscheibe beim Testreadout
      else
        {row=0; col++;}
      break;
    case 6: incr(col, row, chan, slice); row++; break;
    case 7: row++; break;
    default:
      cerr<<"unknown code "<<code<<endl;
    }
  }
cerr<<"no end marker!"<<endl;
return count;
}

int main()
{
    scan_subevent(iptr);
    return 0;
}
