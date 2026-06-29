param(
    [string]$Shot = "",
    [string]$Type = "",
    [string]$Keys = "",
    [int]$BootWait = 16,
    [int]$AfterTypeWait = 4,
    [switch]$NoUsb,
    [switch]$KeepRunning
)

# QEMU test harness for Vibio OS. Boots the generated UEFI image under OVMF with
# an AHCI boot disk and (by default) an emulated xHCI controller carrying a USB
# mass-storage stick - so the post-boot USB driver can be developed and tested
# here WITHOUT the VirtualBox Extension Pack. VM-only; touches only build\ files.
#
#   .\tools\run-qemu.ps1 -Shot build\qemu-shot.png            (boot + screenshot)
#   .\tools\run-qemu.ps1 -NoUsb                               (no USB device)
#
# Screenshots are captured over QMP (screendump -> PPM) and converted to PNG.

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$Qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe"
$FwCode = "C:\Program Files\qemu\share\edk2-x86_64-code.fd"
$FwVarsTemplate = "C:\Program Files\qemu\share\edk2-i386-vars.fd"

if (-not (Test-Path $Qemu)) { throw "QEMU not found at $Qemu" }
if (-not (Test-Path $FwCode)) { throw "OVMF firmware not found at $FwCode" }

$img = Join-Path $BuildDir "vibio.img"
if (-not (Test-Path $img)) { throw "Missing $img (run .\build.ps1 first)" }

# Writable per-run copy of the UEFI variable store.
$varsCopy = Join-Path $BuildDir "qemu-ovmf-vars.fd"
Copy-Item -LiteralPath $FwVarsTemplate -Destination $varsCopy -Force

# A small FAT image to expose as the USB mass-storage device for driver testing.
$usbImg = Join-Path $BuildDir "qemu-usb.img"
if (-not $NoUsb -and -not (Test-Path $usbImg)) {
    # 64 MiB raw image; the USB driver work will format/inspect it. For now it is
    # just a block device behind xHCI so enumeration can be exercised.
    $fs = [System.IO.File]::Create($usbImg)
    $fs.SetLength(64MB)
    $fs.Close()
}

$qmpPort = 55555
$qmpArg = "tcp:127.0.0.1:$qmpPort,server,nowait"

$qemuArgs = @(
    "-machine", "q35",
    "-m", "512",
    "-rtc", "base=localtime",
    "-drive", "if=pflash,format=raw,readonly=on,file=$FwCode",
    "-drive", "if=pflash,format=raw,file=$varsCopy",
    "-drive", "id=bootdisk,if=none,format=raw,file=$img",
    "-device", "ich9-ahci,id=ahci0",
    "-device", "ide-hd,drive=bootdisk,bus=ahci0.0",
    "-display", "none",
    "-qmp", $qmpArg
)
if (-not $NoUsb) {
    $qemuArgs += @(
        "-device", "qemu-xhci,id=xhci",
        "-drive", "id=usbstick,if=none,format=raw,file=$usbImg",
        "-device", "usb-storage,bus=xhci.0,drive=usbstick"
    )
}

Write-Host "Launching QEMU (q35 + OVMF + AHCI$(if(-not $NoUsb){' + xHCI usb-storage'}))..."
# Build a single quoted command line: Start-Process -ArgumentList with an array
# does NOT quote tokens that contain spaces (firmware path has 'Program Files',
# the build path has 'vibio os'), which would split a -drive value across argv.
$cmdline = ($qemuArgs | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }) -join ' '
$proc = Start-Process -FilePath $Qemu -ArgumentList $cmdline -PassThru
Start-Sleep -Seconds $BootWait

function Send-Qmp([string]$json, [System.IO.StreamWriter]$w, [System.IO.StreamReader]$r) {
    $w.WriteLine($json); $w.Flush(); Start-Sleep -Milliseconds 250
    while ($r.Peek() -ge 0) { $null = $r.ReadLine() }
}

# Map a character to a QEMU qcode for send-key. Letters/digits map to themselves;
# a few specials are named. Returns $null for unsupported characters.
function Get-Qcode([char]$ch) {
    if ($ch -cmatch '[a-z]') { return [string]$ch }
    if ($ch -cmatch '[A-Z]') { return [string]([char]::ToLower($ch)) }
    if ($ch -cmatch '[0-9]') { return @('0','1','2','3','4','5','6','7','8','9')[[int]([string]$ch)] }
    switch ($ch) {
        ' ' { return 'spc' }
        '-' { return 'minus' }
        '.' { return 'dot' }
        default { return $null }
    }
}

