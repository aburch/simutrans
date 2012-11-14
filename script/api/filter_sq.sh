#!/bin/bash

# Filter for Doxygen source code browsing
gawk -f squirrel_types.awk -f export.awk -v mode=sq $1
