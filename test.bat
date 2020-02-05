@echo off
call build.bat
gcc -o bin/test test/*.c -L./bin -lcdtp
bin\test
