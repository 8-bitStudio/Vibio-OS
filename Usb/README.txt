Vibio OS - USB drop folder
==========================

Everything you put in this folder is copied VERBATIM onto the emulated USB
mass-storage stick that QEMU presents to Vibio (build/qemu-usb.img) when you
run build.ps1 - any file type, any name, subfolders and all.

After building, run the QEMU rig (tools/run-qemu.ps1) and the files show up in
Vibio's Files app under the "USB MASS STORAGE" source, read live over the
post-boot xHCI + USB Mass Storage driver (no UEFI preload).

On REAL hardware you don't need this folder at all: just copy your files onto a
real USB stick and Vibio reads them directly the same way.
