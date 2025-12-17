@echo off
REM 脚本所在目录
set SCRIPT_DIR=%~dp0

REM exe 所在路径（code 文件夹）
set EXE_PATH=%SCRIPT_DIR%..\code\FinalProject.exe

REM 运行 exe
"%EXE_PATH%"

pause
