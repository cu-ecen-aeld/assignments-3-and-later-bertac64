#!/bin/sh
#Authoir Claudio Bertacchini

# Argument's number check
if [ "$#" -ne 2 ]; then
	echo "Error: missing argument!"
	echo "Usage: $0 <filesdir> <searchstr>"
	exit 1
fi

filesdir=$1
searchstr=$2

# Check if first argument is a directory
if [ ! -d "$filesdir" ]; then
	echo "Error $filesdir is not a folder"
	exit 1
fi

# Counting files and rows
num_files=$(find "$filesdir" -type f | wc -l)
num_lines=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

echo " The number of files are $num_files and the number of matching lines are $num_lines"
exit 0
