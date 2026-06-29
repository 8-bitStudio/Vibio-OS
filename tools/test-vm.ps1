param(
    [string]$Shot = "",
    [string]$Type = "",
    [int]$BootWait = 15,
    [int]$AfterTypeWait = 3,
    [switch]$KeepRunning
)

# Headless boot + optional keyboard injection + screenshot + power off helper
# for the Vibio OS Dev VM. VM-only; never touches host disks.

$ErrorActionPreference = "Stop"
$VmName = "Vibio OS Dev"
$VBoxManage = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"

if (-not (Test-Path -LiteralPath $VBoxManage)) {
    throw "VBoxManage.exe not found at $VBoxManage"
}

function Invoke-VBoxManage([string[]]$ArgsList, [switch]$AllowFailure) {
    & $VBoxManage @ArgsList
    $code = $LASTEXITCODE
    if (($code -ne 0) -and (-not $AllowFailure)) {
        throw "VBoxManage failed with exit code $code`: $($ArgsList -join ' ')"
    }
    return $code
}

function Get-VMState() {
    $info = & $VBoxManage showvminfo $VmName --machinereadable
    if ($LASTEXITCODE -ne 0) {
        throw "Could not read VM state for '$VmName'"
    }
    foreach ($line in $info) {
        if ($line -match '^VMState="([^"]+)"') {
            return $Matches[1]
        }
    }
    return ""
}

# Set 1 make-code map. Each entry is @(make, needsShift) so we can type both
# upper/lower case and shifted symbols for input testing.
$codes = @{
    'a'=@(0x1E,$false);'b'=@(0x30,$false);'c'=@(0x2E,$false);'d'=@(0x20,$false);
    'e'=@(0x12,$false);'f'=@(0x21,$false);'g'=@(0x22,$false);'h'=@(0x23,$false);
    'i'=@(0x17,$false);'j'=@(0x24,$false);'k'=@(0x25,$false);'l'=@(0x26,$false);
    'm'=@(0x32,$false);'n'=@(0x31,$false);'o'=@(0x18,$false);'p'=@(0x19,$false);
    'q'=@(0x10,$false);'r'=@(0x13,$false);'s'=@(0x1F,$false);'t'=@(0x14,$false);
    'u'=@(0x16,$false);'v'=@(0x2F,$false);'w'=@(0x11,$false);'x'=@(0x2D,$false);
    'y'=@(0x15,$false);'z'=@(0x2C,$false);
    '1'=@(0x02,$false);'2'=@(0x03,$false);'3'=@(0x04,$false);'4'=@(0x05,$false);
    '5'=@(0x06,$false);'6'=@(0x07,$false);'7'=@(0x08,$false);'8'=@(0x09,$false);
    '9'=@(0x0A,$false);'0'=@(0x0B,$false);' '=@(0x39,$false);
    '-'=@(0x0C,$false);'='=@(0x0D,$false);'['=@(0x1A,$false);']'=@(0x1B,$false);
    ';'=@(0x27,$false);"'"=@(0x28,$false);'`'=@(0x29,$false);'\'=@(0x2B,$false);
    ','=@(0x33,$false);'.'=@(0x34,$false);'/'=@(0x35,$false);
    '!'=@(0x02,$true);'@'=@(0x03,$true);'#'=@(0x04,$true);'$'=@(0x05,$true);
    '%'=@(0x06,$true);'^'=@(0x07,$true);'&'=@(0x08,$true);'*'=@(0x09,$true);
    '('=@(0x0A,$true);')'=@(0x0B,$true);'_'=@(0x0C,$true);'+'=@(0x0D,$true);
    '{'=@(0x1A,$true);'}'=@(0x1B,$true);':'=@(0x27,$true);'"'=@(0x28,$true);
    '~'=@(0x29,$true);'|'=@(0x2B,$true);'<'=@(0x33,$true);'>'=@(0x34,$true);
    '?'=@(0x35,$true)
}

$SHIFT = 0x2A

# Send a list of integer scancodes as one atomic keyboardputscancode call so
# modifier+key sequences cannot race across separate VBoxManage invocations.
function Send-Codes([int[]]$bytes) {
    $hex = $bytes | ForEach-Object { '{0:x2}' -f $_ }
    $cmd = @("controlvm", $VmName, "keyboardputscancode") + $hex
    Invoke-VBoxManage $cmd | Out-Null
}

function Send-Make([int]$make) {
    Send-Codes @($make, ($make -bor 0x80))
}

function Send-Char($ch) {
    $isUpper = ($ch -cmatch '[A-Z]')
    $key = if ($isUpper) { [string]$ch.ToString().ToLower() } else { [string]$ch }
    if (-not $codes.ContainsKey($key)) { return }
    $entry = $codes[$key]
    $make = $entry[0]
    $needShift = $entry[1] -or $isUpper
    if ($needShift) {
        Send-Codes @($SHIFT, $make, ($make -bor 0x80), ($SHIFT -bor 0x80))
    } else {
        Send-Codes @($make, ($make -bor 0x80))
    }
}

$state = Get-VMState
if ($state -eq "running") {
    Write-Host "$VmName is already running"
} else {
    Invoke-VBoxManage @("startvm", $VmName, "--type", "headless") | Out-Null
}
Start-Sleep -Seconds $BootWait

$state = Get-VMState
if ($state -ne "running") {
    throw "$VmName is not running after boot wait; current state is '$state'"
}

if ($Type -ne "") {
    foreach ($ch in $Type.ToCharArray()) {
        Send-Char $ch
        Start-Sleep -Milliseconds 60
    }
    # Enter
    Send-Make 0x1C
    Start-Sleep -Seconds $AfterTypeWait
}

if ($Shot -ne "") {
    $shotPath = $Shot
    if (-not [System.IO.Path]::IsPathRooted($shotPath)) {
        $shotPath = Join-Path (Get-Location) $shotPath
    }
    $shotDir = Split-Path -Parent $shotPath
    if ($shotDir -ne "") {
        New-Item -ItemType Directory -Path $shotDir -Force | Out-Null
    }

    Invoke-VBoxManage @("controlvm", $VmName, "screenshotpng", $shotPath) | Out-Null
    if (-not (Test-Path -LiteralPath $shotPath)) {
        throw "VBoxManage reported screenshot success, but no file was written: $shotPath"
    }
    if ((Get-Item -LiteralPath $shotPath).Length -le 0) {
        throw "Screenshot file is empty: $shotPath"
    }
    Write-Host "Saved screenshot: $shotPath"
}

if (-not $KeepRunning) {
    if ((Get-VMState) -eq "running") {
        Invoke-VBoxManage @("controlvm", $VmName, "poweroff") | Out-Null
        Write-Host "Powered off $VmName"
    }
}
