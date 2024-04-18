all:
	clang -Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3 -flto -fembed-bitcode instrumentationCode_new.c -o instrumentationCode_new.ll -std=c2x -fPIC -fpass-plugin=../build/src/libPostInstrumentationPass.so
