@echo off
cd /d C:\Users\bobcumulus\Desktop\bob_compiler\module_07
"C:\Program Files\JetBrains\CLion 2023.3.2\bin\mingw\bin\gcc.exe" -Wall -Wextra -Werror -std=c11 -o sema64 source.c lexer.c symtab.c ast.c parser.c sema.c main.c > compile64_out.txt 2>&1
echo EXIT:%ERRORLEVEL% >> compile64_out.txt
