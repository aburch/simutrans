#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

#
# script to fetch pak sets
# Downloads pak sets and installs them into $(pwd)/
#

# make sure that non-existing variables are not ignored
set -u

# fall back to "/tmp" if TEMP is not set
TEMP=${TEMP:-/tmp}


#
# Download file.
# Parameters:
#  URL
#  filename
#
# Status codes:
#  0: Success
#  1: Generic error
#  2: File not found
#
do_download()
{
	if which curl >/dev/null; then
		curl --progress-bar -L "$1" > "$2" || {
			echo "Error: download of file '$2' failed (curl returned $?)" >&2
			rm -f "$2"
			return 1
		}
	elif which wget >/dev/null; then
		wget -q --show-progress -O "$2" "$1" || {
			result=$?
			rm -f "$2"

			if [[ $result -eq 8 ]]; then
				echo "Error downloading file: Not found at URL '$1'" >&2
				return 2
			else
				echo "Unknown error downloading file (wget returned $result)" >&2
				return 1
			fi
		}
	else
		echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
		return 1
	fi

	return 0
}


install_cab()
{
	pakzippath="$1"
	files=$(cabextract --list "$pakzippath" 2>/dev/null | head -n -2 | tail -n +4 | cut -d '|' -f3- | cut -b 2-) || {
		echo "Error: Cannot extract .cab file (cabextract required)" >&2
		return 1
	}

	# First check if we only have a simutrans/ directory at the root.
	# If so, extract the archive here, otherwise into simutrans/
	if [ -n "$(echo "$files" | grep -v "^simutrans/")" ]; then
		mkdir -p "simutrans"
		destdir="$(pwd)"
	else
		destdir="$(pwd)/.."
	fi

	cabextract -qd "$destdir" "$pakzippath" || {
		echo "Error: Could not extract '$pakzippath' to '$destdir'" >&2
		return 1
	}

	return 0
}


install_tgz()
{
	pakzippath="$1"
	files=$(tar -ztf "$pakzippath") || {
		echo "Error: Cannot extract .tar.gz file (tar with gzip support required)" >&2
		return 1
	}

	# First check if we only have a simutrans/ directory at the root.
	# If so, extract the archive here, otherwise into simutrans/
	if [ -n "$(echo "$files" | grep -v "^simutrans/")" ]; then
		destdir="$(pwd)"
	else
		destdir="$(pwd)/.."
	fi

	tar -zxf "$pakzippath" -C "$destdir" || {
		echo "Error: Could not extract '$pakzippath' to '$destdir'" >&2
		return 1
	}

	return 0
}


install_zip()
{
	pakzippath="$1"

	# First check if we only have a simutrans/ directory at the root.
	# If so, extract the archive here, otherwise into simutrans/
	files=$(unzip -Z1 "$pakzippath" -x "simutrans/*" 2>/dev/null)
	result=$?

	if [ $result -eq 11 ]; then
		# Not matched - We only have a simutrans/ directory
		destdir="$(pwd)/"
	elif [ $result -ge 2 ]; then
		echo "Error: Could not extract '$pakzippath' (unzip -Z returned: $result)" >&2
		return 1
	elif [ -n "$files" ]; then
		destdir="$(pwd)/simutrans"
	else
		destdir="$(pwd)/.."
	fi

	echo "Extracting '$pakzippath' to '$destdir'..."
	unzip -qoC "$pakzippath" -d "$destdir"
	result=$?

	if [ $result -ge 2 ]; then
		# unzip failed
		echo "Error: Could not extract '$pakzippath' to '$destdir' (unzip returned: $result)" >&2
		return 1
	fi

	return 0
}


# two parameters:
#   - url
#   - zipfilename
download_and_install_pakset()
{
	url="$1"
	pakzippath="$2"

	echo "Downloading from '$1'"
	do_download "$url" "$pakzippath" || return 1

	destdir=""

	if [ -n "$(file "$pakzippath" | grep -i "Microsoft Cabinet archive data")" ]; then
		# .cab file
		install_cab "$pakzippath"
	elif [ -n "$(file "$pakzippath" | grep -i "gzip compressed data")" ]; then
		# .tar.gz
		install_tgz "$pakzippath"
	elif [ -n "$(file "$pakzippath" | grep -i "Zip archive data")" ]; then
		# .zip
		install_zip "$pakzippath"
	else
		rm -f "$pakzippath"
		echo "Error: Cannot extract unknown archive format" >&2
		return 1
	fi

	result=$?
	rm -f "$pakzippath"

	if [ $result -ne 0 ]; then
		echo "Installation failed." >&2
		return 1
	fi

	echo "Installation completed."
	return 0
}


