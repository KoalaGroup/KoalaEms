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


