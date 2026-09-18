@echo off
chcp 65001 >nul
setlocal
pushd "%~dp0"

rem Visual Studio 开发环境入口；当前验证程序使用 x64 MSVC 编译。
set "VSDEVCMD=D:\CodeSoftware\vs2022\Common7\Tools\VsDevCmd.bat"
rem 仓库内 MVS SDK 根目录；构建不再依赖 D:\Program Files\MVS\Development。
for %%I in ("%~dp0..\..\..\third_party\mvs") do set "MVS_SDK=%%~fI"

if not exist "%VSDEVCMD%" (
    echo [ERROR] Visual Studio developer command file not found:
    echo %VSDEVCMD%
    popd
    exit /b 1
)

if not exist "%MVS_SDK%\include\MvCameraControl.h" (
    echo [ERROR] MVS SDK header not found:
    echo %MVS_SDK%\include\MvCameraControl.h
    echo Run sync_mvs_sdk.bat first.
    popd
    exit /b 1
)

if not exist "%MVS_SDK%\lib\msvc\x64\MvCameraControl.lib" (
    echo [ERROR] MVS SDK import library not found:
    echo %MVS_SDK%\lib\msvc\x64\MvCameraControl.lib
    echo Run sync_mvs_sdk.bat first.
    popd
    exit /b 1
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio build environment.
    popd
    exit /b 1
)

if not exist build mkdir build

cl /nologo /std:c++17 /EHsc /utf-8 /W4 /Fo"build\\" /I"%MVS_SDK%\include" src\main.cpp /link /LIBPATH:"%MVS_SDK%\lib\msvc\x64" MvCameraControl.lib /OUT:build\hikrobot_trigger_verify.exe
if errorlevel 1 (
    echo [ERROR] PC trigger verifier build failed.
    popd
    exit /b 1
)

echo [OK] PC trigger verifier build completed.
echo MVS SDK: %MVS_SDK%
echo EXE: %CD%\build\hikrobot_trigger_verify.exe
popd
exit /b 0
