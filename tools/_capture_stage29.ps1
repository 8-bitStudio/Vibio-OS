param(
    [int]$BootWait = 16,
    [int]$CmdWait = 3
)

# Stage 29 capture helper: boots the dev VM headless, drives the terminal with
# injected scancodes, and saves the required screenshots. VM-only; never touches
# host disks. Powers the VM off at the end.

$ErrorActionPreference = "Stop"
$VmName = "Vibio OS Dev"
$VBoxManage = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
$ShotDir = Join-Path (Split-Path -Parent $PSScriptRoot) "docs\screenshots"
New-Item -ItemType Directory -Force -Path $ShotDir | Out-Null

$codes = @{
    'a'=0x1E;'b'=0x30;'c'=0x2E;'d'=0x20;'e'=0x12;'f'=0x21;'g'=0x22;'h'=0x23;
    'i'=0x17;'j'=0x24;'k'=0x25;'l'=0x26;'m'=0x32;'n'=0x31;'o'=0x18;'p'=0x19;
    'q'=0x10;'r'=0x13;'s'=0x1F;'t'=0x14;'u'=0x16;'v'=0x2F;'w'=0x11;'x'=0x2D;
    'y'=0x15;'z'=0x2C;'1'=0x02;'2'=0x03;'3'=0x04;'4'=0x05;'5'=0x06;'6'=0x07;
    '7'=0x08;'8'=0x09;'9'=0x0A;'0'=0x0B;' '=0x39
}

function Get-VMState() {
    $info = & $VBoxManage showvminfo $VmName --machinereadable
    foreach ($line in $info) { if ($line -match '^VMState="([^"]+)"') { return $Matches[1] } }
    return ""
}

function Send-Codes([int[]]$bytes) {
    $hex = $bytes | ForEach-Object { '{0:x2}' -f $_ }
    & $VBoxManage controlvm $VmName keyboardputscancode @hex | Out-Null
}
function Send-Make([int]$make) { Send-Codes @($make, ($make -bor 0x80)) }
function Send-Char([string]$ch) {
    if (-not $codes.ContainsKey($ch)) { return }
    Send-Make $codes[$ch]
}
function Type-Command([string]$text) {
    foreach ($ch in $text.ToCharArray()) { Send-Char ([string]$ch); Start-Sleep -Milliseconds 55 }
    Send-Make 0x1C  # Enter
    Start-Sleep -Seconds $CmdWait
}
function Shot([string]$name) {
    $p = Join-Path $ShotDir $name
    & $VBoxManage controlvm $VmName screenshotpng $p | Out-Null
    if ((Test-Path $p) -and ((Get-Item $p).Length -gt 0)) { Write-Host "Saved $p" }
    else { throw "Screenshot failed: $p" }
}
function Boot-Headless() {
    if ((Get-VMState) -ne "running") {
        & $VBoxManage startvm $VmName --type headless | Out-Null
    }
    Start-Sleep -Seconds $BootWait
    if ((Get-VMState) -ne "running") { throw "VM not running after boot wait" }
}
function Power-Off() {
    if ((Get-VMState) -eq "running") {
        & $VBoxManage controlvm $VmName poweroff | Out-Null
        Start-Sleep -Seconds 3
    }
}

# --- 1. First boot: VERSION, SELFTEST, RWSTATUS, RWTEST ----------------------
Power-Off
Boot-Headless

Type-Command "version"
Shot "2026-06-25-stage29-version.png"

Type-Command "clear"
Type-Command "selftest"
Shot "2026-06-25-stage29-selftest.png"

Type-Command "clear"
Type-Command "rwstatus"
Shot "2026-06-25-stage29-rwstatus.png"

Type-Command "clear"
Type-Command "rwtest"
Shot "2026-06-25-stage29-rwtest-pass.png"

Power-Off

# --- 2. Full power cycle: prove persistence with RWREAD ----------------------
Boot-Headless
Type-Command "rwread"
Shot "2026-06-25-stage29-rw-persistence.png"
Power-Off

Write-Host "Stage 29 capture complete."
