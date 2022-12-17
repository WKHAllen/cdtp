.PHONY: all build test clean

CC = gcc
SOURCES = $(wildcard src/*.c)
BUILD_FLAGS = \
	-std=gnu11 -pedantic -Wall \
	-Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
	-Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
	-Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
	-Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
	-Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls \
	-Wold-style-definition -Wno-pedantic-ms-format -Werror \
	-g -O0 \
	-fno-omit-frame-pointer -ffloat-store -fno-common

ifeq ($(OS),Windows_NT)
	INCLUDE_FLAGS = -I"C:\Program Files\OpenSSL-Win64\include"
	LINK_FLAGS = -lWs2_32 -L"C:\Program Files\OpenSSL-Win64\bin" -llibcrypto-3-x64
	BUILD_SHARED_OUT = bin/cdtp.dll
	MOVE_OBJECTS = move /y *.o bin >NUL
	CLEAN_OBJECTS = del bin\*.o
	TEST_BINARY = bin\test
	POST_BUILD_CMD = cd.
	CLEAN_CMD = del bin\libcdtp.so bin\cdtp.dll bin\test bin\test.exe bin\*.o *.o
else
	INCLUDE_FLAGS =
	LINK_FLAGS = -lpthread -L/usr/src/openssl-3.0.7 -l:libcrypto.so.3
	BUILD_SHARED_OUT = bin/libcdtp.so
	MOVE_OBJECTS = mv *.o bin/
	CLEAN_OBJECTS = rm -f bin/*.o
	TEST_BINARY = ./bin/test
	POST_BUILD_CMD = chmod +x ./bin/test
	CLEAN_CMD = rm -f bin/libcdtp.so bin/cdtp.dll bin/test bin/test.exe bin/*.o *.o
endif

all: build

build:
	$(CC) -c \
		-fPIC \
		$(INCLUDE_FLAGS) \
		$(BUILD_FLAGS) \
		$(SOURCES) && \
	$(MOVE_OBJECTS) && \
	$(CC) -o $(BUILD_SHARED_OUT) \
		-fPIC \
		-shared \
		$(BUILD_FLAGS) \
		bin/*.o \
		$(LINK_FLAGS) && \
	$(CC) -o bin/test \
		$(INCLUDE_FLAGS) \
		$(BUILD_FLAGS) \
		test/*.c -L./bin -Wl,-rpath=./bin -lcdtp && \
	$(POST_BUILD_CMD) && \
	$(CLEAN_OBJECTS)

test:
	$(TEST_BINARY)

clean:
	$(CLEAN_CMD)
