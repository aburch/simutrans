#!/bin/bash

#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

# Filter for Doxygen input files
gawk -f squirrel_types.awk -f export.awk -v mode=cpp $1
