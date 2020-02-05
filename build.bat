@echo off
gcc -shared -o bin/cdtp.dll src/*.c -lws2_32
mkdir bin\include 2>NUL
xcopy /s /y src\*.h bin\include >NUL
