#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

#
# script to fetch language files
#
# assumes to be called from main dir "tools/get_lang_files.sh"
#

OUTPUT_DIR=simutrans/text
TRANSLATOR_URL=https://translator.simutrans.com

# get the translations for basis
# The first file is longer, but only because it contains SQL error messages
# - discard it after complete download (although parsing it would give us the archive's name):
# first test which URL actually works

# Use curl if available, else use wget
curl -q -h > /dev/null
if [ $? -eq 0 ]; then
	if ![ curl --head --silent --fail $TRANSLATOR_URL 2> /dev/null ]; then
		TRANSLATOR_URL=https://translator.simutrans.com
	fi
	echo "Using translator at $TRANSLATOR_URL"
    curl -q -L -d "version=0&choice=all&submit=Export%21" $TRANSLATOR_URL/script/main.php?page=wrap > /dev/null || {
      echo "Error: generating file language_pack-Base+texts.zip failed (curl returned $?)" >&2;
      exit 3;
    }
    curl -q -L $TRANSLATOR_URL/data/tab/language_pack-Base+texts.zip > language_pack-Base+texts.zip || {
      echo "Error: download of file language_pack-Base+texts.zip failed (curl returned $?)" >&2
      rm -f "language_pack-Base+texts.zip"
      exit 4
    }
else
    wget -q --help > /dev/null
    if [ $? -eq 0 ]; then
		if ![ wget -q --method=HEAD $TRANSLATOR_URL 2> /dev/null ]; then
			TRANSLATOR_URL=https://translator.simutrans.com
		fi
		echo "Using translator at $TRANSLATOR_URL"
        wget -q --post-data "version=0&choice=all&submit=Export!"  --delete-after $TRANSLATOR_URL/script/main.php?page=wrap || {
          echo "Error: generating file language_pack-Base+texts.zip failed (wget returned $?)" >&2;
          exit 3;
        }
        wget -q -N $TRANSLATOR_URL/data/tab/language_pack-Base+texts.zip || {
          echo "Error: download of file language_pack-Base+texts.zip failed (wget returned $?)" >&2
          rm -f "language_pack-Base+texts.zip"
          exit 4
        }
    else
        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
        exit 6
    fi
fi

unzip -otv "language_pack-Base+texts.zip" -d ${OUTPUT_DIR} || {
   echo "Error: file language_pack-Base+texts.zip seems to be defective" >&2
   rm -f "language_pack-Base+texts.zip"
   exit 5
}
unzip -o "language_pack-Base+texts.zip" -d ${OUTPUT_DIR}
rm language_pack-Base+texts.zip
# remove Chris English (may become simple English ... )
rm -f ${OUTPUT_DIR}/ce.tab
rmdir ${OUTPUT_DIR}/ce
rm -f ${OUTPUT_DIR}/_objectlist.txt
# Remove check test
#rm xx.tab
#rm -rf xx

