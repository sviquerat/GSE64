@ECHO OFF
md log >nul 2>&1
md bin >nul 2>&1
del /q log\* 2>&1
del /q bin\* 2>&1

echo "Compiling GSEMERGE..."
gcc -Wall -o bin/GSE64.exe GSEMERGE.c 2> log/build64_log.txt
gcc -Wall -o bin/GSE64_DYN.exe GSEMERGE_DYN_INTERVAL.c 2> log/build64_log_dynamic.txt
rem pause