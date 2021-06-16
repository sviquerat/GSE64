@ECHO OFF
md log >nul 2>&1
md bin >nul 2>&1
del /q log\* 2>&1
del /q bin\* 2>&1

echo "Compiling GSEMERGE..."
gcc -Wall -o bin/GSE64.exe GSE64.c 2> log/build64.log
pause