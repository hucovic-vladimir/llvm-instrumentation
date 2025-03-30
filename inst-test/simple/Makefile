UNAME_S := $(shell uname -s)

CC=/Users/vladimirhucovic/school/pp1/llvm-project/build-dylib2/bin/clang
CFLAGS= -g -c -fpass-plugin=/Users/vladimirhucovic/school/pp1/llvm-instrumentation/build/src/libInstructionCount.dylib 
LDFLAGS= -fuse-ld="/Users/vladimirhucovic/school/pp1/llvm-instrumentation/src/link.sh" 

ifeq ($(UNAME_S),Darwin)
    SDK_PATH := $(shell xcrun --show-sdk-path)
    override CFLAGS += -isystem $(SDK_PATH)/usr/include
		override LDFLAGS += -L$(SDK_PATH)/lib
endif

all:
	$(CC) test.c $(CFLAGS) -o test.o
	$(CC) $(LDFLAGS) test.o -o test 

clean:
	rm -rf *.o test .basicblocks .patterns modules.tmp profile_data_pid_*.json

