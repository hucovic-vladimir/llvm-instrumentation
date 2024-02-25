# For now, used to compile and instrument CPython code
# Might not be necessary in the future, but could be expanded to
# include other functionality in case it is needed

import sys
import os

BBCOUNT_PATH = "bbcount.tmp"
PASS_LIB_PATH = "/home/vladimir/Documents/IP1/LLVMPass/build/src/libInstructionCount.so"
INST_CODE_LIB_PATH = "/home/vladimir/Documents/IP1/LLVMPass/src/"
INST_CODE_LIB_NAME = "instrumentationCode"

def compileAndInstrument(compile_commands):
    os.system(compile_commands)


def removeBBCountFile():
    os.remove(BBCOUNT_PATH)

def main():
    compileAndInstrument(f"./configure CC='clang -B/home/vladimir/cpython/cpython' CXX=clang++ CFLAGS='-fpass-plugin={PASS_LIB_PATH}'\
                         LDFLAGS='-fuse-ld=/home/vladimir/cpython/cpython/link.sh'")
    removeBBCountFile()


if __name__ == "__main__":
    main()
