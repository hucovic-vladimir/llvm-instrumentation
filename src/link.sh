#!/bin/bash
# A linker wrapper script which compiles the instrumentation code before 
# linking starts and then links the compiled instrumentation code with the program
# the INSTCODE_SRC_PATH variable should be set to the directory with the instrumentation code.
# Afterwards, this script should be passed to clang as a linker script.


LINKER=ld
INSTCODE_SRC_PATH="/home/vladimir/Documents/llvm-instrumentation/src"
INSTCODE_OBJ_FILE="./instrumentationCode.o"

object_files=()

for arg in "$@"; do
		if [[ $arg == *.o && ! $arg == *lib*.o ]]; then
				object_files+=($arg)
		fi
done

echo "Object files: ${object_files[@]}"

cp ./modules.tmp $INSTCODE_SRC_PATH/modules.tmp
env -i make -C $INSTCODE_SRC_PATH -f Makefile hashtable
clang++ -static -c -o $INSTCODE_OBJ_FILE $INSTCODE_SRC_PATH/instrumentationCode_new.ll -O3

$LINKER "$@" $INSTCODE_OBJ_FILE 
