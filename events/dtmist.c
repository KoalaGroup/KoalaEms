#include <stdio.h>
#include <errno.h>

main()
{
char a1[32], a2[24];
int r1, r2;
char* s;
char *s1="read 32";
char *s2="write 32";
char *s3="read 24";
while (1)
  {
  r1=read(0, a1, 32);
  if (r1!=32) {s=s1; goto raus;}
  r1=write(1, a1, 32);
  if (r1!=32) {s=s2; goto raus;}
  r2=read(0, a2, 24);
  if (r2!=24) {s=s3; goto raus;}
  }
raus:
perror(s);
}
