$ErrorActionPreference = "Stop"

$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$BootObj = Join-Path $BuildDir "boot_main.obj"
$EfiOut = Join-Path $BuildDir "BOOTX64.EFI"
$KernelObj = Join-Path $BuildDir "kernel_main.o"
$Mp4Obj = Join-Path $BuildDir "mp4.o"
$H264Obj = Join-Path $BuildDir "h264.o"
$KernelElf = Join-Path $BuildDir "kernel.elf"
$KernelBin = Join-Path $BuildDir "KERNEL.BIN"
$RawImage = Join-Path $BuildDir "vibio.img"
$VdiImage = Join-Path $BuildDir "vibio.vdi"
$SoundDir = Join-Path $Root "Assets\Sounds"
$HtmlDir = Join-Path $Root "Assets"
$VmName = "Vibio OS Dev"
$VBoxManage = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$python = (Get-Command py -ErrorAction Stop).Source

# Embed the browser's built-in HTML pages into a kernel header so they work with
# no disk and no file on the boot medium (real-hardware NVMe/USB boot). Pure
# standard-library Python, so the normal build stays Pillow-free.
& $python (Join-Path $Root "tools\make_html.py")
if ($LASTEXITCODE -ne 0) {
    throw "make_html.py failed"
}

$MediaOutDir = Join-Path $BuildDir "media"
& $python (Join-Path $Root "tools\make_media_assets.py") --out $MediaOutDir
if ($LASTEXITCODE -ne 0) {
    throw "make_media_assets.py failed"
}

# Native MP4 demuxer test assets (real ISO-BMFF containers; no ffmpeg needed):
# TEST.MP4 (H.264 Constrained Baseline 320x180 + AAC), UNSUP.MP4 (HEVC),
# BAD.MP4 (corrupt). Packed onto the VM disk by make_fat32_image.py.
& $python (Join-Path $Root "tools\make_mp4_assets.py") --out $MediaOutDir
if ($LASTEXITCODE -ne 0) {
    throw "make_mp4_assets.py failed"
}
$badPng = Join-Path $MediaOutDir "BAD.PNG"
$badJpg = Join-Path $MediaOutDir "BAD.JPG"
[System.IO.File]::WriteAllBytes($badPng, [byte[]](0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,0x00,0x00,0x00,0x00))
[System.IO.File]::WriteAllBytes($badJpg, [byte[]](0xFF,0xD8,0xFF,0xDA,0x00,0x04,0x00,0x00))

# Stage 37: build the read-only FAT32 USB test image QEMU exposes as the
# usb-storage stick (build\qemu-usb.img), so the post-boot xHCI USB driver can be
# exercised. Pure standard-library Python; QEMU-only file under build\.
& $python (Join-Path $Root "tools\make_usb_image.py")
if ($LASTEXITCODE -ne 0) {
    throw "make_usb_image.py failed"
}

$clang = (Get-Command clang -ErrorAction Stop).Source
$lld = (Get-Command lld-link -ErrorAction Stop).Source
$ldlld = (Get-Command ld.lld -ErrorAction Stop).Source
$objcopy = (Get-Command llvm-objcopy -ErrorAction Stop).Source

& $clang `
    -target x86_64-pc-win32 `
    -O2 `
    -ffreestanding `
    -fshort-wchar `
    -mno-red-zone `
    -fno-stack-protector `
    -fno-builtin `
    -fno-strict-aliasing `
    -fno-vectorize `
    -fno-slp-vectorize `
    -Wall `
    -Wextra `
    -I (Join-Path $Root "src\boot") `
    -I (Join-Path $Root "src\shared") `
    -c (Join-Path $Root "src\boot\main.c") `
    -o $BootObj
if ($LASTEXITCODE -ne 0) {
    throw "clang failed"
}

& $lld `
    /subsystem:efi_application `
    /entry:efi_main `
    /nodefaultlib `
    /machine:x64 `
    "/out:$EfiOut" `
    $BootObj
if ($LASTEXITCODE -ne 0) {
    throw "lld-link failed"
}

