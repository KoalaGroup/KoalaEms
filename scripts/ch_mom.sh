#!/bin/bash

SUPER_SETUP_FILE=/home/koala/.daq_control_rc
#SUPER_SETUP_FILE=/home/koala/workspace/bt2019/configuration/daq_control_rc
PREFIX="$1"

echo "Change filename prefix to: $PREFIX"
#echo "New DAQ Configuration File: "$SUPER_SETUP_FILE"

sed -i "s/bt2019\/configuration\/.*\/master\.wad/bt2019\/configuration\/$PREFIX\/master\.wad/" $SUPER_SETUP_FILE


