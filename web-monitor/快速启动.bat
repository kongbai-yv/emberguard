@echo off
chcp 65001 >nul
title 工业物联网监控大屏 - 快速启动

echo ========================================
echo   工业物联网监控大屏
echo   IoT Monitoring Dashboard
echo ========================================
echo.

REM 切换到脚本所在目录
cd /d "%~dp0"

REM [1/4] 检查 Node.js 是否安装
echo [1/4] 检查 Node.js 环境...
where node >nul 2>&1
if errorlevel 1 (
    echo.
    echo ❌ 错误: 未检测到 Node.js
    echo    请先安装 Node.js: https://nodejs.org/
    echo.
    pause
    exit /b 1
)
echo    ✅ Node.js 已安装: 
node -v
echo.

REM [2/4] 检查并安装依赖
echo [2/4] 检查项目依赖...
if not exist "node_modules" (
    echo    📦 首次运行，正在安装依赖...
    echo.
    call npm install
    if errorlevel 1 (
        echo.
        echo ❌ 依赖安装失败！
        echo    请检查网络连接后重试
        pause
        exit /b 1
    )
    echo    ✅ 依赖安装完成
) else (
    echo    ✅ 依赖已就绪
)
echo.

REM [3/4] 清理旧进程和端口
echo [3/4] 清理旧进程...
for /f "tokens=5" %%a in ('netstat -ano ^| findstr :5173 2^>nul') do (
    echo    关闭占用端口 5173 的进程 (PID: %%a)
    taskkill /F /PID %%a >nul 2>&1
)
echo    ✅ 端口清理完成
echo.

REM [4/4] 启动开发服务器
echo [4/4] 启动开发服务器...
echo.
echo ========================================
echo   服务器启动中...
echo   地址: http://localhost:5173
echo ========================================
echo.
echo 💡 提示:
echo    - 浏览器将在 5 秒后自动打开
echo    - 按 Ctrl+C 可停止服务器
echo    - 保持此窗口打开以运行服务器
echo.

REM 在后台启动 Vite 服务器
start "Vite Server" cmd /c "npm run dev"

REM 等待服务器启动
echo ⏳ 等待服务器就绪...
for /l %%i in (1,1,10) do (
    timeout /t 1 /nobreak >nul
    netstat -ano | findstr :5173 >nul 2>&1
    if not errorlevel 1 (
        echo    ✅ 服务器已就绪！
        goto :open_browser
    )
    echo    ⏳ 等待中... (%%i/10)
)

:open_browser
echo.
echo 🌐 正在打开浏览器...
timeout /t 2 /nobreak >nul
start http://localhost:5173

echo.
echo ========================================
echo   ✅ 启动完成！
echo   📊 监控大屏已打开
echo   🔧 服务器在后台运行
echo ========================================
echo.
echo 按任意键关闭此窗口 (服务器继续运行)...
pause >nul
