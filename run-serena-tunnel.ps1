# Run OpenAI Tunnel + Serena MCP
# Project: D:\@Project\Hybride Inverter

$ErrorActionPreference = "Stop"

$TunnelDir = "C:\OpenAI-Tunnel"
$TunnelClient = Join-Path $TunnelDir "tunnel-client.exe"
$Profile = "serena"

if (-not (Test-Path $TunnelClient)) {
    Write-Host "ERROR: tunnel-client.exe not found: $TunnelClient" -ForegroundColor Red
    exit 1
}

Set-Location $TunnelDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " OpenAI Tunnel + Serena MCP" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Profile : $Profile"
Write-Host "Project : D:\@Project\Hybride Inverter"
Write-Host ""

Write-Host "[1/2] Checking tunnel profile..." -ForegroundColor Yellow
& $TunnelClient doctor --profile $Profile --explain
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Tunnel profile check failed." -ForegroundColor Red
    Write-Host "Run setup/init for profile '$Profile' first." -ForegroundColor Yellow
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "[2/2] Starting Tunnel + Serena MCP..." -ForegroundColor Green
Write-Host "Keep this window open while using Serena." -ForegroundColor Yellow
Write-Host ""

& $TunnelClient run --profile $Profile

Write-Host ""
Write-Host "Tunnel stopped." -ForegroundColor Yellow
Pause
