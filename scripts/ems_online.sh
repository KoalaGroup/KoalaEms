# server in the background
ONLINE_HOME=/home/koala/ems/KoalaEms/build/events++

if [ -d "$ONLINE_HOME" ]; then
    cd $ONLINE_HOME
else
    echo "ERROR: $ONLINE_HOME does not exist!"
    exit
fi

# kill existing
# pkill has a maximum charactor limit
pkill datacli_koala
# nohup ./datacli_koala localhost:5555 &>/dev/null &
gnome-terminal --tab "data client" -e "./datacli_koala localhost:5555"


pkill online_koala
# nohup ./online_koala &>/dev/null &
gnome-terminal --tab "online koala" -e "./online_koala"
