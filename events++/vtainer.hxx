/*
 * vtainer.hxx
 * 
 * created 2014-Nov-04 PW
 */

#ifndef _vtainer_hxx_
#define _vtainer_hxx_

#include <iostream>

template<class T>
class vtainer {
    public:
        vtainer(void);
        ~vtainer();
        T *data;
        T& operator[](int idx) {return data[idx];}
        int size(void) const {return pos;}
        int append(T&);
        void clear(void);
    protected:
        int grow(void);
        int capacity;
        int pos;
};

template<class T>
vtainer<T>::vtainer(void)
{
    data=0;
    capacity=0;
    pos=0;
}

template<class T>
vtainer<T>::~vtainer()
{
    delete data;
}

template<class T>
void vtainer<T>::clear(void)
{
    pos=0;
    // leave capacity and data unchanged, we can probably reuse the space
}

template<class T>
int vtainer<T>::grow(void)
{
    int newcap=capacity*2;
    if (!newcap)
        newcap=256;
    T *help=new T[newcap];
    if (!help) {
        std::cerr<<"vtainer: cannot allocate "<<newcap<< "entries"<<std::endl;
        return -1;
    }
    bcopy(data, help, pos*sizeof(T));
    delete data;
    data=help;
    capacity=newcap;
    return 0;
}

template<class T>
int vtainer<T>::append(T &entry)
{
    if (pos==capacity) {
        int res=grow();
        if (res)
            return res;
    }
    data[pos++]=entry;
    return 0;
}

#endif
