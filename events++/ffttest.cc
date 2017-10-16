#include <iostream>
#include <iomanip>
#include <math.h>

#include "fft1.hxx"

const unsigned long nn=1024;

float data[2*nn];

int main(int argc, char* argv[])
{
    for (int i=0; i<nn; i++) {
        float x=i/100.;
        data[2*i]=sinf(x);
        data[2*i+1]=0.;
    }

    if (four1(data, nn, 1)<0)
        return 1;

    for (int i=0; i<nn; i++) {
        cout<<data[2*i]<<"\t"<<data[2*i+1]<<endl;
    }
    return 0;
}
