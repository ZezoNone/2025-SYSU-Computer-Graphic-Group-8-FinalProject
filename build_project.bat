chcp 65001 >nul
@echo off
setlocal

:: -------------------------
:: 1. 配置 VS 版本与构建目录
:: -------------------------
echo 检测 Visual Studio...
:: 尝试使用 vswhere 工具查找最新 VS 版本
for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do set VS_PATH=%%i

if not defined VS_PATH (
    echo 未检测到 Visual Studio，请确保已安装 VS 并包含 MSBuild
    pause
    exit /b 1
)

echo 找到 Visual Studio 安装目录: %VS_PATH%

set BUILD_DIR=%CD%\build
if not exist "%BUILD_DIR%" (
    echo 创建 build 文件夹...
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"

:: -------------------------
:: 2. 调用 CMake 生成 Release 配置
:: -------------------------
echo 调用 CMake 生成项目 (Release 模式)...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..

if %errorlevel% neq 0 (
    echo CMake 配置失败，请检查错误信息
    pause
    exit /b 1
)

:: -------------------------
:: 3. 构建解决方案
:: -------------------------
echo 开始构建项目...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo 构建失败，请检查错误信息
    pause
    exit /b 1
)

:: -------------------------
:: 4. 打开生成的 .sln 文件
:: -------------------------
echo 构建完成，打开解决方案文件...
start "" "FinalProject.sln"

:: -------------------------
:: 5. 提示用户下一步操作
:: -------------------------
echo.
echo ===============================
echo 操作提示：
echo 1. 在打开的 Visual Studio 中，右键将 FinalProject 设置为启动项目
echo 2. 点击 "本地 Windows 调试器" 或按 F5 运行
echo ===============================
echo.

pause
endlocal
