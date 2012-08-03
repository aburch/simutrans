#!/bin/bash

# Filter for Doxygen input files
gawk -f squirrel_types.awk -f export.awk -v mode=cpp $1
