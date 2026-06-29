param(
    [switch]$Recreate,   # rebuild the scratch image + VDI even if it already exists
    [switch]$Detach,     # detach the scratch disk from the VM (for negative testing)
    [string]$VmName = "Vibio OS Dev",
    [int]$Port = 1,
    [int]$SizeMiB = 64
)

# Stage 29: manage the VM-only FAT32 read/write sandbox scratch disk.
#
# This script ONLY ever touches files under build\ and the development VM's SATA
# port 1. It never formats, resizes, or writes to the boot USB, host disks,
# internal disks, Windows, or Ubuntu. The scratch disk is disposable.

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$ScratchImg = Join-Path $BuildDir "vibio-rw-scratch.img"
$ScratchVdi = Join-Path $BuildDir "vibio-rw-scratch.vdi"
$VBoxManage = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$python = (Get-Command py -ErrorAction Stop).Source

function Test-UnderBuild([string]$path) {
    $full = [System.IO.Path]::GetFullPath($path)
    $buildFull = [System.IO.Path]::GetFullPath($BuildDir)
    if (-not $full.StartsWith($buildFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to touch a path outside the build directory: $full"
    }
}

function Detach-Scratch() {
    if (-not (Test-Path $VBoxManage)) { return }
    & $VBoxManage showvminfo $VmName *> $null
    if ($LASTEXITCODE -ne 0) { return }

    # Only detach when something is actually attached at this port; calling
    # storageattach --medium none on an empty slot is a (noisy) error.
    $info = & $VBoxManage showvminfo $VmName --machinereadable
    $attached = $false
    foreach ($line in $info) {
        if ($line -match ('^"SATA-{0}-0"="(.+)"$' -f $Port)) {
            if ($Matches[1] -ne 'none') { $attached = $true }
        }
    }
    if ($attached) {
        try { & $VBoxManage storageattach $VmName --storagectl "SATA" --port $Port --device 0 --medium none *> $null } catch {}
        Write-Host "Detached scratch disk from '$VmName' SATA port $Port."
    }
}

if ($Detach) {
    Detach-Scratch
    return
}

# 1. Build the raw image + VDI if missing (or when -Recreate). Writes persist
#    across plain kernel rebuilds because we only (re)create when absent.
$needBuild = $Recreate -or (-not (Test-Path $ScratchVdi))
if ($needBuild) {
    Test-UnderBuild $ScratchImg
    Test-UnderBuild $ScratchVdi

    & $python (Join-Path $Root "tools\make_rw_sandbox.py") `
        --out $ScratchImg --label VIBIORW --size-mib $SizeMiB
    if ($LASTEXITCODE -ne 0) { throw "make_rw_sandbox.py failed" }

    if (Test-Path $VBoxManage) {
        # Drop any previous registration of this scratch VDI, then convert fresh.
        Detach-Scratch
        try { & $VBoxManage closemedium disk $ScratchVdi --delete *> $null } catch {}
        if (Test-Path $ScratchVdi) { Remove-Item -LiteralPath $ScratchVdi -Force }

        & $VBoxManage convertfromraw --format VDI $ScratchImg $ScratchVdi
        if ($LASTEXITCODE -ne 0) { throw "VBoxManage convertfromraw (scratch) failed" }
        Write-Host "Built scratch VDI: $ScratchVdi"
    } else {
        Write-Host "Built scratch image: $ScratchImg (VirtualBox not found; VDI skipped)"
    }
}

# 2. Attach the scratch VDI at SATA port 1 of the dev VM (boot disk stays port 0).
if (Test-Path $VBoxManage) {
    & $VBoxManage showvminfo $VmName *> $null
    if ($LASTEXITCODE -eq 0) {
        # Make sure the SATA controller exposes enough ports, then (re)attach.
        & $VBoxManage storagectl $VmName --name "SATA" --portcount 2 *> $null
        Detach-Scratch
        & $VBoxManage storageattach $VmName `
            --storagectl "SATA" `
            --port $Port `
            --device 0 `
            --type hdd `
            --medium $ScratchVdi
        if ($LASTEXITCODE -ne 0) { throw "VBoxManage storageattach (scratch) failed" }
        Write-Host "Attached scratch VDI to '$VmName' SATA port $Port (boot disk remains port 0)."
    }
}
