#!/bin/bash
#

# first assum unix ...
simexe=
updatepath="/"
updater="get_pak.sh"
simarchiv=simumac


getNightlies()
{
	# Use curl if available, else use wget
	curl -h > /dev/null
	if [ $? -eq 0 ]; then
	    curl http://simutrans-germany.com/stn/Download.php?d=9 > sim-macintel.zip || {
	      echo "Error: download of file sim-macintel.zip failed (curl returned $?)" >&2
	      rm -f "sim-macintel.zip"
	      exit 4
	    }
	    curl http://simutrans-germany.com/stn/Download.php?d=11 > sim-macpowerpc.zip || {
	      echo "Error: download of file sim-macpowerpc.zip failed (curl returned $?)" >&2
	      rm -f "sim-macpowerpc.zip"
	      exit 4
	    }
	else
	    wget --help > /dev/null
	    if [ $? -eq 0 ]; then
	        wget -N http://simutrans-germany.com/stn/Download.php?d=9 || {
	          echo "Error: download of file sim-macintel.zip failed (wget returned $?)" >&2
	          rm -f "sim-macintel.zip"
	          exit 4
	        }
	        wget -N http://simutrans-germany.com/stn/Download.php?d=11 || {
	          echo "Error: download of file sim-macpowerpc.zip failed (wget returned $?)" >&2
	          rm -f "sim-macpowerpc.zip"
	          exit 4
	        }
	    else
	        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
	        exit 6
	    fi
	fi
	unzip -tv "sim-macintel.zip" || {
	   echo "Error: file sim-macintel.zip seems to be defective" >&2
	   rm -f "sim-macintel.zip"
	   exit 5
	}
	unzip -tv "sim-macpowerpc.zip" || {
	   echo "Error: file sim-macpowerpc.zip seems to be defective" >&2
	   rm -f "sim-macpowerpc.zip"
	   exit 5
	}
}


# (otherwise there will be many .svn included under windows)
distribute()
{
	# pack all files of the current release
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll"  -o -name "*.pak" -o  -name "*.nut" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/$updater
	FILELISTE=`find simutrans/simutrans.app`
	zip -9r $simarchiv.zip $FILELISTE
}

# fetch language files
sh ./get_lang_files.sh

# now download the nightlies
cd simutrans
cp ..updatepath$updater .

getNightlies
unzip "sim-macpowerpc.zip"
unzip "sim-macintel.zip"
rm "sim-macpowerpc.zip"
rm "sim-macintel.zip"

# save the version number
simversion=`grep -a "Version " sim-macintel | awk 'BEGIN { RS = "\000" } {print $0 "\n"}' | grep "Version " `
# builds a bundle for MAC OS
mkdir -p "simutrans.app/Contents/MacOS"
mkdir -p "simutrans.app/Contents/Resources"
mv sim-macpowerpc "simutrans.app/Contents/MacOS/simutrans.ppc"
mv sim-macintel "simutrans.app/Contents/MacOS/simutrans.i386"
cp "../OSX/simutrans.icns" "simutrans.app/Contents/Resources/simutrans.icns"
cp "../OSX/simutrans.sh" "simutrans.app/Contents/MacOS/simutrans.sh"
echo "APPL????" > "simutrans.app/Contents/PkgInfo"
sh ../OSX/plistgen.sh "simutrans.app" "simutrans.sh" "$simversion"
echo "#!/bin/sh\n"${0}.`uname -p`"\nif [ "$?" != "0" ] ; then\n /usr/bin/osascript <<-EOF\n tell application "Console" to activate\n EOF\fi" > "simutrans.app/Contents/MacOS/simutrans.sh"

cd ..
distribute

# .. finally delete executable and language files
rm -rf simutrans/simutrans.app
