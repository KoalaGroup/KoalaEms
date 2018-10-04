#!/bin/sh -e

TOPDIR=`pwd`
trap 'cd $TOPDIR' EXIT

# package 'common', 'support', 'commu', 'clientlib', 'proclib' 'events++'
packages=( common support commu clientlib proclib events++)
for package in $packages
do
    if [ ! -d $package ];then
        mkdir $package
    fi

    if cd $package;then
        if [ -e Makefile ];then
            make distclean
        else
            rm -rf ./*
        fi
    
        ../../$package/configure && make depend && make
        cd $TOPDIR
    fi
done


