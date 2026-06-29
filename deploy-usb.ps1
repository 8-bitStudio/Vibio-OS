param(
    [Parameter(Mandatory = $true)]
    [string]$Drive
)

# Copy the latest Vibio OS boot files onto a removable USB so a real laptop can
# boot them. This is a non-destructive copy: it does NOT format the drive and it
# only writes Vibio's own files. It refuses to run against a non-removable disk.
#
# Usage:
#   .\deploy-usb.ps1 -Drive G
#
# What it copies (mirrors the generated VM disk so the firmware-preloaded and
# FAT32 paths both find their files, while the browser home page is also baked
# into KERNEL.BIN so it works even if nothing else does):
#   build\BOOTX64.EFI -> <drive>:\EFI\BOOT\BOOTX64.EFI
#   build\KERNEL.BIN  -> <drive>:\KERNEL.BIN
#   Assets\START.HTM  -> <drive>:\START.HTM
#   Assets\*.HTM/PNG/JPG/BMP/GIF -> <drive>:\*.*
#   Assets\image files -> <drive>:\*.VIM generated browser fallback images
#   Assets\Sounds\*   -> <drive>:\SOUNDS\*  (8.3 names matching the VM image)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "build"

$letter = $Drive.TrimEnd(":").TrimEnd("\")
if ($letter.Length -ne 1) {
    throw "Pass a single drive letter, e.g. -Drive G"
}
# NOTE: PowerShell variable names are case-insensitive, so this must not be
# named $root (that would alias $Root, the project directory) — use $DriveRoot.
$DriveRoot = "${letter}:\"

# Safety: only ever write to a REMOVABLE drive, never a fixed/system disk.
$vol = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='${letter}:'" -ErrorAction SilentlyContinue
if ($null -eq $vol) {
    throw "Drive ${letter}: was not found."
}
# DriveType 2 = Removable. 3 = Local fixed disk.
if ([int]$vol.DriveType -ne 2) {
    throw "Drive ${letter}: is not a removable drive (DriveType=$($vol.DriveType)). Refusing to write. This script only deploys to USB."
}
if ($letter -ieq $env:SystemDrive.TrimEnd(":")) {
    throw "Drive ${letter}: is the Windows system drive. Refusing."
}

Write-Host "Target: ${letter}: ($($vol.VolumeName)) removable, $([math]::Round($vol.Size/1GB,1)) GB free $([math]::Round($vol.FreeSpace/1MB,0)) MB"

$efi = Join-Path $BuildDir "BOOTX64.EFI"
$kernel = Join-Path $BuildDir "KERNEL.BIN"
$startHtm = Join-Path $Root "Assets\START.HTM"
$assetsDir = Join-Path $Root "Assets"
$soundsDir = Join-Path $Root "Assets\Sounds"

foreach ($f in @($efi, $kernel)) {
    if (-not (Test-Path -LiteralPath $f)) {
        throw "Missing build output: $f  (run .\build.ps1 first)"
    }
}

# 1) Boot loader at the UEFI fallback path.
$efiDir = Join-Path $DriveRoot "EFI\BOOT"
New-Item -ItemType Directory -Force -Path $efiDir | Out-Null
Copy-Item -LiteralPath $efi -Destination (Join-Path $efiDir "BOOTX64.EFI") -Force
Write-Host "Copied BOOTX64.EFI -> ${letter}:\EFI\BOOT\BOOTX64.EFI"

# 2) Kernel (this is what carries the embedded browser home page).
Copy-Item -LiteralPath $kernel -Destination (Join-Path $DriveRoot "KERNEL.BIN") -Force
Write-Host "Copied KERNEL.BIN  -> ${letter}:\KERNEL.BIN"

# 3) START.HTM at the root, so the UEFI preload and FAT32 reader also find it.
if (Test-Path -LiteralPath $startHtm) {
    Copy-Item -LiteralPath $startHtm -Destination (Join-Path $DriveRoot "START.HTM") -Force
    Write-Host "Copied START.HTM   -> ${letter}:\START.HTM"
}

# 4) Root browser/image assets. The real USB boot fallback preloads only root
# files before ExitBootServices, so put the images where UEFI can read them and
# generate .VIM fallbacks for formats the kernel cannot decode natively yet.
if (Test-Path -LiteralPath $assetsDir) {
    $assetConvertScript = @'
import os
import shutil
import struct
import sys

try:
    from PIL import Image
except Exception:
    Image = None

ASSET_EXTS = {".htm", ".html", ".png", ".jpg", ".jpeg", ".bmp", ".gif"}
IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".gif"}
MAX_DIM = 2048

def short_name(path):
    stem, ext = os.path.splitext(os.path.basename(path))
    clean = "".join(ch for ch in stem.upper() if ch.isalnum())
    if not clean:
        clean = "ASSET"
    ext = ext.lower()
    if ext == ".html":
        ext3 = "HTM"
    elif ext == ".jpeg":
        ext3 = "JPG"
    else:
        ext3 = ext[1:].upper()[:3]
    return clean[:8] + "." + ext3

def convert_vim(src_path):
    with Image.open(src_path) as im:
        im = im.convert("RGBA")
        w, h = im.size
        scale = min(1.0, float(MAX_DIM) / float(max(w, h)))
        if scale < 1.0:
            im = im.resize((max(1, int(w * scale)), max(1, int(h * scale))), Image.Resampling.LANCZOS)
            w, h = im.size
        return b"VIMG" + struct.pack("<III", w, h, 1) + im.tobytes()

def main():
    src_dir, dst_dir = sys.argv[1], sys.argv[2]
    for name in sorted(os.listdir(src_dir)):
        src_path = os.path.join(src_dir, name)
        if not os.path.isfile(src_path):
            continue
        ext = os.path.splitext(name)[1].lower()
        if ext not in ASSET_EXTS:
            continue
        dst_name = short_name(src_path)
        shutil.copy2(src_path, os.path.join(dst_dir, dst_name))
        print("Copied {0} -> {1}".format(name, dst_name))
        if ext in IMAGE_EXTS:
            if Image is None:
                print("Skipped VIM for {0}: Pillow is not installed".format(name))
                continue
            vim_name = os.path.splitext(dst_name)[0] + ".VIM"
            with open(os.path.join(dst_dir, vim_name), "wb") as f:
                f.write(convert_vim(src_path))
            print("Generated {0} -> {1}".format(name, vim_name))
    if Image is not None:
        for name in sorted(os.listdir(dst_dir)):
            src_path = os.path.join(dst_dir, name)
            if not os.path.isfile(src_path):
                continue
            ext = os.path.splitext(name)[1].lower()
            if ext not in IMAGE_EXTS:
                continue
            vim_name = os.path.splitext(short_name(src_path))[0] + ".VIM"
            with open(os.path.join(dst_dir, vim_name), "wb") as f:
                f.write(convert_vim(src_path))
            print("Generated USB root {0} -> {1}".format(name, vim_name))

if __name__ == "__main__":
    main()
'@
    $tempScript = [System.IO.Path]::GetTempFileName() + ".py"
    try {
        Set-Content -LiteralPath $tempScript -Value $assetConvertScript -Encoding UTF8
        $py = Get-Command py -ErrorAction SilentlyContinue
        if ($null -ne $py) {
            & $py.Source -3 $tempScript $assetsDir $DriveRoot
        } else {
            $python = Get-Command python -ErrorAction SilentlyContinue
            if ($null -eq $python) {
                Write-Warning "Python was not found; copying original browser assets but skipping generated .VIM images."
                $assetExts = @(".htm", ".html", ".png", ".jpg", ".jpeg", ".bmp", ".gif")
                Get-ChildItem -LiteralPath $assetsDir -File | Where-Object { $assetExts -contains $_.Extension.ToLowerInvariant() } | ForEach-Object {
                    $base = [regex]::Replace($_.BaseName.ToUpperInvariant(), "[^A-Z0-9]", "")
                    if ($base.Length -eq 0) { $base = "ASSET" }
                    if ($base.Length -gt 8) { $base = $base.Substring(0, 8) }
                    $ext = $_.Extension.ToLowerInvariant()
                    if ($ext -eq ".html") {
                        $ext3 = "HTM"
                    } elseif ($ext -eq ".jpeg") {
                        $ext3 = "JPG"
                    } else {
                        $ext3 = $_.Extension.TrimStart(".").ToUpperInvariant()
                    }
                    $dstName = "$base.$ext3"
                    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $DriveRoot $dstName) -Force
                    Write-Host "Copied $($_.Name) -> ${letter}:\$dstName"
                }
            } else {
                & $python.Source $tempScript $assetsDir $DriveRoot
            }
        }
    } finally {
        Remove-Item -LiteralPath $tempScript -Force -ErrorAction SilentlyContinue
    }
}

