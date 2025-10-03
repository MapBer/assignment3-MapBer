#!/bin/bash
filesdir="$1"
searchstr="$2"
if [[ -z "$filesdir" || -z "$searchstr" ]]; then
	echo "Incorrect input parameters"
	exit 1
else
	echo "Correct input parameters"
fi

if [ -d "$1" ]; then
	echo "Directory exists"
	echo "$filesdir"
	echo "$searchstr"
else
	echo "No such directory"
	exit 1
fi

totalFiles=0
totalLines=0

for file in $(find "$filesdir" -type f); do
	if [ -f "$file" ]; then
		echo "$file"
		count=$(grep -c "$searchstr" "$file")
		totalLines=$((totalLines + count))
		echo "$count"
		totalFiles=$((totalFiles + 1))
	fi
done

echo "The number of files are ${totalFiles}  and the number of matching lines are ${totalLines}"
exit 0
