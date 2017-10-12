#!/bin/sh

if [ ! -f count ] ; then
  echo 0 >count
fi
count=`cat count`
lines=`wc $1|awk '{ print $1 }'`
echo $1 $lines
count=`expr $count + $lines`
echo $count > count
