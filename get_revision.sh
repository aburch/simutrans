#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

#
# script to the revision number and put it into the file revision.h
#

svn info 2>/dev/null 1>/dev/null
if [ $? -eq 0 ]; then
  # using svn for verything
  REV=$(svnversion)
  HASH=$REV
else
  # try using git
  git describe --always 2>/dev/null 1>/dev/null
  if [ $? -eq 0 ]; then
    REV=$(git log -1 | grep "git-svn-id" | sed "s/^.*trunk\@//" | sed "s/ .*$//")
    if [ -z "$REV" ]; then
      # make preudo rev from git
      REV=$(git rev-list --count --first-parent HEAD)
      REV=`expr $REV + 328`
    fi
  else
    REV=""
  fi
fi

echo $REV