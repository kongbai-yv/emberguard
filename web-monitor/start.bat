@echo off
chcp 65001 >nul
title MQTT Monitor Dashboard - Startup

echo ========================================
echo   MQTT Monitor Dashboard
echo   Starting Development Server...
echo ========================================
echo.

REM Check if node_modules exists
if not exist "node_modules" (
    echo [1/3] Installing dependencies...
    call npm install
    if errorlevel 1 (
        echo.
        echo ERROR: Failed to install dependencies!
        pause
        exit /b 1
    )
    echo.
) else (
    echo [1/3] Dependencies already installed
    echo.
)

REM Kill any existing Vite processes
taskkill /F /IM node.exe >nul 2>&1

echo [2/3] Starting Vite development server...
start "" cmd /k "npm run dev"

echo [3/3] Waiting for server to start...
timeout /t 3 /nobreak >nul

echo.
echo Opening browser...
start http://localhost:5173

echo.
echo ========================================
echo   Server started successfully!
echo   URL: http://localhost:5173
echo ========================================
echo.
echo Press Ctrl+C in the server window to stop
echo.
pause
