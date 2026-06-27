@echo off
set PATH=C:\msys64\mingw32\bin;%PATH%
cd /d C:\Users\bobcumulus\Desktop\bob_compiler\module_07
gcc -Wall -Wextra -Werror -std=c11 -o sema source.c lexer.c symtab.c ast.c parser.c sema.c main.c 1>compile_msys.txt 2>&1
echo EXIT_CODE:%ERRORLEVEL% >> compile_msys.txt
