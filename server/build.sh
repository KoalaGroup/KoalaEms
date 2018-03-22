#!/bin/sh -e

TOPDIR=`pwd`
trap 'cd $TOPDIR' EXIT

# package 'common', 'support', 'commu', 'clientlib', 'proclib'
packages="common support commu clientlib proclib"
for package in $packages
do
    if [ ! -d $package ];then
        mkdir $package
    fi

    if cd $package;then
        if [ -e Makefile ];then
            make distclean
        fi
    
        ../../$package/configure && make depend && make
        cd $TOPDIR
    fi
done

# package tcl_clientlib
TCL_INCLUDEDIR=/usr/include/tcl8.4
TK_INCLUDEDIR=/usr/include/tcl8.4
TCL_LIBDIR=/usr/lib/x86_64-linux-gnu
TK_LIBDIR=/usr/lib/x86_64-linux-gnu

if [ ! -d tcl_clientlib];then
    mkdir tcl_clientlib
fi

if cd tcl_clientlib;then
    if [-e Makefile];then
        make distclean
    fi

    ../../tcl_clientlib/configure --with-tcl=$TCL_INCLUDEDIR --with-tk=$TK_INCLUDEDIR --with-tcllib=$TCL_LIBDIR --with-tklib=$TK_LIBDIR && make depend && make
    cd $TOPDIR
fi

