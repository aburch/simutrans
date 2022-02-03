#!/bin/sh

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

date=`date +%Y`

PROG="$2"
COPYRIGHT="Copyright 1997-${date} by the simutrans team"
if [ -z "$3" ]; then
 VERSION="`../OSX/getversion`"
else
 VERSION=$3
fi
echo "Executable $PROG"
echo "$VERSION"


echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"
\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
        <key>CFBundleDevelopmentRegion</key>
        <string>English</string>
        <key>CFBundleDisplayName</key>
        <string>${PROG}</string>
        <key>CFBundleExecutable</key>
        <string>${PROG}</string>
        <key>CFBundleGetInfoString</key>
        <string>${VERSION}, ${COPYRIGHT}</string>
        <key>CFBundleIconFile</key>
        <string>${PROG}.icns</string>
        <key>CFBundleIdentifier</key>
        <string>org.${PROG}.${PROG}</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundleName</key>
        <string>${PROG}</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleShortVersionString</key>
        <string>${VERSION}</string>
        <key>CFBundleVersion</key>
        <string>${VERSION}</string>
        <key>NSHumanReadableCopyright</key>
        <string>${COPYRIGHT}</string>
        <key>NSPrincipalClass</key>
        <string>NSApplication</string>
</dict>
</plist>" > "$1"/Contents/Info.plist
