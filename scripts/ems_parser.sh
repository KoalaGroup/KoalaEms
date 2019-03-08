#!/bin/bash

LIBDIR=/home/koala/ems/KoalaEms/build/events++
DECODER=$LIBDIR/new_parser_koala

LD_LIBRARY_PATH=$LIBDIR:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

$DECODER -m 3 "$@"
