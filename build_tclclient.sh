#!/bin/sh -e

# package tcl_clientlib
TCL_INCLUDEDIR=/usr/include/tcl8.4
TK_INCLUDEDIR=/usr/include/tcl8.4
TCL_LIBDIR=/usr/lib/x86_64-linux-gnu
TK_LIBDIR=/usr/lib/x86_64-linux-gnu

if [ ! -d tcl_clientlib ];then
    mkdir tcl_clientlib
fi

if cd tcl_clientlib;then
    if [ -e Makefile ];then
        make distclean
    fi

    ../../tcl_clientlib/configure --with-tcl=$TCL_INCLUDEDIR --with-tk=$TK_INCLUDEDIR --with-tcllib=$TCL_LIBDIR --with-tklib=$TK_LIBDIR && make depend && make
    cd $TOPDIR
fi
