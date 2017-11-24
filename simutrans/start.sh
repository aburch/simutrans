#!/bin/sh

WORKDIR=$(pwd)
LOG_FILE="$WORKDIR/simu.log"
ARGS="-lang en"
EXEC="sim"

echo "starting Simutrans..."
echo "looking for executable..."
for bin in $WORKDIR/sim*; do
    if [ -f $bin ] && [ -x $bin ]; then
        echo "found executable: $bin"
        EXEC="$bin"
        break;
    fi
done;

if [ ! -x $EXEC ]; then
    echo "error: no suitable executable found in $WORKDIR" 1>&2
    exit 1
fi

echo "logging to file $LOG_FILE"
echo "execute: $EXEC $ARGS $@ > $LOG_FILE 2>&1"
$EXEC $ARGS "$@" > $LOG_FILE 2>&1
