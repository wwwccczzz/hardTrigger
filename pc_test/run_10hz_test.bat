@echo off
chcp 65001 >nul
setlocal
pushd "%~dp0"
rem 优先使用仓库内的 MVS x64 Runtime；本目录下不存在时回退到系统安装的 MVS 运行时。
set "MVS_RUNTIME="
for %%I in ("%~dp0..\..\..\third_party\mvs\runtime\win64") do if exist "%%~fI\MvCameraControl.dll" set "MVS_RUNTIME=%%~fI"
for %%I in ("C:\Program Files (x86)\Common Files\MVS\Runtime\Win64_x64") do if not defined MVS_RUNTIME if exist "%%~fI\MvCameraControl.dll" set "MVS_RUNTIME=%%~fI"
if defined MVS_RUNTIME set "PATH=%MVS_RUNTIME%;%PATH%"
if not defined MVS_RUNTIME echo [WARN] 未找到 MvCameraControl.dll，将依赖系统 PATH；请确认已安装MVS客户端。
if not exist "build\hikrobot_trigger_verify.exe" call build_pc_test.bat
if errorlevel 1 (
    popd
    exit /b 1
)
build\hikrobot_trigger_verify.exe 10 10
rem 保存验证程序退出码，供命令行和上层脚本判断本次测试是否通过。
set "TEST_EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %TEST_EXIT_CODE%
