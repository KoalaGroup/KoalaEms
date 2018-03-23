#!/bin/sh -e

TCL_CLIENTLIB_DIR=/home/koala/ems/KoalaEms/tcl_clientlib
EMS_BUILD_DIR=/home/koala/ems/KoalaEms/build

export PATH=$EMS_BUILD_DIR/tcl_clientlib:$PATH
export EMSTCL_HOME=$TCL_CLIENTLIB_DIR/scripts
