#include <stdio.h>

int spec[144];

int main(int argc, char* argv[])
{
    int i, j;
    for (i=0; i<144; i++)
        spec[i]=0;
    for (j=1; j<144; j++) {
        for (i=0; i<1000; i++) {
            int a, b, d;
            a=(i*3)/4;
            b=((i+j)*3)/4;
            d=b-a;
            spec[d]++;
        }
    }
    for (i=0; i<144; i++) {
        printf("[%3d]: %4d\n", i, spec[i]);
    }
    return 0;
}
