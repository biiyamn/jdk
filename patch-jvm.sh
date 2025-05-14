#!/bin/bash
#change visibility to allow symbols to be exported
FILE="./make/autoconf/flags-cflags.m4"

if [[ -f "$FILE" ]]; then
  sed -i 's/-fvisibility=hidden/-fvisibility=default/g' "$FILE"
  echo "Updated -fvisibility=hidden to -fvisibility=default in $FILE"
else
  echo "File $FILE does not exist."
fi


SRC="mapfile.txt"
DEST="./make/data/hotspot-symbols/symbols-shared"

if [[ -f "$SRC" && -f "$DEST" ]]; then
  cat "$SRC" >> "$DEST"
  echo "Appended content of $SRC to $DEST"
else
  echo "Either $SRC or $DEST does not exist."
fi
#
# comment CHECK_UNHANDLED_OOPS in case of fastdebug , so it is disabled,
# otherwise undefined symbol: ZNK13CollectedHeap13containsnullEPKP7oopDesc prevent compilation and execution of java
# Detect the debug level from any spec.gmk file
DEBUG_LEVEL=$(grep '^DEBUG_LEVEL\s*:=' build/*/spec.gmk | awk -F ':=' '{print $2}' | tr -d ' ')

if [[ "$DEBUG_LEVEL" == "fastdebug" ]]; then
  echo "Detected DEBUG_LEVEL=fastdebug — disabling -DCHECK_UNHANDLED_OOPS..."
  sed -i '/else ifeq\s*(\$(DEBUG_LEVEL), fastdebug)/,/endif/ {
    /JVM_CFLAGS_DEBUGLEVEL\s*+=\s*-DCHECK_UNHANDLED_OOPS/ s/^/# /
  }' make/hotspot/lib/JvmFlags.gmk
  echo "Line commented."
else
  echo "DEBUG_LEVEL is '$DEBUG_LEVEL' — no change made."
fi
