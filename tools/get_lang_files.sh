#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# updating SVN too?
UPDATE_SVN=0
if [ "$#" -gt 0 ] && [ "$1" = '-svn' ]; then
  echo "Updating SVN too"
  UPDATE_SVN=1
else
  echo "Use option -svn to update the repo."
fi

move_if_differ_add()
{
  pattern="$1"
  dest_path="$2"

  mkdir -p $dest_path
  if [[ $UPDATE_SVN -eq 1 ]]; then
    # only copy changed texts
    for f in $pattern; do
      if [[ -f "$dest_path/$f" ]]; then
        cat $f | grep "^[^#]" > $f.tmp
        a=`cat $dest_path/$f | grep "^[^#]" | diff -wq $f.tmp -`
        if [[ ! -z "$a" ]] ; then
          echo "Updating $f to texts"
          mv $f $dest_path/$f
        fi
      else
        # new file: move it and add it to svn
        echo "Creating $f to texts"
        mv $f $dest_path/$f
        svn add $dest_path/$f
        svn ps svn:eol-style native $dest_path/$f
      fi
    done
  else
    # only copy changed texts
    for f in $pattern; do
      cat $f | grep "^[^#]" > $f.tmp
      a=`cat $dest_path/$f | grep "^[^#]" | diff -wq $f.tmp -`
      if [[ ! -z "$a" ]] ; then
        echo "Updating $f to texts"
        mv $f $dest_path/$f
      fi
    done
  fi
}

move_add()
{
  pattern="$1"
  dest_path="$2"

  if ! [[ -d "$dest_path" ]]; then
    mkdir -p $dest_path
    if [[ $UPDATE_SVN -eq 1 ]];then
      svn add $dest_path
    fi
  fi

  # only copy changed texts
  for f in $pattern; do
    if [[ $UPDATE_SVN -eq 1 ]]  &&  ! [[ -f "$dest_path/$f" ]]; then
      # new file: move it and add it to svn
      echo "Creating $f to texts"
      mv $f $dest_path/$f
      svn add $dest_path/$f
      svn ps svn:eol-style native $dest_path/$f
    else
      mv $f $dest_path/$f
    fi
  done
}

#
# script to fetch language files
#
# assumes to be called from main dir "tools/get_lang_files.sh"
#

OUTPUT_DIR=simutrans/text
TEMP_DIR=lang-tmp
#TRANSLATOR_URL=https://translator.simutrans.com
TRANSLATOR_URL=https://makie.de/translator

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

# create temporary directory
rm -rf  $TEMP_DIR
mkdir  $TEMP_DIR

# test archive
unzip -otv "language_pack-Base+texts.zip" -d $TEMP_DIR || {
   echo "Error: file language_pack-Base+texts.zip seems to be defective" >&2
   rm -f "language_pack-Base+texts.zip"
   exit 5
}
unzip -o "language_pack-Base+texts.zip" -d $TEMP_DIR
rm language_pack-Base+texts.zip

# now we do this inside the tmmp directory
cd $TEMP_DIR

# remove Chris English (may become simple English ... )
rm -f ce.tab
rmdir ce
rm -f _objectlist.txt
# Remove check test
#rm xx.tab
#rm -rf xx

# user credits
mv _translate_users.txt ../$OUTPUT_DIR/translate_users.txt

# only copy changed texts
move_if_differ_add "*.tab" "../$OUTPUT_DIR/"

#if [[ $UPDATE_SVN -eq 1 ]]; then
#  svn ps svn:eol-style native ../$OUTPUT_DIR/*.tab
#fi

# remove the unchanged files (apart from dates and translators)
rm *.tab
rm *.tab.tmp

# now move the help files
for f in *; do
  if [ -z "$(ls -A $f)" ]; then
   echo "Remove empty help files for $f"
  else
    cd $f
    move_add "*.txt" "../../$OUTPUT_DIR/$f"
    cd ..
    rm -rf $f
  fi
done

cd ..
rm -rf  $TEMP_DIR
