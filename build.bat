@echo off
setlocal

set "ARMCC_BIN=E:\Keil_v5\ARM\ARMCC\bin"
set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=%PROJECT_DIR%build"

if not exist "%ARMCC_BIN%\armcc.exe" (
  echo [ERROR] Cannot find ARMCC at %ARMCC_BIN%
  exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

"%ARMCC_BIN%\armasm.exe" --cpu Cortex-M3 -g "%PROJECT_DIR%startup\startup_stm32f10x_hd.s" -o "%BUILD_DIR%\startup_stm32f10x_hd.o"
if errorlevel 1 exit /b 1

"%ARMCC_BIN%\armcc.exe" --cpu Cortex-M3 --c99 -c -g -O0 -DSTM32F10X_HD -I"%PROJECT_DIR%include" -I"%PROJECT_DIR%cmsis" "%PROJECT_DIR%src\system_stm32f10x.c" -o "%BUILD_DIR%\system_stm32f10x.o"
if errorlevel 1 exit /b 1

"%ARMCC_BIN%\armcc.exe" --cpu Cortex-M3 --c99 -c -g -O0 -DSTM32F10X_HD -I"%PROJECT_DIR%include" -I"%PROJECT_DIR%cmsis" "%PROJECT_DIR%src\main.c" -o "%BUILD_DIR%\main.o"
if errorlevel 1 exit /b 1

"%ARMCC_BIN%\armlink.exe" --cpu Cortex-M3 --scatter "%PROJECT_DIR%camera_hard_trigger.sct" --map --list "%BUILD_DIR%\camera_hard_trigger.map" "%BUILD_DIR%\startup_stm32f10x_hd.o" "%BUILD_DIR%\system_stm32f10x.o" "%BUILD_DIR%\main.o" -o "%BUILD_DIR%\camera_hard_trigger.axf"
if errorlevel 1 exit /b 1

"%ARMCC_BIN%\fromelf.exe" --i32combined --output "%BUILD_DIR%\camera_hard_trigger.hex" "%BUILD_DIR%\camera_hard_trigger.axf"
if errorlevel 1 exit /b 1

"%ARMCC_BIN%\fromelf.exe" --bin --output "%BUILD_DIR%\camera_hard_trigger.bin" "%BUILD_DIR%\camera_hard_trigger.axf"
if errorlevel 1 exit /b 1

echo [OK] Build completed.
echo HEX: %BUILD_DIR%\camera_hard_trigger.hex
echo BIN: %BUILD_DIR%\camera_hard_trigger.bin
endlocal
