/*
 * ems/tsm/triple.hxx
 * 
 * created 2004-12-02 PW
 */

template<class T1, class T2, class T3> struct triple {
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;
    T1 first;
    T2 second;
    T3 third;
    triple():first(T1()), second(T2()), third(T3()){}
    triple(const T1& a, const T2& b, const T3& c):first(a), second(b), third(c){}
    template<class U1, class U2, class U3>triple(const triple<U1, U2, U3>&t)
        :first(t.first), second(t.second), third(t.third){}
    bool operator< (const triple&) const;
};

template<class T1, class T2, class T3>
triple<T1, T2, T3> make_triple(const T1& a, const T2& b, const T3& c);
