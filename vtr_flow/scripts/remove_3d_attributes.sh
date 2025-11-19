#!/usr/bin/env bash

# Exit if any command fails
set -e

# Check that a file was provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

FILE="$1"

# Check file existence
if [ ! -f "$FILE" ]; then
    echo "Error: File '$FILE' not found."
    exit 1
fi

# Remove all occurrences in-place
sed -i \
    -e 's/layer="0"//g' \
    -e 's/layer_low="0"//g' \
    -e 's/layer_high="0"//g' \
    "$FILE"

echo "Removed layer attributes from '$FILE'."
