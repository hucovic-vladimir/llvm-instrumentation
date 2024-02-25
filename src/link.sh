#!/bin/bash
# A linker wrapper script which compiles the instrumentation code with the appropriate
# size of the static array which holds the basic block execution counts
# Invokes the actual linker and adds the compiled inst. code object file to link
# with the rest of the object files.

LINKER=ld
INSTCODE_SRC_PATH="/home/vladimir/Documents/IP1/LLVMPass/src/" 
INSTCODE_OBJ_FILE="./instrumentationCode.o"

read -r bbcount < ./bbcount.tmp
# bbcount=$(awk '{ sum += $1 } END { print sum }' "bbcount.tmp") 

make -C $INSTCODE_SRC_PATH -f Makefile c BBCOUNT=$bbcount
clang -c -o $INSTCODE_OBJ_FILE $INSTCODE_SRC_PATH/instrumentationCode_new.ll

$LINKER "$@" $INSTCODE_OBJ_FILE 
