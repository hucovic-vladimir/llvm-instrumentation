# Prerequisites
- LLVM toolchain (tested on version 17.0.0)
- CMake
- GNU Make / Ninja / other build system compatible with CMake

# Building 

1. Set the LLVM_INSTALL_DIR and LLVM_INCLUDE_DIR variables in CMakeLists.txt

2. `mkdir build && cd build && cmake .. && make`

# Usage
The compiled library libInstructionCount.so can be used as a plugin for `opt` using the option `-load-pass-plugin=`
or using `clang` with `-fpass-plugin=`.

The pass inserts calls to instrumentation code functions located in `src/instrumentationCode_new.c`. This code needs to be compiled
with a defined macro `BBCOUNT` which should contain the number of basic blocks in the profiled program. The pass automatically records
the number of basic blocks in the `bbcount.tmp` file - after compiling and instrumenting all your files, you can pass the number in this file
to the compiler when compiling the instrumentation code. Then you can link the instrumentation code with your program.

The pass also records the information (like ID, instruction count or source code location info) of instrumented basic blocks into the `bbinfo.csv` file.

Run your instrumented program. The profiled data is dumped into `profile_data.txt`. Any errors are written to `profiling_errors.log`.
The format of the profile data is a simple text file with the ID of a basic block and the number of executions, separated by a colon.

## Projects using build systems
If you are using a build system, you can easily integrate this tool into your project. Change the compiler to `clang`, add the previously mentioned `-fpass-plugin` option
to `CFLAGS` and tell `clang` to use `link.sh` as the linker script with `-B` (CFLAGS) and `-fuse-ld=` (LDFLAGS) options.
