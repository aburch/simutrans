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
  files=$(cabextract --list "$pakzippath" 2>/dev/null) || {
    #echo "Warning: no cabextract, assuming mingw on windows" >&2
    files=$(extrac32 //D "$pakzippath" 2>/dev/null) || {
      echo "Error: Cannot extract .cab file (Either cabextract or makecab required)" >&2
      return 1
    }
    # this is likely Mingw on windows using extrac32
    # First check if we only have a simutrans/ directory at the root.
    files=$(echo "$files" | grep ' simutrans\\')
    if [ $? -eq 0 ]; then
      # has simutrans folder, but cabextract cannot handle unix path on windows
      destdir="."
    else
      mkdir -p simutrans
      destdir="simutrans"
    fi
    files=$(extrac32 //L "$destdir" //Y //E "$pakzippath" 2>/dev/null) || {
      echo "Error: Could not extract '$pakzippath' to '$destdir'" >&2
      return 1
    }
    return 0
  }

  # First check if we only have a simutrans/ directory at the root.
  files=$(echo "$files" | grep "| simutrans/")
  if [ $? -eq 0 ]; then
    # has simutrans folder, but cabextract cannot handle unix path on windows
    destdir="."
  else
    mkdir -p simutrans
    destdir="simutrans"
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
  files=$(echo "$files"|grep "^simutrans")
  if [ $? -eq 0 ]; then
    # has simutrans folder
    destdir="$(pwd)"
    extra="--strip-components=1"
  else
    mkdir -p simutrans
    destdir="$(pwd)/simutrans"
    extra=""
  fi

  # No quotes around $extra, since adding quotes breaks extraction if $extra evaluates to the empty string
  # (This will be interpreted as the file(s) to extract and not as a command line parameter)
  tar -xzf "$pakzippath" $extra -C "$destdir" || {
    echo "Error: Could not extract '$pakzippath' to '$destdir'" >&2
    return 1
  }

  return 0
}


