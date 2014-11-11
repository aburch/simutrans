#!/bin/bash
#
# script to fetch language files
#
cd simutrans/text
# get the translations for basis
# The first file is longer, but only because it contains SQL error messages
# - discard it after complete download (although parsing it would give us the archive's name):

# Use curl if available, else use wget
curl -q -h > /dev/null
if [ $? -eq 0 ]; then
    curl -q -d "version=0&choice=all&submit=Export%21" http://simutrans-germany.com/translator/script/main.php?page=wrap > /dev/null || {
      echo "Error: generating file language_pack-Base+texts.zip failed (curl returned $?)" >&2;
      exit 3;
    }
    curl -q http://simutrans-germany.com/translator/data/tab/language_pack-Base+texts.zip > language_pack-Base+texts.zip || {
      echo "Error: download of file language_pack-Base+texts.zip failed (curl returned $?)" >&2
      rm -f "language_pack-Base+texts.zip"
      exit 4
    }
else
    wget -q --help > /dev/null
    if [ $? -eq 0 ]; then
        wget -q --post-data "version=0&choice=all&submit=Export!"  --delete-after http://simutrans-germany.com/translator/script/main.php?page=wrap || {
          echo "Error: generating file language_pack-Base+texts.zip failed (wget returned $?)" >&2;
          exit 3;
        }
        wget -q -N http://simutrans-germany.com/translator/data/tab/language_pack-Base+texts.zip || {
          echo "Error: download of file language_pack-Base+texts.zip failed (wget returned $?)" >&2
          rm -f "language_pack-Base+texts.zip"
          exit 4
        }
    else
        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
        exit 6
    fi
fi
unzip -otv "language_pack-Base+texts.zip" || {
   echo "Error: file language_pack-Base+texts.zip seems to be defective" >&2
   rm -f "language_pack-Base+texts.zip"
   exit 5
}
unzip -o "language_pack-Base+texts.zip"
rm language_pack-Base+texts.zip
# remove Chris English (may become british ... )
rm -f ce.tab
# Remove check test
#rm xx.tab
#rm -rf xx
cd ../..
