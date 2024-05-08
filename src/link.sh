#!/bin/bash
# A linker wrapper script which compiles the instrumentation code before 
# linking starts and then links the compiled instrumentation code with the program
# This script should be passed to clang as a linker script.

LINKER=ld.lld
INSTCODE_SRC_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"
INSTCODE_OBJ_FILE=${INSTCODE_SRC_PATH}"/instrumentationCode.o"

cp ./modules.tmp $INSTCODE_SRC_PATH/modules.tmp
clang -Wall -Wextra -Werror -O3 -S -emit-llvm -g3 ${INSTCODE_SRC_PATH}/instrumentationCode_new.c -o\
	${INSTCODE_SRC_PATH}/instrumentationCode_new.ll -std=c2x -fPIC -fpass-plugin=${INSTCODE_SRC_PATH}/../build/src/libPostInstrumentationPass.so
clang -c -o $INSTCODE_OBJ_FILE $INSTCODE_SRC_PATH/instrumentationCode_new.ll -O3

$LINKER "$@" $INSTCODE_OBJ_FILE 

