.PHONY: all build test clean

CC = gcc
ARCHIVER = ar
SOURCES = $(wildcard src/*.c)
TEST = false
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
	NULL_CMD = cd.
	INCLUDE_FLAGS = -I"C:\Program Files\OpenSSL-Win64\include"
	LINK_FLAGS_STATIC = -lWs2_32 -L"C:\Program Files\OpenSSL-Win64\lib" -l:libcrypto.lib
	LINK_FLAGS_SHARED = -lWs2_32 -L"C:\Program Files\OpenSSL-Win64\bin" -l:libcrypto-3-x64.dll
	STATIC_LIB = cdtp.lib
	SHARED_LIB = cdtp.dll
	BUILD_STATIC_OUT = bin/$(STATIC_LIB)
	BUILD_SHARED_OUT = bin/$(SHARED_LIB)
	MOVE_OBJECTS = move /y *.o bin >NUL
	CLEAN_OBJECTS = del bin\*.o
	TEST_BINARY_STATIC = bin\test-static
	TEST_BINARY_SHARED = bin\test-shared
	POST_BUILD_CMD = $(NULL_CMD)
	CLEAN_CMD = del bin\*.a bin\*.so bin\*.lib bin\*.dll bin\test-* bin\test-*.exe bin\*.o *.o
else
	NULL_CMD = :
	INCLUDE_FLAGS =
	LINK_FLAGS_STATIC = -lpthread -L/usr/src/openssl-3.0.7 -l:libcrypto.a -ldl
	LINK_FLAGS_SHARED = -lpthread -L/usr/src/openssl-3.0.7 -l:libcrypto.so.3
	STATIC_LIB = libcdtp.a
	SHARED_LIB = libcdtp.so
	BUILD_STATIC_OUT = bin/$(STATIC_LIB)
	BUILD_SHARED_OUT = bin/$(SHARED_LIB)
	MOVE_OBJECTS = mv *.o bin/
	CLEAN_OBJECTS = rm -f bin/*.o
	TEST_BINARY_STATIC = ./bin/test-static
	TEST_BINARY_SHARED = ./bin/test-shared
	POST_BUILD_CMD = chmod +x ./bin/test-static ./bin/test-shared
	CLEAN_CMD = rm -f bin/*.a bin/*.so bin/*.lib bin/*.dll bin/test-* bin/test-*.exe bin/*.o *.o
endif

ifeq ($(TEST),true)
	BUILD_DIRECTIVES = -DCDTP_TEST
	BUILD_TEST_BINARY_CMD = \
		$(CC) -o bin/test-static \
			$(INCLUDE_FLAGS) \
			$(BUILD_FLAGS) \
			test/*.c -L./bin -Wl,-rpath=./bin -l:$(STATIC_LIB) $(LINK_FLAGS_STATIC) && \
		$(CC) -o bin/test-shared \
			$(INCLUDE_FLAGS) \
			$(BUILD_FLAGS) \
			test/*.c -L./bin -Wl,-rpath=./bin -l:$(SHARED_LIB) && \
		$(POST_BUILD_CMD)
else
	BUILD_DIRECTIVES =
	BUILD_TEST_BINARY_CMD = $(NULL_CMD)
endif

all: build

build:
	$(CC) -c \
		-fPIC \
		$(INCLUDE_FLAGS) \
		$(BUILD_FLAGS) \
		$(BUILD_DIRECTIVES) \
		$(SOURCES) && \
	$(MOVE_OBJECTS) && \
	$(ARCHIVER) rcs $(BUILD_STATIC_OUT) \
		bin/*.o && \
	$(CC) -o $(BUILD_SHARED_OUT) \
		-fPIC \
		-shared \
		$(BUILD_FLAGS) \
		bin/*.o \
		$(LINK_FLAGS_SHARED) && \
	$(BUILD_TEST_BINARY_CMD) && \
	$(CLEAN_OBJECTS)

test:
	$(TEST_BINARY_STATIC) && $(TEST_BINARY_SHARED)

clean:
	$(CLEAN_CMD)
