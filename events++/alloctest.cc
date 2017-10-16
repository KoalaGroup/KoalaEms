#include <iostream>
#include <errno.h>

#define N 10000000

using namespace std;

int main()
{
delete[] x;
for (int j=0; j<10; j++) {
    int** ptrs=new int*[N];
    int i;

    cerr<<"ptrs="<<ptrs<<endl;
    for (i=0; i<N; i++) {
        ptrs[i]=new int[2];
    }
    cerr<<"allocated "<<N<<" items"<<endl;
    for (i=0; i<N; i++) {
        delete[] ptrs[i];
    }
    cerr<<"freed "<<N<<" items"<<endl;
    delete[] ptrs;
}
}
