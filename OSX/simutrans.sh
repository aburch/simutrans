#!/bin/sh

"${0}.`uname -p`"

if [ "$?" != "0" ]
then
/usr/bin/osascript <<-EOF
    tell application "Console" to activate
EOF
fi
