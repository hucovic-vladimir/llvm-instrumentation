#!/bin/bash
# A linker wrapper script which compiles the instrumentation code before 
# linking starts and then links the compiled instrumentation code with the program
# the INSTCODE_SRC_PATH variable should be set to the directory with the instrumentation code.
# Afterwards, this script should be passed to clang as a linker script.

LINKER=ld.lld
INSTCODE_SRC_PATH="/home/vladimir/Documents/llvm-instrumentation/src"
INSTCODE_OBJ_FILE="./instrumentationCode.o"

object_files=()

unset CFLAGS

# Iterate over arguments
# for arg in "$@"; do
#     if $output_flag; then
#         linked_binary="$arg"
#         output_flag=false
#     elif [[ $arg == -o ]]; then
#         output_flag=true
#     elif [[ $arg == *.o && ! $arg == */lib/*.o ]]; then
#         object_files+=("$arg")
#     fi
# done

# echo "Object files: ${object_files[@]}"
# echo "Output file: $linked_binary"

# for file in "${object_files[@]}"; do
#     output_file="$file.dis.ll"
#     llvm-dis "$file" -o "$output_file"
#     dis_output_files+=("$output_file")
# done

# echo "Disassembled output files: ${dis_output_files[@]}"

# llvm-link ${dis_output_files[@]} -S -o ./"${linked_binary}_linked.ll"


cp ./modules.tmp $INSTCODE_SRC_PATH/modules.tmp
clang -Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3 -flto -fembed-bitcode ${INSTCODE_SRC_PATH}/instrumentationCode_new.c -o\
	${INSTCODE_SRC_PATH}/instrumentationCode_new.ll -std=c2x -fPIC -fpass-plugin=/home/vladimir/Documents/llvm-instrumentation/build/src/libPostInstrumentationPass.so
clang -c -o $INSTCODE_OBJ_FILE $INSTCODE_SRC_PATH/instrumentationCode_new.ll -O3

$LINKER "$@" $INSTCODE_OBJ_FILE 
