#!/bin/sh -e

TOPDIR=`pwd`
trap 'cd $TOPDIR' EXIT

# package 'tcl_clientlib'
TCL_INCLUDEDIR=/usr/include/tcl8.6
TK_INCLUDEDIR=/usr/include/tcl8.6
TCL_LIBDIR=/usr/lib/x86_64-linux-gnu
TK_LIBDIR=/usr/lib/x86_64-linux-gnu

# tcl libraries
TCL_PACKAGES=(common daq)

if [ ! -d tcl_clientlib ];then
    mkdir tcl_clientlib
fi

if cd tcl_clientlib;then
    if [ -e Makefile ];then
        make distclean
    else
        rm -rf ./*
    fi

    ../../tcl_clientlib/configure --with-tcl=$TCL_INCLUDEDIR --with-tk=$TK_INCLUDEDIR --with-tcllib=$TCL_LIBDIR --with-tklib=$TK_LIBDIR && make depend && make

    cd ../../tcl_clientlib/scripts
    for package in $TCL_PACKAGES
    do
        cd $package
        ./index
        cd ..
    done
    cd $TOPDIR
fi
