#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# This file is needed to compile Simutrans on the OpenTTD compile farm.

# The output is  <REV>\t<REV_NR>\t<MODIFIED>\t<CLEAN_REV>
# where REV is something like "r1234M", REV_NR just the number (without r),
# MODIFIED either 0=pristine, 1=unknown, 2=modified and CLEAN_REV the same
# as rev (since simutrans very rarley uses branches latetly).
# THIS NEEDS TO BE CHANGED WHEN THERE WILL BE BRANCHES AGAIN!

# Assume the dir is not modified
MODIFIED="0"
TEST_SVN=`svnversion`
if [ -n TEST_SVN ]; then
#if [ -d "/.svn" ]; then
	# We are an svn checkout
	if [ -n "`svnversion | grep 'M'`" ]; then
		MODIFIED="2"
	fi
	REV_NR=`svnversion`
	REV="r$REV_NR"
	CLEAN_REV=$REV
	REV_NR=`svnversion | sed "s/[0-9]*://" | sed "s/M.*//"`
else
	# status unknown
	MODIFIED="1"
fi

echo "$REV	$REV_NR	$MODIFIED	$CLEAN_REV"
