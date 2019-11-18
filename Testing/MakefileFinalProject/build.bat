@echo off

TASKLIST /FI "WINDOWTITLE EQ COM6 - PUTTY" | FIND /I "putty.exe"
if %errorlevel% == 1 (start putty -serial COM6 -sercfg 115200)

ubuntu run make
STM32_Programmer_CLI -c port=SWD freq=4000 -e all -w build\MakefileFinalProject.elf -s
