#!/bin/bash

# Define variables
COMMIT_HASH="9e0f9440d1396de15164fc5d787074dd1e6d372e"
DEST_USER="android"
DEST_HOST="10.150.21.89"
DEST_PATH="android/caf/t/external/perfetto/"
TMP_FILE_LIST="/tmp/rsync-file-list.txt"

# Generate the list of changed files since the specified commit
git diff --name-only $COMMIT_HASH > $TMP_FILE_LIST

# Check if the file list is empty
if [ ! -s $TMP_FILE_LIST ]; then
    echo "No files have changed. Exiting."
    rm -f $TMP_FILE_LIST
    exit 0
fi

# Use rsync to copy those files, excluding specific patterns
rsync -avz --exclude-from='rsync-exclude.txt' --files-from=$TMP_FILE_LIST -e ssh . ${DEST_USER}@${DEST_HOST}:${DEST_PATH}

# Clean up
rm -f $TMP_FILE_LIST

echo "Synchronization complete."

