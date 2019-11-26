@echo off

for /f "delims=" %%I in ('wmic path Win32_SerialPort get DeviceID /format:list ^| find "COM"') do (
    set "%%I"
)
set COM=%DeviceID%
TASKLIST /FI "WINDOWTITLE EQ %COM% - PUTTY" | FIND /I "putty.exe"
if %errorlevel% == 1 (start putty -serial %COM% -sercfg 115200)

ubuntu run make
STM32_Programmer_CLI -c port=SWD freq=4000 -e all -w build\MakefileFinalProject.elf -s
