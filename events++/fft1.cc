
#include <math.h>

#include "fft1.hxx"

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr


int
four1(float data[], unsigned long nn, int isign)
/*
Replaces data[1..2*nn] by its discrete Fourier transform, if isign is input
as 1; or replaces data[1..2*nn] by nn times its inverse discrete Fourier
transform, if isign is input as -1. data is a complex array of length nn or,
equivalently, a real array of length 2*nn. nn MUST be an integer power of 2
(this is not checked for!).
*/
{
    unsigned long n,mmax,m,j,istep,i;
    /* Double precision for the trigonometric recurrences */
    double wtemp,wr,wpr,wpi,wi,theta; 
    float tempr,tempi;                

    /* sanity check */
    if (nn^nn-1) {
        cerr<<"four1: nn is not a power of 2 ("<<nn<<")"<<endl;
        return -1;
    }

    /* This is the bit-reversal section of the routine */
    n=nn << 1;
    j=1;
    for (i=1;i<n;i+=2) {
        if (j > i) {
            SWAP(data[j],data[i]); /* Exchange the two complex numbers. */
            SWAP(data[j+1],data[i+1]);
        }
        m=nn;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    /* Here begins the Danielson-Lanczos section of the routine */
    mmax=2;
    while (n > mmax) { /*Outer loop executed log2 nn times */
        istep=mmax << 1;
        /* Initialize the trigonometric recurrence */
        theta=isign*(6.28318530717959/mmax);
        wtemp=sin(0.5*theta);
        wpr = -2.0*wtemp*wtemp;
        wpi=sin(theta);
        wr=1.0;
        wi=0.0;
        for (m=1;m<mmax;m+=2) {
            for (i=m;i<=n;i+=istep) {
                j=i+mmax;
                tempr=wr*data[j]-wi*data[j+1];
                tempi=wr*data[j+1]+wi*data[j];
                data[j]=data[i]-tempr;
                data[j+1]=data[i+1]-tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr=(wtemp=wr)*wpr-wi*wpi+wr;
            wi=wi*wpr+wtemp*wpi+wi;
        }
        mmax=istep;
    }
    return 0;
}
