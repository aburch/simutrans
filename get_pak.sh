#!/bin/bash

#
# This file is part of the Simutrans-Extended project under the Artistic License.
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
	if which wget >/dev/null; then
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
	elif which curl >/dev/null; then
		curl --progress-bar -L "$1" > "$2" || {
			echo "Error: download of file '$2' failed (curl returned $?)" >&2
			rm -f "$2"
			return 1
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
		destdir="$(pwd)/simutrans"
	else
		destdir="$(pwd)"
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
		mkdir -p "simutrans"
		destdir="$(pwd)/simutrans"
	else
		destdir="$(pwd)"
	fi

	# Second, check if ground.Outside.pak exists at the root.
	# If so, we need to create a separate pakset directory (do not install directly into simutrans/)
	if [ -n "$(echo "$files" | grep -o "./ground\.Outside\.pak")" ]; then
		destdir="$destdir/$(basename $pakzippath | sed -E -e "s/\.t(ar\.)?gz//")" # Chop off extension
		mkdir -p "$destdir"
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
		mkdir -p "simutrans"
		destdir="$(pwd)/simutrans/"
	else
		destdir="$(pwd)/"
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
	"https://github.com/Phystam/pak256-release/archive/2.0.3.0.zip" \
	"http://bridgewater-brunel.me.uk/downloads/nightly/pakset/pak128.britain-ex-nightly.tar.gz" \
	"http://bridgewater-brunel.me.uk/downloads/nightly/pakset/pak128.sweden-ex-nightly.tar.gz" \
	"https://github.com/Flemmbrav/Pak192.Comic/releases/download/Nightly/pak192.comic-extended-nightly.zip" \
)


#
# main script
#
choices=()
installpak=()

pwd | grep "/simutrans$" >/dev/null || {
	echo "Cannot install paksets: Must be in a simutrans/ directory" >&2
	exit 1
}

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
