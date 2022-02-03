#!/bin/sh

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# find the cpu type for SDL
sdl_cpu="`uname -p`"

if [ "${sdl_cpu}" == "i386" ]
then
	# OSX 10.4 for intel uses ppc SDL
	os_version="`/usr/sbin/system_profiler SPSoftwareDataType -detailLevel mini | egrep '(System Version): ' | cut -d: -f2-`"
	os_version="${os_version#*10.}"
	os_version="${os_version%.*}"

	if [ "${os_version}" == "4" ]
	then
		sdl_cpu="ppc"
	fi
fi

framework_path="${0%/*}"
if [ "${framework_path:0:1}" != "/" ]
then
	framework_path="`pwd`"
fi
framework_path="${framework_path}/../Frameworks"

# remake the link every time. The bundle might have been moved to a new system.
ln -f -s "${framework_path}/SDL-${sdl_cpu}.framework" "${framework_path}/SDL.framework"

# execute the game
"${0}.`uname -p`"

# open the console if the game crashed
if [ "$?" != "0" ]
then
/usr/bin/osascript <<-EOF
    tell application "Console" to activate
EOF
fi
