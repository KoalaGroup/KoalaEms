#include <stdio.h>
main()
{
int nummod, numchan, i, aa, bb;
float faa, fbb;
FILE* f;

f=fopen("xfile", "r");
fscanf(f, "%d", &nummod);
numchan=nummod*1024;
for (i=0; i<numchan; i++)
  {
  fscanf(f, "%d%d", &aa, &bb);
  faa=*(float*)&aa; fbb=*(float*)&bb;
  printf("%04d: %.3f %.3f\n", i, faa, fbb);
  }
}
