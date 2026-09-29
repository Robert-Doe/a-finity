@echo off
cd /d C:\Users\bobcumulus\Desktop\bob_compiler\module_07
C:\msys64\mingw32\bin\gcc.exe -Wall -Wextra -Werror -std=c11 -o sema source.c lexer.c symtab.c ast.c parser.c sema.c main.c > compile_out.txt 2>&1
echo EXIT:%ERRORLEVEL% >> compile_out.txt
