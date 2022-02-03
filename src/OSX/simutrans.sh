#!/bin/sh

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

"${0}.`uname -p`"

if [ "$?" != "0" ]
then
/usr/bin/osascript <<-EOF
    tell application "Console" to activate
EOF
fi
