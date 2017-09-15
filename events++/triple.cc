/*
 * ems/tsm/triple.cc
 * 
 * created 2004-12-02 PW
 */

#include "triple.hxx"

template<class T1, class T2, class T3>
bool triple<T1, T2, T3>::operator<(const triple<T1, T2, T3>& t) const
{
    if (first<t.first) return true;
    if (t.first<first) return false;
    //first==t.first
    if (second<t.second) return true;
    if (t.second<second) return false;
    //second==t.second
    if (third<t.third) return true;
    if (t.third<third) return false;
    //third==t.third
    return false;
}

template<class T1, class T2, class T3>
triple<T1, T2, T3> make_triple(const T1& a, const T2& b, const T3& c)
{
    return triple<T1, T2, T3>(a, b, c);
}
