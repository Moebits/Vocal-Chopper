#!/bin/bash

BINARY_BUILDER=~/Documents/JUCE/extras/BinaryBuilder/Builds/MacOSX/build/Debug/BinaryBuilder

WEBVIEW_ZIP="build/webview_files.zip"
CHOP_SCRIPT="python/chopper.py"
OUTPUT_DIR="editor"
CLASS_NAME="BinaryData"

mkdir -p "$OUTPUT_DIR"

TMP_DIR=$(mktemp -d)
cp "$WEBVIEW_ZIP" "$TMP_DIR/"
cp "$CHOP_SCRIPT" "$TMP_DIR/"

"$BINARY_BUILDER" "$TMP_DIR" "$OUTPUT_DIR" "$CLASS_NAME"

rm -rf "$TMP_DIR"

echo " - $OUTPUT_DIR/${CLASS_NAME}.cpp"
echo " - $OUTPUT_DIR/${CLASS_NAME}.h"