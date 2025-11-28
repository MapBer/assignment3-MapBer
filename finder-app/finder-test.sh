#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

# Uncomment the following lines to disable make commands
# #Clean any previous build artifacts
# make clean
# #Compile the writer utility as a native application
# make all

set -e
set -u

NUMFILES=10
## Ensure the result file exists so reading it won't fail when the script is run
if [ ! -f /tmp/assignment4-result.txt ]; then
	touch /tmp/assignment4-result.txt
fi

# Load previous finder output (if any) as default write string; this may be
# empty if the result file has no content yet.
WRITESTR=$(cat /tmp/assignment4-result.txt)
WRITEDIR=/tmp/aeld-data
username=$(cat /etc/finder-app/conf/username.txt)

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write"
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write"
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
assignment=`cat /etc/finder-app/conf/assignment.txt`

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$WRITEDIR"

	#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$WRITEDIR" ]
	then
		echo "$WRITEDIR created"
	else
		exit 1
	fi
fi
#echo "Removing the old writer utility and compiling as a native application"
#make clean
#make

for i in $( seq 1 $NUMFILES)
do
	/usr/bin/writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

OUTPUTSTRING=$(/usr/bin/finder.sh "$WRITEDIR" "$WRITESTR")

# Run the finder command and save its output to a result file for grading
# The test harness expects the finder output to be available at /tmp/assignment4-result.txt
./finder.sh "$WRITEDIR" "$WRITESTR" > /tmp/assignment4-result.txt

# Also capture the output in a variable for the existing in-script checks
OUTPUTSTRING=$(cat /tmp/assignment4-result.txt)

# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
	echo "success"
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
	exit 1
fi
