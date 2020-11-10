#!/bin/bash

#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

# Filter for Doxygen source code browsing
gawk -f squirrel_types.awk -f export.awk -v mode=sq $1
