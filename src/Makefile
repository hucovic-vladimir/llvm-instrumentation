# TODO rework this entire thing

CFLAGS = -Wall -Wextra -Werror -O3 -march=native -S -emit-llvm -g3
CFLAGS_LIB = -Wall -Wextra -Werror -O3 -march=native -g3 -c
BBCOUNT = 0

all: 
	clang++ $(CFLAGS) instrumentationCode.cpp -o instrumentationCode.ll

v2:
	@echo $(BBCOUNT)
	clang++ $(CFLAGS) -DBBCOUNT=$(BBCOUNT) instrumentationCode2.cpp -static -o instrumentationCode2.ll

clean:
	rm -f instrumentationCode.ll


lib:
	clang++ $(CFLAGS_LIB) -o instrumentationCode.bc instrumentationCode.cpp
	ar rcs libinstrumentationCode.a instrumentationCode.bc

lib-c:
	clang $(CFLAGS_LIB) -o instrumentationCode.bc instrumentationCode_new.c
	ar rcs libinstrumentationCode.a instrumentationCode.bc

c:
	clang $(CFLAGS) instrumentationCode_new.c -o instrumentationCode_new.ll -DBBCOUNT=$(BBCOUNT) -std=c2x -fPIC
