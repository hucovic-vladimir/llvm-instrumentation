UNAME_S := $(shell uname -s)
SCRIPT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

override CC=clang
override CFLAGS=-Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3 -flto -fembed-bitcode -std=c2x -fPIC

ifeq ($(UNAME_S),Darwin)
    SDK_PATH := $(shell xcrun --show-sdk-path)
    override CFLAGS += -isystem $(SDK_PATH)/usr/include
    PLUGIN_EXT = .dylib
else
    PLUGIN_EXT = .so
endif

override CFLAGS += -fpass-plugin=$(SCRIPT_DIR)/../build/src/libPostInstrumentationPass$(PLUGIN_EXT) 

# Paths for instrumentation code
INST_CODE_SRC = $(SCRIPT_DIR)/instrumentationCode_new.c
INST_CODE_LL = $(SCRIPT_DIR)/instrumentationCode_new.ll
INST_CODE_OBJ = $(SCRIPT_DIR)/instrumentationCode.o

all: $(INST_CODE_OBJ)
	@echo "Instrumentation code compiled successfully"

# Rule to generate the LLVM IR file
$(INST_CODE_LL): $(INST_CODE_SRC)
	$(CC) $(CFLAGS) $< -o $@

# Rule to generate the object file from LLVM IR
$(INST_CODE_OBJ): $(INST_CODE_LL)
	$(CC) -c -o $@ $< -O3

clean:
	rm -f $(INST_CODE_LL) $(INST_CODE_OBJ)