& $clang `
    -target x86_64-unknown-elf `
    -O2 `
    -ffreestanding `
    -mno-red-zone `
    -fno-stack-protector `
    -fno-builtin `
    -fno-pic `
    -fno-jump-tables `
    -fno-strict-aliasing `
    -fno-vectorize `
    -fno-slp-vectorize `
    -Wall `
    -Wextra `
    -I (Join-Path $Root "src\shared") `
    -I (Join-Path $Root "src\kernel") `
    -c (Join-Path $Root "src\kernel\main.c") `
    -o $KernelObj
if ($LASTEXITCODE -ne 0) {
    throw "kernel clang failed"
}

# Isolated MP4 (ISO-BMFF) demuxer/probe - its own translation unit so the MP4
# container code stays out of the 22k-line main.c and is simply linked in.
& $clang `
    -target x86_64-unknown-elf `
    -O2 `
    -ffreestanding `
    -mno-red-zone `
    -fno-stack-protector `
    -fno-builtin `
    -fno-pic `
    -fno-jump-tables `
    -fno-strict-aliasing `
    -fno-vectorize `
    -fno-slp-vectorize `
    -Wall `
    -Wextra `
    -I (Join-Path $Root "src\shared") `
    -I (Join-Path $Root "src\kernel") `
    -c (Join-Path $Root "src\kernel\mp4.c") `
    -o $Mp4Obj
if ($LASTEXITCODE -ne 0) {
    throw "mp4 clang failed"
}

& $clang `
    -target x86_64-unknown-elf `
    -O2 `
    -ffreestanding `
    -mno-red-zone `
    -fno-stack-protector `
    -fno-builtin `
    -fno-pic `
    -fno-jump-tables `
    -fno-strict-aliasing `
    -fno-vectorize `
    -fno-slp-vectorize `
    -Wall `
    -Wextra `
    -I (Join-Path $Root "src\shared") `
    -I (Join-Path $Root "src\kernel") `
    -c (Join-Path $Root "src\kernel\h264.c") `
    -o $H264Obj
if ($LASTEXITCODE -ne 0) {
    throw "h264 clang failed"
}

& $ldlld `
    -m elf_x86_64 `
    -T (Join-Path $Root "src\kernel\kernel.ld") `
    -nostdlib `
    -o $KernelElf `
    $KernelObj `
    $Mp4Obj `
    $H264Obj
if ($LASTEXITCODE -ne 0) {
    throw "kernel link failed"
}

& $objcopy -O binary $KernelElf $KernelBin
if ($LASTEXITCODE -ne 0) {
    throw "kernel objcopy failed"
}

$imageArgs = @(
    "-3",
    (Join-Path $Root "tools\make_fat32_image.py"),
    "--efi",
    $EfiOut,
    "--kernel",
    $KernelBin,
    "--out",
    $RawImage,
    "--label",
    "VIBIO OS",
    "--size-mib",
    "128"
)
if (Test-Path -LiteralPath $SoundDir) {
    $imageArgs += @("--sounds", $SoundDir)
}
if (Test-Path -LiteralPath $HtmlDir) {
    $imageArgs += @("--html", $HtmlDir)
}
if (Test-Path -LiteralPath $MediaOutDir) {
    $imageArgs += @("--media", $MediaOutDir)
}

function Update-BaselineWav {
    param(
        [Parameter(Mandatory=$true)][string]$SourceMp4,
        [Parameter(Mandatory=$true)][string]$DiskWav
    )
    if (!(Test-Path -LiteralPath $SourceMp4)) {
        return
    }
    $ffmpegCmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($null -eq $ffmpegCmd) {
        if (!(Test-Path -LiteralPath $DiskWav)) {
            Write-Host "build: ffmpeg not found; skipping $DiskWav"
        }
        return
    }
    $needsRefresh = !(Test-Path -LiteralPath $DiskWav)
    if (!$needsRefresh) {
        $srcTime = (Get-Item -LiteralPath $SourceMp4).LastWriteTimeUtc
        $wavTime = (Get-Item -LiteralPath $DiskWav).LastWriteTimeUtc
        $needsRefresh = $srcTime -gt $wavTime
    }
    if ($needsRefresh) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $DiskWav) | Out-Null
        & $ffmpegCmd.Source -hide_banner -loglevel error -y -i $SourceMp4 -vn -acodec pcm_s16le -ar 44100 -ac 2 $DiskWav
        if ($LASTEXITCODE -ne 0) {
            throw "ffmpeg failed while extracting $DiskWav"
        }
        Write-Host "build: $(Split-Path -Leaf $DiskWav) refreshed from $SourceMp4"
    }
}

