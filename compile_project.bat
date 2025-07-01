@echo off
echo ================================
echo STM32 ????????????
echo ================================
echo.

REM ??Keil????
if exist "C:\Keil_v5\UV4\UV4.exe" (
    set KEIL_PATH="C:\Keil_v5\UV4\UV4.exe"
    echo ?? Keil ????: C:\Keil_v5\UV4\UV4.exe
) else if exist "D:\Keil_v5\UV4\UV4.exe" (
    set KEIL_PATH="D:\Keil_v5\UV4\UV4.exe" 
    echo ?? Keil ????: D:\Keil_v5\UV4\UV4.exe
) else (
    echo ??: ??? Keil ????
    echo ????? Keil IDE ????
    pause
    exit /b 1
)

echo.
echo ??????...
echo.

REM ???????
cd /d "MDK-ARM"

REM ????
%KEIL_PATH% -b "Three-channel controller_v3.0.uvprojx" -j0

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================
    echo ?????
    echo ================================
) else (
    echo.
    echo ================================  
    echo ?????
    echo ================================
)

echo.
pause
