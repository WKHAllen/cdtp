mkdir bin/obj 2>/dev/null
gcc -c -Wall -Werror -fpic src/*.c
mv ./*.o bin/obj/
gcc -shared -o bin/libcdtp.so bin/obj/*.o -lpthread
mkdir bin/include 2>/dev/null
cp src/*.h bin/include/