# Keep the VM-facing baseline test name stable while using the real MP4 sample
# from Usb\ for MP4/H.264 playback tests.
$BaselineMp4Source = Join-Path $Root "Usb\vibio_test_baseline_320_aac - Copy.mp4"
$BaselineMp4Disk = Join-Path $Root "Disk\BASE320.MP4"
$BaselineMp4Wav = Join-Path $Root "Disk\BASE320.WAV"
if (Test-Path -LiteralPath $BaselineMp4Source) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $BaselineMp4Disk) | Out-Null
    Copy-Item -LiteralPath $BaselineMp4Source -Destination $BaselineMp4Disk -Force
    Write-Host "build: BASE320.MP4 refreshed from $BaselineMp4Source"
    Update-BaselineWav -SourceMp4 $BaselineMp4Source -DiskWav $BaselineMp4Wav
}

$Baseline1080Source = Join-Path $Root "Usb\vibio_test_baseline_1080p.mp4"
$Baseline1080Disk = Join-Path $Root "Disk\BASE1080.MP4"
$Baseline1080Wav = Join-Path $Root "Disk\BASE1080.WAV"
if (Test-Path -LiteralPath $Baseline1080Source) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Baseline1080Disk) | Out-Null
    Copy-Item -LiteralPath $Baseline1080Source -Destination $Baseline1080Disk -Force
    Write-Host "build: BASE1080.MP4 refreshed from $Baseline1080Source"
    Update-BaselineWav -SourceMp4 $Baseline1080Source -DiskWav $Baseline1080Wav
}

# Everything in the project's Disk\ folder is copied verbatim onto the boot disk
# (any file type, subfolders included) so it all shows up in Vibio's Files app.
$DiskDir = Join-Path $Root "Disk"
if (Test-Path -LiteralPath $DiskDir) {
    $imageArgs += @("--files", $DiskDir)
}

& $python @imageArgs
if ($LASTEXITCODE -ne 0) {
    throw "image builder failed"
}

if (Test-Path $VBoxManage) {
    $buildFullPath = [System.IO.Path]::GetFullPath($BuildDir)
    $vdiFullPath = [System.IO.Path]::GetFullPath($VdiImage)

    if (-not $vdiFullPath.StartsWith($buildFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a VDI outside the build directory: $vdiFullPath"
    }

    if (Test-Path $VdiImage) {
        try {
            & $VBoxManage showvminfo $VmName *> $null
            if ($LASTEXITCODE -eq 0) {
                & $VBoxManage storageattach $VmName --storagectl "SATA" --port 0 --device 0 --medium none *> $null
            }
        } catch {
        }

        try {
            & $VBoxManage closemedium disk $VdiImage --delete *> $null
        } catch {
        }
        if (Test-Path $VdiImage) {
            Remove-Item -LiteralPath $VdiImage -Force
        }
    }

    & $VBoxManage convertfromraw --format VDI $RawImage $VdiImage
    if ($LASTEXITCODE -ne 0) {
        throw "VBoxManage convertfromraw failed"
    }

    try {
        & $VBoxManage showvminfo $VmName *> $null
        if ($LASTEXITCODE -eq 0) {
            & $VBoxManage modifyvm $VmName `
                --firmware efi `
                --memory 512 `
                --vram 16 `
                --cpus 1 `
                --boot1 disk `
                --boot2 none `
                --boot3 none `
                --boot4 none `
                --nic1 none `
                --audio-enabled on `
                --audio-driver default `
                --audio-controller ac97 `
                --audio-out on `
                --audio-in off `
                --clipboard disabled `
                --drag-and-drop disabled `
                --usb off
            if ($LASTEXITCODE -ne 0) {
                throw "VBoxManage modifyvm failed"
            }

            & $VBoxManage storageattach $VmName `
                --storagectl "SATA" `
                --port 0 `
                --device 0 `
                --type hdd `
                --medium $VdiImage
            if ($LASTEXITCODE -ne 0) {
                throw "VBoxManage storageattach failed"
            }

            Write-Host "Attached $VdiImage to VM '$VmName'"

            # Stage 29: ensure the VM-only FAT32 read/write sandbox scratch disk
            # exists (create-if-missing so its contents survive kernel rebuilds)
            # and is attached at SATA port 1. The boot disk stays on port 0. This
            # only ever touches build\ and the dev VM - never host/USB/real disks.
            & (Join-Path $Root "tools\rw-sandbox.ps1")
            if ($LASTEXITCODE -ne 0) {
                throw "rw-sandbox.ps1 failed"
            }
        }
    } catch {
        throw
    }

    Write-Host "Built $VdiImage"
} else {
    Write-Host "Built $RawImage"
    Write-Host "VirtualBox was not found at $VBoxManage, so VDI conversion was skipped."
}
