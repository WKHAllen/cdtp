./build.sh
gcc -L./bin -Wl,-rpath=./bin -Wall -o ./bin/test test/*.c -lcdtp
chmod +x ./bin/test
./bin/test
