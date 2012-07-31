#!/bin/bash
#
# script to fetch pak sets
#

# parameter: url and filename
do_download(){
curl -h > /dev/null
if [ $? -eq 0 ]; then
    curl $1 > $2 || {
      echo "Error: download of file $2 failed (curl returned $?)" >&2
      rm -f $2
      exit 4
    }
else
    wget --help > /dev/null
    if [ $? -eq 0 ]; then
        wget -q -N $1 -O $2 || {
          echo "Error: download of file $2 failed (wget returned $?)" >&2
          rm -f $2
          exit 4
        }
    else
        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
        exit 6
    fi
fi
}


# two parameter, url, zipfilename
DownloadInstallZip(){
  echo "downloading from $1"
  do_download $1 $TEMP/$2
  echo "installing from $2"
  unzip -o -C -q $TEMP/$2 -d .
  rm $TEMP/$2
}


# generated list of pak sets with well-formatted paking
paksets=( \
"http://downloads.sourceforge.net/project/simutrans/pak64/111-3/simupak64-111-3.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak.german/pak64.german-110-0c/simupak-german64-110-0c.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak64.japan/111-3/simupak64.japan-111-3.zip" \
"http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20111-2/pak128-2.1.0--111.2.zip" \
"http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_0.4_111.3/PAK128.german_0.4_111.3.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192.comic_102-2-1/pak192.comic_0-1-9-1_102-2-1.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip" \
)

# these sets are not in a simutrans folder in the zip
malformedpaksets=( \
"http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip" \
"http://hd.simutrans.com/release/PakHD_v04B_100-0.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20111-0/pak128.Britain.1.11-111-2.zip" \
"http://downloads.sourceforge.net/project/simutrans/pak128.japan/for%20Simutrans%20110.0.1/pak128.japan-110.0.1-version16-08-2011.zip" \
"http://downloads.sourceforge.net/project/ironsimu/pak48.Excentrique/v018/pak48-excentrique_v018.zip" \
)
# not a zip
#"http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz" \

pushd ..
let setcount=0
let "maxcount = ${#paksets[*]}"
while [ $setcount -lt $maxcount ]; do
  while true; do
      urlname=${paksets[$setcount]}
      zipname="${urlname##http*\/}"
      let "setcount += 1"
      echo $zipname
      read -p "Do you wish to install $zipname ? (y/n/x)" yn
      case $yn in
          [Yy]* ) DownloadInstallZip $urlname $zipname; break;;
          [Nn]* ) break;;
          [Xx]* ) exit;;
          * ) if [ $setcount -lt 2 ]; then
            DownloadInstallZip $urlname $zipname;
           fi
           break;;
      esac
  done
done

#for the wrong paksets, just change to simutrans folder
popd
let setcount=0
let "maxcount = ${#malformedpaksets[*]}"
while [ $setcount -lt $maxcount ]; do
  while true; do
      urlname=${malformedpaksets[$setcount]}
      zipname="${urlname##http*\/}"
      let "setcount += 1"
      read -p "Do you wish to install $zipname ? (y/n/x)" yn
      case $yn in
          [Yy]* ) DownloadInstallZip $urlname $zipname; break;;
          [Nn]* ) break;;
          [Xx]* ) exit;;
          * ) if [ $setcount -lt 2 ]; then
            DownloadInstallZip $urlname $zipname;
           fi
           break;;
      esac
  done
 done
exit
