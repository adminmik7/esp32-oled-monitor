@echo off
chcp 65001 >nul
title ESP32 OLED Monitor
echo ==========================================
echo   ESP32 OLED PC Monitor (Windows Edition)
echo ==========================================
echo.

where py >nul 2>&1
if %errorlevel% equ 0 (set PYTHON_CMD=py) else (
    where python >nul 2>&1
    if %errorlevel% equ 0 (set PYTHON_CMD=python) else (
        echo [!] Python not found. Please install Python.
        pause & exit /b 1
    )
)

echo [*] Installing dependencies...
%PYTHON_CMD% -m pip install pyserial psutil wmi --quiet

echo.
echo [*] Starting monitor...
%PYTHON_CMD% sender.py
pause
