#!/bin/sh
#Author: Claudio Bertacchini

# Argument's number check
if [ "$#" -ne 2 ]; then
	echo "Error: missing argument!"
	echo "Usage: $0 <writefile> <writestr>"
	exit 1
fi

writefile=$1
writestr=$2

dir=$(dirname "$writefile")
# folder creation
if [ ! -d $dir ]; then
	echo "Creating folder $dir..."
	if ! mkdir -p "$dir"; then
		echo "Error: cannot create folder $dir"
		exit 1
	fi
fi

#create file and write text into the file
if ! echo "$writestr" > "$writefile"; then
	echo "Error: cannot write data into the file $writefile"
	exit 1
fi

if ! [ -f "$writefile" ]; then
	echo "Error: file does not exists!"
	exit 1
fi

read_content=$(cat "$writefile")
if [ "$read_content" != "$writestr" ]; then
	echo "Error: content does not matches with $writestr"
	exit 1
fi

exit 0