install_zip()
{
  pakzippath="$1"

  # First check if we only have a simutrans/ directory at the root.
  files=$(unzip -Z1 "$pakzippath" | grep "^simutrans/")
  if [ $? -eq 0 ]; then
    # we have simutrans already
    destdir="$(pwd)"
    echo "Extracting '$pakzippath' to '$destdir'..."
    temp=$(mktemp -d) && unzip -qoC "$pakzippath" -d "$temp" && mv "$temp"/*/* "$destdir" && rmdir "$temp"/* "$temp"
  else
    destdir="$(pwd)"
    echo "Extracting '$pakzippath' to '$destdir'..."
    unzip -qoC "$pakzippath" -d "$destdir"
  fi

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
obsolete_start_index=11
paksets=( \
  "http://downloads.sourceforge.net/project/simutrans/pak64/124-1-1/simupak64-124-1-1.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20ST%20%20124.1up%20%282.9.1%29/simupak128-2.9.1.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192.comic%20V0.7.1/pak192-comic.zip" \
  "http://simutrans-germany.com/pak.german/pak64.german_0-124-0-0-3_full.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak64.japan/123-0/simupak64.japan-123-0.zip" \
  "https://github.com/wa-st/pak-nippon/releases/download/v0.6.2/pak.nippon-v0.6.2.zip" \
  "http://downloads.sourceforge.net/project/simutrans/Pak128.CS/nightly%20builds/pak128.CS-r2096.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20120-3/pak128.Britain.1.18-120-3.zip" \
  "http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_2.2_for_ST_124.0/PAK128.german_2.2_for_ST_124.0.zip" \
  "https://github.com/Varkalandar/pak144.Excentrique/releases/download/r0.08/pak144.Excentrique_v008.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pakTTD/simupakTTD-124-0.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip" \
  "http://pak128.jpn.org/souko/pak128.japan.120.0.cab" \
  "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip" \
  "https://github.com/Varkalandar/pak48.Excentrique/releases/download/v0.19_RC3/pak48.excentrique_v019rc3.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip" \
  "http://hd.simutrans.com/release/PakHD_v04B_100-0.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip" \
  "http://downloads.sourceforge.net/project/simutrans/pak64.scifi/pak64.scifi_112.x_v0.2.zip" \
  "https://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz" \
)

#
# main script
#
choices=()
installpak=()

paks_header="../../src/paksetinfo.h"
nsis_header="../../src/Windows/nsis/paksets.nsh"


if [ "$#" -gt 0 ] && [ "$1" = '-generate_h' ]; then
  echo "Generate input file for Simutrans and NSIS"

  mkdir tmp
  cd tmp
  mkdir simutrans

  echo "; Generated by \"get_pak.sh -generate_h\". DO NOT EDIT." > "$nsis_header"

  echo "// Generated by \"get_pak.sh -generate_h\". DO NOT EDIT." > "$paks_header"
  echo "#define PAKSET_COUNT ${#paksets[@]}" >> "$paks_header"
  echo "#define OBSOLETE_FROM ($obsolete_start_index)" >> "$paks_header"
  echo "paksetinfo_t pakinfo[PAKSET_COUNT] = {" >> "$paks_header"

  pakcount=0
  for urlname in "${paksets[@]}"
  do
    zipname="${urlname##http*\/}"

    echo "Downloading $zipname"
    do_download "$urlname" "$zipname" || return 1
    downloadcall=""
    skip_in_exe="0"
    destdir=""

    if [ -n "$(file "$zipname" | grep -i "Microsoft Cabinet archive data")" ]; then
      # .cab file
      install_cab "$zipname"
      skip_in_exe="1"
      downloadcall="DownloadInstallCabWithoutSimutrans"
    elif [ -n "$(file "$zipname" | grep -i "gzip compressed data")" ]; then
      # .tar.gz
      install_tgz "$zipname"
      skip_in_exe="1"
      downloadcall="DownloadInstallTgzWithoutSimutrans"
    elif [ -n "$(file "$zipname" | grep -i "Zip archive data")" ]; then
      # .zip
      # First check if we only have a simutrans/ directory at the root.
      files=$(unzip -Z1 "$zipname" | grep "^simutrans/")
      if [ $? -eq 0 ]; then
        # we have simutrans already
        downloadcall="DownloadInstallZip"
        unzip -qoC "$zipname"
      else
        downloadcall="DownloadInstallZipWithoutSimutrans"
        unzip -qoC "$zipname" -d "simutrans"
        echo "without"
      fi
    else
      rm -f "$zipname"
      echo "Error: Cannot extract unknown archive format" >&2
      exit 1
    fi

    size="$(du -s -Bk simutrans | sed 's/K.*$//')"
    echo "Size $size"
    rm -rf simutrans/themes
    rm -rf simutrans/config
    choicename="$(ls simutrans)"
    echo "choicename >$choicename<"
    versionstring=""
    count="$( od -An -tu2 -j 99 -N2 --endian=little simutrans/$choicename/ground.Outside.pak | tr -d ' ')"
    echo "count $count"
    if [ "$count" != "0" ] ; then
      versionstring="$(dd bs=1 skip=101 count=$count if=simutrans/$choicename/ground.Outside.pak status=none)"
      echo "version $versionstring"
    fi

    # paksetinfo.h
    echo $'\t'\{ "\"$urlname\", \"$choicename\", \"$versionstring\", $size }," >> "$paks_header"

    # nsh output

    if ((pakcount == $obsolete_start_index )); then
      # obsolete paks from here
      printf '\n; OBSOLETE PAKS from here\nSectionGroup /e "Not currently developed" slowPakgroup\n\n'  >>  "$nsis_header"
    fi


    if [ "$choicename" == "pak" ]; then
      # sectiongroup for pak64
      printf "SectionGroup /e \"Pak64: main and addons\" pak64group\n\n" >>  "$nsis_header"
    fi

    # normal section
    echo $"Section /o \"$choicename\" $choicename" >> "$nsis_header"
    echo $"  AddSize $size" >>  "$nsis_header"
    echo $"  StrCpy \$downloadlink \"$urlname\"" >>  "$nsis_header"
    echo "  SetOutPath \$PAKDIR" >>  "$nsis_header"
    echo "  StrCpy \$archievename \"$zipname\"" >>  "$nsis_header"
    echo "  StrCpy \$downloadname \"$choicename\"" >>  "$nsis_header"
    echo "  StrCpy \$VersionString \"$versionstring\"" >>  "$nsis_header"
    echo "  Call $downloadcall" >>  "$nsis_header"
    echo "SectionEnd"  >>  "$nsis_header"

    if [ "$choicename" == "pak" ]; then
      # pak64 addons
      printf "Section /o \"pak64 Food addon\"\n  AddSize 228\n  StrCpy \$downloadlink \"http://downloads.sourceforge.net/project/simutrans/pak64/121-0/simupak64-addon-food-120-4.zip\"\n  StrCpy \$archievename \"simupak64-addon-food-120-4.zip\"\n  StrCpy \$downloadname \"pak\"\n" >>  "$nsis_header"
      printf "  StrCpy \$VersionString \"\"\n  StrCmp \$multiuserinstall \"1\" +3\n  ; no multiuser => install in normal directory\n  Call DownloadInstallAddonZipPortable\n  goto +2\n  Call DownloadInstallAddonZip\nSectionEnd\n\n"  >>  "$nsis_header"
      printf "SectionGroupEnd\n\n" >>  "$nsis_header"
    fi
    echo ""  >>  "$nsis_header"

    rm -rf simutrans
    rm -rf $zipname
    mkdir simutrans

    pakcount=$((pakcount+1))

  done
  printf "SectionGroupEnd\n\n" >>  "$nsis_header"
  echo "};" >> "$paks_header"

  cd ..
  rm -rf tmp

  exit 0
fi

pwd | grep "/simutrans$" >/dev/null || {
  echo "Warning install csb-paksets might fail if not in a simutrans/ directory" >&2
}

# first find out, if we have a command options and just install these paks
# parameter is the full path to the pakset
if  [ "$#" -gt 0 ]; then

  for pakname in "$@"
  do
    if [[ $pakname == "http"* ]]; then

      # direct download if whatever
      zipname="${pakname##http*\/}"
      choicename="${zipname%.*}"
      choicename="${choicename%.tar}" # for .tar.gz
      choicename="${choicename/simupak/pak}"

      echo "-- Installing $choicename --"
      tempzipname="$TEMP/$zipname"
      download_and_install_pakset "$urlname" "$tempzipname" || {
        echo "Error installing pakset $choicename"
        exit 1
      }
    else
      # find pakset with this folder name
      has_match="0"
      for urlname in "${paksets[@]}"
      do
        if [[ $urlname == *"/$pakname"/* ]]; then

          zipname="${urlname##http*\/}"
          choicename="${zipname%.*}"
          choicename="${choicename%.tar}" # for .tar.gz
          choicename="${choicename/simupak/pak}"

          echo "-- Installing $choicename --"
          tempzipname="$TEMP/$zipname"
          download_and_install_pakset "$urlname" "$tempzipname" || {
            echo "Error installing pakset $choicename"
            exit 1
          }
          has_match="1"
        fi
      done
      if [[ $has_match == "0" ]]; then
        echo "No pak matches $pakname"
        exit 1
      fi
    fi
  done
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

exit $fail
