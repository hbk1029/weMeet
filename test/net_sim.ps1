# WeMeet network simulation - PowerShell
param($ClumsyPath)

$filter = 'tcp.DstPort == 8080 or tcp.SrcPort == 8080'

$scenarios = @{
    '1' = @{ Name='Light Loss 5%';           Args="--filter `"$filter`" --drop on --drop-chance 5.0" }
    '2' = @{ Name='Moderate Delay 200ms';    Args="--filter `"$filter`" --lag on --lag-time 200" }
    '3' = @{ Name='High Jitter 0-300ms';     Args="--filter `"$filter`" --lag on --lag-time 150 --o-o-o on --o-o-o-chance 20.0" }
    '4' = @{ Name='Combined (5%+200ms)';     Args="--filter `"$filter`" --drop on --drop-chance 5.0 --lag on --lag-time 200" }
    '5' = @{ Name='Severe (10%+400ms)';      Args="--filter `"$filter`" --drop on --drop-chance 10.0 --lag on --lag-time 400" }
}

function Show-Menu {
    Write-Host "`n=== WeMeet Network Simulation ===" -ForegroundColor Cyan
    foreach ($k in $scenarios.Keys | Sort-Object) {
        Write-Host "  $k. $($scenarios[$k].Name)"
    }
    Write-Host "  6. Custom (enter clumsy args)"
    Write-Host "  7. Stop clumsy"
    Write-Host "  0. Exit"
}

function Stop-Clumsy {
    $p = Get-Process clumsy -ErrorAction SilentlyContinue
    if ($p) {
        $p | Stop-Process -Force
        Write-Host "clumsy stopped" -ForegroundColor Green
    } else {
        Write-Host "clumsy not running"
    }
}

function Start-Clumsy($argsStr) {
    Stop-Clumsy
    Start-Sleep -Seconds 1
    Write-Host "Launching: clumsy $argsStr" -ForegroundColor Yellow
    Start-Process -FilePath $ClumsyPath -ArgumentList $argsStr -WindowStyle Normal
    Write-Host "Running (close clumsy window to stop)" -ForegroundColor Green
}

if (!(Test-Path $ClumsyPath)) {
    Write-Host "[ERROR] clumsy.exe not found: $ClumsyPath" -ForegroundColor Red
    Write-Host "Download: https://github.com/jagt/clumsy/releases/download/0.2/clumsy-0.2-win64.zip"
    Write-Host "Extract clumsy.exe to: $(Split-Path $ClumsyPath)"
    exit 1
}

while ($true) {
    Show-Menu
    $ch = Read-Host "Choice"
    switch ($ch) {
        { 1..5 -contains $_ } {
            Start-Clumsy $scenarios[$ch].Args
        }
        '6' {
            $custom = Read-Host "clumsy args (e.g. --lag on --lag-time 100)"
            if ($custom) { Start-Clumsy $custom }
        }
        '7' { Stop-Clumsy }
        '0' { exit 0 }
        default { Write-Host "Invalid choice" -ForegroundColor Red }
    }
}
