# For now, used to compile and instrument CPython code
# Might not be necessary in the future, but could be expanded to
# include other functionality in case it is needed

import sys
import os

PASS_LIB_PATH = "/home/vladimir/Documents/llvm-instrumentation/build/src/libInstructionCount.so"

def configure(compile_commands):
    os.system(compile_commands)

def main():
    if(len(sys.argv) == 1 or sys.argv[1] == "instrument"):
        configure(f"make -j8 CC='clang -B/home/vladimir/cpython/cpython' CXX=clang++ CFLAGS='-fpass-plugin={PASS_LIB_PATH} -fno-discard-value-names'\
                         LDFLAGS='-fuse-ld=/home/vladimir/Documents/llvm-instrumentation/src/link.sh'")
    elif(sys.argv[1] == "clean"):
        os.system("make clean")
        os.system("rm -rf .basicblocks/* .patterns/* modules.tmp .llfiles/*")

if __name__ == "__main__":
    main()
