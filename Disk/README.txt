Vibio OS - Disk drop folder
===========================

Everything you put in this folder is copied VERBATIM onto Vibio's boot disk
(the AHCI SATA disk in the VM) when you run build.ps1 - any file type, any
name, subfolders and all. No conversion, no allow-list.

After building, the files show up in Vibio's Files app under the boot disk and
can be opened (images in the Media Viewer, .MP4 in the Media Player's native
MP4 probe, .WAV/.VIV in the player, .HTM in the browser, etc.).

The disk image auto-grows to fit whatever you drop here, so large files are
fine. Just drop files in, run build.ps1, and they appear in Vibio.

(This folder is for YOUR files. The OS's own files - kernel, bootloader,
sounds, the browser's built-in pages, generated image fallbacks - are added
automatically and do not need to live here.)
