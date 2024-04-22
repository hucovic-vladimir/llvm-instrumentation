override CC=clang
override CFLAGS=-Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3 -flto -fembed-bitcode -std=c2x -fPIC -fpass-plugin=../build/src/libPostInstrumentationPass.so


all:
	clang $(CFLAGS) instrumentationCode_new.c -o instrumentationCode_new.ll
