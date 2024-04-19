CFLAGS = -Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3

all:
	clang++ $(CFLAGS) instrumentationCode_new.c -o instrumentationCode_new.ll -std=c2x -fPIC -fpass-plugin=../build/src/libPostInstrumentationPass.so

hashtable:
	clang++ $(CFLAGS) -static instrumentationCode.cpp -o instrumentationCode_new.ll -fPIC
