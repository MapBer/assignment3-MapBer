#!/bin/bash

writefile="$1"
writestr="$2"

if [[ -z "$writefile" || -z "$writestr" ]]; then
	echo "Incorrect input parameters"
	exit 1
#else
#	echo "Proper input parameters"
fi

mkdir -p "$(dirname "$writefile")"

echo "$writestr" > "$writefile"

