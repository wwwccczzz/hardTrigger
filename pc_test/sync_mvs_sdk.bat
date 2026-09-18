@echo off
chcp 65001 >nul
setlocal
pushd "%~dp0"

rem 调用仓库内统一的 MVS SDK 同步脚本，避免测试程序维护另一份复制逻辑。
for %%I in ("%~dp0..\..\..\cmake\sync_mvs_sdk.bat") do set "SYNC_SCRIPT=%%~fI"
call "%SYNC_SCRIPT%"
rem 保存统一同步脚本的退出码，确保失败时测试入口也返回失败。
set "SYNC_EXIT_CODE=%ERRORLEVEL%"

popd
exit /b %SYNC_EXIT_CODE%
