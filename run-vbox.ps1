param(
    [switch]$Headless
)

$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$VdiImage = Join-Path $BuildDir "vibio.vdi"
$VmBase = Join-Path $BuildDir "vbox"
$VmName = "Vibio OS Dev"
$VBoxManage = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"

if (-not (Test-Path $VBoxManage)) {
    throw "VirtualBox was not found at $VBoxManage"
}

function Invoke-VBoxManage {
    & $VBoxManage @args
    if ($LASTEXITCODE -ne 0) {
        throw "VBoxManage failed: $($args -join ' ')"
    }
}

try {
    & $VBoxManage showvminfo $VmName *> $null
    $vmExists = ($LASTEXITCODE -eq 0)
} catch {
    $vmExists = $false
}

if (-not $vmExists) {
    New-Item -ItemType Directory -Force -Path $VmBase | Out-Null
    Invoke-VBoxManage createvm --name $VmName --ostype Other_64 --basefolder $VmBase --register
    Invoke-VBoxManage storagectl $VmName --name "SATA" --add sata --controller IntelAhci
}

try {
    & $VBoxManage storageattach $VmName --storagectl "SATA" --port 0 --device 0 --medium none *> $null
} catch {
}

& (Join-Path $Root "build.ps1")
if ($LASTEXITCODE -ne 0) {
    throw "build.ps1 failed"
}

Invoke-VBoxManage modifyvm $VmName `
    --firmware efi `
    --memory 128 `
    --vram 16 `
    --cpus 1 `
    --boot1 disk `
    --boot2 none `
    --boot3 none `
    --boot4 none `
    --nic1 none `
    --audio-enabled on `
    --audio-driver default `
    --audio-controller hda `
    --audio-out on `
    --audio-in off `
    --clipboard disabled `
    --drag-and-drop disabled `
    --usb off

Invoke-VBoxManage storageattach $VmName `
    --storagectl "SATA" `
    --port 0 `
    --device 0 `
    --type hdd `
    --medium $VdiImage

$startType = if ($Headless) { "headless" } else { "gui" }
Invoke-VBoxManage startvm $VmName --type $startType