# generated list of pak sets
paksets=( \
	"http://downloads.sourceforge.net/project/simutrans/pak64/123-0/simupak64-123-0.zip" \
	"http://simutrans-germany.com/pak.german/pak64.german_0-123-0-0-2_full.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak64.japan/123-0/simupak64.japan-123-0.zip" \
	"http://github.com/wa-st/pak-nippon/releases/download/v0.5.0/pak.nippon-v0.5.0.zip" \
	"https://github.com/Varkalandar/pak48.Excentrique/releases/download/v0.19_RC3/pak48.excentrique_v019rc3.zip" \
	"http://downloads.sourceforge.net/project/simutrans/Pak128.CS/nightly%20builds/pak128.CS-r2078.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20120-3/pak128.Britain.1.18-120-3.zip" \
	"http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_2.1_for_ST_123.0/PAK128.german_2.1_for_ST_123.0.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak128/pak128%202.8.2%20for%20ST%20123up/simupak128-2.8.2-for123.zip" \
	"http://github.com/Flemmbrav/Pak192.Comic/releases/download/2021-V0.6-RC2/pak192.comic.0.6.RC2.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip" \
	"http://pak128.jpn.org/souko/pak128.japan.120.0.cab" \
	"http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip" \
	"http://hd.simutrans.com/release/PakHD_v04B_100-0.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip" \
	"http://downloads.sourceforge.net/project/simutrans/pak64.scifi/pak64.scifi_112.x_v0.2.zip" \
	"http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz" \
)


#
# main script
#
choices=()
installpak=()

if [ "$#" -gt 0 ] && [ "$1" = '-generate_h' ]; then
	echo "Generate input file for Simutrans"
	echo "// Generated by \"get_pak.sh -generate_h\"" > "paksetinfo.h"
	echo "#define PAKSET_COUNT ${#paksets[@]}" >> "paksetinfo.h"
	echo "const char *pakinfo[PAKSET_COUNT*2] = {" >> "paksetinfo.h"
	for urlname in "${paksets[@]}"
	do
		zipname="${urlname##http*\/}"
		choicename="${zipname%.*}"
		choicename="${choicename%.tar}" # for .tar.gz
		choicename="${choicename/simupak/pak}"
		echo $'\t'"\"$urlname\", \"$choicename\"," >> "paksetinfo.h"
	done
	echo "};" >> "paksetinfo.h"
	exit 0
fi

pwd | grep "/simutrans$" >/dev/null || {
	echo "Cannot install paksets: Must be in a simutrans/ directory" >&2
	exit 1
}

# first find out, if we have a command options and just install these paks
# parameter is the full path to the pakset
if  [ "$#" -gt 0 ]; then

	for urlname in "$@"
	do
		cd ..
		zipname="${urlname##http*\/}"
		choicename="${zipname%.*}"
		choicename="${choicename%.tar}" # for .tar.gz
		choicename="${choicename/simupak/pak}"

		echo "-- Installing $choicename --"
		tempzipname="$TEMP/$zipname"
		download_and_install_pakset "$urlname" "$tempzipname" || {
			echo "Error installing pakset $choicename"
		}
	done
	echo "Installation finished."
	exit 0
fi


echo "-- Choose at least one of these paks --"

let setcount=0
let choicecount=0
let "maxcount = ${#paksets[*]}"

while [ "$setcount" -lt "$maxcount" ]; do
	installpak[choicecount]=0
	urlname=${paksets[$setcount]}
	zipname="${urlname##http*\/}"
	choicename="${zipname%.*}"
	choicename="${choicename%.tar}" # for .tar.gz
	choicename="${choicename/simupak/pak}"
	choices[choicecount]=$choicename
	let "setcount += 1"
	let "choicecount += 1"
	echo "${choicecount}) ${choicename}"
done

while true; do
	read -p "Which paks to install? (enter number or (i) to install or (x) to exit)" pak
	#exit?
	if [[ $pak = [xX] ]]; then
		exit 0
	fi

	# test if installation now
	if [[ $pak = [iI] ]]; then
		echo ""
		echo "You will install now"

		let setcount=0
		while [ $setcount -lt $choicecount ]; do
			if [ ${installpak[$setcount]} -gt 0 ]; then
				let "displaycount=$setcount+1"
				echo "${displaycount}) ${choices[$setcount]}"
			fi
			let "setcount += 1"
		done

		echo "into directory '$(pwd)'"
		read -p "Is this correct? (y/n)" yn
		if [[ $yn = [yY] ]]; then
			break
		fi

		# edit again
		echo ""
		echo "-- Choose again one of these paks --"
		let setcount=0
		while [ $setcount -lt $choicecount ]; do
			echo "${setcount}) ${choices[$setcount]}"
			let "setcount += 1"
		done
		let "pak=0"
	fi

	# otherwise it should be a number
	if [[ $pak =~ ^[0-9]+$ ]]; then
		let "setcount=pak-1"
		if [ $setcount -lt $choicecount ]; then
			if [ $setcount -ge 0 ]; then
				status=${installpak[$setcount]}
				if [ $status -lt 1 ]; then
					echo "adding ${choices[$setcount]}"
					installpak[$setcount]=1
				else
					echo "not installing ${choices[$setcount]}"
					installpak[$setcount]=0
				fi
			fi
		fi
	fi
done

echo ""

# install the regular pak sets first
let setcount=0
let "maxcount = ${#paksets[*]}"
fail=0

pushd .. 1>/dev/null

while [ "$setcount" -lt "$maxcount" ]; do
	if [ "${installpak[$setcount]}" -gt 0 ]; then
		urlname=${paksets[$setcount]}
		zipname="${urlname##http*\/}"

		echo "-- Installing ${choices[$setcount]} --"
		tempzipname="$TEMP/$zipname"
		download_and_install_pakset "$urlname" "$tempzipname" || {
			echo "Error installing pakset ${choices[$setcount]}"
			let "fail++"
		}

		echo ""
	fi
	let "setcount++"
done

popd 1>/dev/null
exit $fail
