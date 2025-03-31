#!/bin/bash
# A linker wrapper script which links the pre-compiled instrumentation code with the program
# This script should be passed to clang as a linker script.

LINKER=ld64.lld 
SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"
INSTCODE_OBJ_FILE=${SCRIPT_PATH}"/instrumentationCode.o"

echo $INSTCODE_OBJ_FILE

# Save modules.tmp if it exists
if [ -f ./modules.tmp ]; then
    cp ./modules.tmp $SCRIPT_PATH/modules.tmp
fi

make clean -C ${SCRIPT_PATH}
make -C ${SCRIPT_PATH}

# Check if the instrumentation object file exists
if [ ! -f "$INSTCODE_OBJ_FILE" ]; then
    echo "Error: Instrumentation object file not found at $INSTCODE_OBJ_FILE"
    echo "Compiling of instrumentation code probably failed."
    exit 1
fi

# On macOS, get the SDK path for linking
if [ "$(uname)" == "Darwin" ]; then
    SDK_LIB="$(xcrun --show-sdk-path)/usr/lib"
    SDK_ARGS="-L$SDK_LIB"
else
    SDK_ARGS=""
fi

# Run the actual linker
$LINKER "$@" $SDK_ARGS $INSTCODE_OBJ_FILE

# Cleanup
if [ -f "$SCRIPT_PATH/modules.tmp" ]; then
    rm "$SCRIPT_PATH/modules.tmp"
fi
