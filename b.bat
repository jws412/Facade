@echo off
(gcc -O1 -funroll-loops -finline-functions -fdelete-null-pointer-checks -fcaller-saves -fdevirtualize -fgcse-after-reload -fipa-cp-clone -floop-interchange -floop-unroll-and-jam -fpeel-loops -fpredictive-commoning -fsplit-loops -fsplit-paths -ftree-loop-distribution -ftree-partial-pre -funswitch-loops -fvect-cost-model=dynamic -fversion-loops-for-strides main.c -o a.exe -luser32 -lgdi32 -lwinmm -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option -Werror=vla -std=c11 -Wunused -Wunused-macros || GOTO FAIL)
(mt.exe -manifest main.manifest -outputresource:a.exe || GOTO FAIL)
echo Build is successful.
EXIT /B

:FAIL
echo Build failed.
EXIT /B