#!/bin/bash

# Filter for Doxygen source code browsing
gawk -f squirrel_types_scenario.awk -f squirrel_types_ai.awk -f export.awk -v mode=sq $1
