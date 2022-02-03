#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# Filter for Doxygen input files
gawk -f squirrel_types_scenario.awk -f squirrel_types_ai.awk -f export.awk -v mode=cpp $1