# 5) Sounds under \SOUNDS using the same 8.3 names the VM image uses.
$soundMap = @{
    "Boot.wav"       = "BOOT.WAV"
    "Error.wav"      = "ERROR.WAV"
    "notify.wav"     = "NOTIFY.WAV"
    "Usb Insert.wav" = "USBINS.WAV"
    "Usb Remove.wav" = "USBRM.WAV"
}
if (Test-Path -LiteralPath $soundsDir) {
    $dstSounds = Join-Path $DriveRoot "SOUNDS"
    New-Item -ItemType Directory -Force -Path $dstSounds | Out-Null
    foreach ($src in $soundMap.Keys) {
        $srcPath = Join-Path $soundsDir $src
        if (Test-Path -LiteralPath $srcPath) {
            Copy-Item -LiteralPath $srcPath -Destination (Join-Path $dstSounds $soundMap[$src]) -Force
            Write-Host "Copied $src -> ${letter}:\SOUNDS\$($soundMap[$src])"
        }
    }
}

# 6) Video: copy any converted .VIV clips (and matching .WAV audio tracks) the
# build produced under build\media to the USB root. The bootloader preloads them
# at their exact size, so the real laptop can play them with no storage driver.
$mediaDir = Join-Path $BuildDir "media"
if (Test-Path -LiteralPath $mediaDir) {
    Get-ChildItem -LiteralPath $mediaDir -File | Where-Object { $_.Extension -ieq ".VIV" -or $_.Extension -ieq ".VVID" } | ForEach-Object {
        $dst = Join-Path $DriveRoot $_.Name.ToUpperInvariant()
        Copy-Item -LiteralPath $_.FullName -Destination $dst -Force
        Write-Host "Copied $($_.Name) -> ${letter}:\$($_.Name.ToUpperInvariant())  ($([math]::Round($_.Length/1MB,1)) MB)"
        $wav = [System.IO.Path]::ChangeExtension($_.FullName, ".WAV")
        if (Test-Path -LiteralPath $wav) {
            $wavDst = Join-Path $DriveRoot ([System.IO.Path]::GetFileName($wav).ToUpperInvariant())
            Copy-Item -LiteralPath $wav -Destination $wavDst -Force
            Write-Host "Copied $([System.IO.Path]::GetFileName($wav)) -> ${letter}:\$([System.IO.Path]::GetFileName($wav).ToUpperInvariant())"
        }
    }
}

Write-Host ""
Write-Host "Done. The USB now has the latest Vibio OS. Eject it safely, then boot the laptop from it."
Write-Host "The browser home page is embedded in KERNEL.BIN, so it works even if the laptop's disk driver finds no device."
