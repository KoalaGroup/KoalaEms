# commu in the background
pkill commu
commu

# event distributor in the background
pkill event_distributor
nohup event_distributor &>/dev/null &

# server in the background
SERVER_HOME=/home/koala/ems/KoalaEms/build/cc_simple
LOGFILE=/home/koala/workspace/bt2019/logfile/cc_simple.log

if [ ! -f "$LOGFILE" ]; then
    mkdir -p `dirname $LOGFILE` && touch "$LOGFILE"
fi

if [ -d "$SERVER_HOME" ]; then
    cd $SERVER_HOME
else
    echo "ERROR: $SERVER_HOME does not exist!"
    exit
fi

# kill existing
# pkill has a maximum charactor limit
pkill server.cc_simpl
nohup ./server.cc_simple -l:vmep=/dev/sis1100_00\;sis3100 -w $LOGFILE &>/dev/null &