# Type a command string then Enter over QMP send-key (commands are processed by
# the console even though the Terminal window is hidden, like the VBox harness).
function Send-Type([string]$text, [System.IO.StreamWriter]$w, [System.IO.StreamReader]$r) {
    foreach ($ch in $text.ToCharArray()) {
        $q = Get-Qcode $ch
        if ($null -eq $q) { continue }
        Send-Qmp ('{"execute":"send-key","arguments":{"keys":[{"type":"qcode","data":"' + $q + '"}]}}') $w $r
        Start-Sleep -Milliseconds 60
    }
    Send-Qmp '{"execute":"send-key","arguments":{"keys":[{"type":"qcode","data":"ret"}]}}' $w $r
}

if ($Type -ne "") {
    try {
        $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $qmpPort)
        $stream = $client.GetStream()
        $w = New-Object System.IO.StreamWriter($stream); $w.AutoFlush = $true
        $r = New-Object System.IO.StreamReader($stream)
        Start-Sleep -Milliseconds 300
        while ($r.Peek() -ge 0) { $null = $r.ReadLine() }
        Send-Qmp '{"execute":"qmp_capabilities"}' $w $r
        Send-Type $Type $w $r
        $client.Close()
    } catch {
        Write-Warning "QMP send-key failed: $($_.Exception.Message)"
    }
    Start-Sleep -Seconds $AfterTypeWait
}

# Send raw qcode key names (space-separated, e.g. "down down down ret") after the
# command, to drive the focused window (e.g. navigate the Files list).
if ($Keys -ne "") {
    try {
        $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $qmpPort)
        $stream = $client.GetStream()
        $w = New-Object System.IO.StreamWriter($stream); $w.AutoFlush = $true
        $r = New-Object System.IO.StreamReader($stream)
        Start-Sleep -Milliseconds 300
        while ($r.Peek() -ge 0) { $null = $r.ReadLine() }
        Send-Qmp '{"execute":"qmp_capabilities"}' $w $r
        foreach ($k in ($Keys -split '\s+')) {
            if ($k -eq "") { continue }
            Send-Qmp ('{"execute":"send-key","arguments":{"keys":[{"type":"qcode","data":"' + $k + '"}]}}') $w $r
            Start-Sleep -Milliseconds 250
        }
        $client.Close()
    } catch {
        Write-Warning "QMP raw send-key failed: $($_.Exception.Message)"
    }
    Start-Sleep -Seconds $AfterTypeWait
}

if ($Shot -ne "") {
    if (-not [System.IO.Path]::IsPathRooted($Shot)) { $Shot = Join-Path (Get-Location) $Shot }
    $shotDir = Split-Path -Parent $Shot
    if ($shotDir -ne "" -and -not (Test-Path $shotDir)) { New-Item -ItemType Directory -Force -Path $shotDir | Out-Null }
    $ppm = [System.IO.Path]::ChangeExtension($Shot, ".ppm")
    try {
        $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $qmpPort)
        $stream = $client.GetStream()
        $w = New-Object System.IO.StreamWriter($stream); $w.AutoFlush = $true
        $r = New-Object System.IO.StreamReader($stream)
        Start-Sleep -Milliseconds 300
        while ($r.Peek() -ge 0) { $null = $r.ReadLine() }   # greeting
        Send-Qmp '{"execute":"qmp_capabilities"}' $w $r
        Send-Qmp ('{"execute":"screendump","arguments":{"filename":"' + ($ppm -replace '\\','/') + '"}}') $w $r
        Start-Sleep -Milliseconds 500
        $client.Close()
    } catch {
        Write-Warning "QMP screenshot failed: $($_.Exception.Message)"
    }
    if (Test-Path $ppm) {
        & (Get-Command py).Source -3 -c "from PIL import Image; Image.open(r'$ppm').save(r'$Shot')"
        Write-Host "Saved screenshot: $Shot"
    } else {
        Write-Warning "No PPM produced at $ppm"
    }
}

if (-not $KeepRunning) {
    try {
        $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $qmpPort)
        $stream = $client.GetStream()
        $w = New-Object System.IO.StreamWriter($stream); $w.AutoFlush = $true
        $r = New-Object System.IO.StreamReader($stream)
        Start-Sleep -Milliseconds 200
        $w.WriteLine('{"execute":"qmp_capabilities"}'); Start-Sleep -Milliseconds 150
        $w.WriteLine('{"execute":"quit"}'); Start-Sleep -Milliseconds 200
        $client.Close()
    } catch { }
    Start-Sleep -Seconds 1
    if (-not $proc.HasExited) { $proc.Kill() }
    Write-Host "QEMU stopped."
}
