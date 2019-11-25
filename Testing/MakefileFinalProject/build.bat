@echo off

set COM=COM5
TASKLIST /FI "WINDOWTITLE EQ %COM% - PUTTY" | FIND /I "putty.exe"
if %errorlevel% == 1 (start putty -serial %COM% -sercfg 115200)

ubuntu run make
STM32_Programmer_CLI -c port=SWD freq=4000 -e all -w build\MakefileFinalProject.elf -s
