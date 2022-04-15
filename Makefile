.PHONY: all build test clean

CC = gcc
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OBJECTS = $(wildcard bin/obj/*.o)
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
	LINK_FLAGS = -lWs2_32
	BUILD_SHARED_OUT = bin/cdtp.dll
	MOVE_OBJECTS = move /y *.o bin\obj
	COPY_HEADERS = xcopy /s /y src\*.h bin\include >NUL
	TEST_BINARY = bin\test
	POST_BUILD_CMD = cd.
	CLEAN_CMD = del bin\libcdtp.so bin\cdtp bin\cdtp.dll bin\cdtp.exe bin\test bin\test.exe bin\include\*.h bin\obj\*.o
else
	LINK_FLAGS = -lpthread -lm
	BUILD_SHARED_OUT = bin/libcdtp.so
	MOVE_OBJECTS = mv *.o bin/obj/
	COPY_HEADERS = cp src/*.h bin/include/
	TEST_BINARY = ./bin/test
	POST_BUILD_CMD = chmod +x ./bin/test
	CLEAN_CMD = rm -f bin/libcdtp.so bin/cdtp bin/cdtp.dll bin/cdtp.exe bin/test bin/test.exe bin/include/*.h bin/obj/*.o
endif

all: build

build: $(SOURCES)
	$(CC) -c \
		-fpic \
		$(BUILD_FLAGS) \
		$(SOURCES) && \
	$(MOVE_OBJECTS) && \
	$(CC) -o $(BUILD_SHARED_OUT) \
		-shared \
		$(BUILD_FLAGS) \
		bin/obj/*.o \
		$(LINK_FLAGS) && \
	$(COPY_HEADERS) && \
	$(CC) -o bin/test \
		$(BUILD_FLAGS) \
		test/*.c -L./bin -Wl,-rpath=./bin -lcdtp && \
	$(POST_BUILD_CMD)

test:
	$(TEST_BINARY)

clean:
	$(CLEAN_CMD)
