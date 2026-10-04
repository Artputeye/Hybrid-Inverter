@echo off
setlocal

cd /d "D:\@Project\Hybride Inverter"

powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\run-serena-tunnel.ps1"

if errorlevel 1 (
    echo.
    echo Serena Tunnel stopped with an error.
    pause
)

endlocal
