# Vibio OS

Vibio OS is starting as a tiny VM-only UEFI boot target. The first milestone is
not an installer and does not modify any host bootloader, EFI partition, Windows
disk, or Ubuntu disk.

The code here is from-scratch project code. It is not based on Linux, BSD, GRUB,
or another operating-system kernel.

## Safety Rules

- Build outputs stay under `build\`.
- The generated disk is a virtual image: `build\vibio.vdi`.
- The VirtualBox VM attaches only that generated virtual disk.
- Networking is disabled for the development VM.
- Do not run `diskpart`, `bcdedit`, boot-repair tools, or any installer scripts
  for this project unless you intentionally choose to build a real installer much
  later.
- Working milestone screenshots are saved under `docs\screenshots\`.
- Milestone notes are recorded in `docs\DEVELOPMENT_LOG.md`.

## Build

```powershell
.\build.ps1
```

This creates:

- `build\BOOTX64.EFI`
- `build\kernel.elf`
- `build\KERNEL.BIN`
- `build\vibio.img`
- `build\vibio.vdi`
- `build\vibio-rw-scratch.vdi` (Stage 29 VM-only FAT32 write sandbox, attached as
  a second SATA disk on port 1; the boot disk stays on port 0)

The scratch disk is created only if missing, so its contents survive rebuilds.
`tools\rw-sandbox.ps1` manages it: run it with no arguments to (re)create and
attach it, `-Recreate` to rebuild it fresh, or `-Detach` to remove it from the VM
(used for the write-safety negative test). It only ever touches files under
`build\` and the dev VM - never host, USB, or internal disks.

## Run In VirtualBox

```powershell
.\run-vbox.ps1
```

The VM is named `Vibio OS Dev`. It uses VirtualBox EFI firmware and boots from
`build\vibio.vdi`.

If you launch it from the VirtualBox GUI, choose `Vibio OS Dev`. The build
script also keeps the latest `build\vibio.vdi` attached to that VM so it can be
started from the GUI after rebuilding.

## Current Milestone

The current boot target is Stage 37:

- UEFI loads `\EFI\BOOT\BOOTX64.EFI` from the generated VM disk.
- The Vibio boot stage opens the same virtual disk filesystem.
- The boot stage loads `\KERNEL.BIN` into VM memory at `0x100000`.
- The boot stage gets the UEFI memory map.
- The boot stage exits UEFI boot services.
- The boot stage passes framebuffer and memory-map details to the kernel.
- The Vibio kernel draws its own screen through framebuffer memory.
- The Vibio kernel counts memory-map entries and usable RAM.
- The Vibio kernel has a simple physical page allocator.
- The allocator hands out 4 KiB pages and verifies test writes.
- The Vibio kernel builds its own initial x86_64 page tables.
- The kernel identity-maps the low 4 GiB and switches CR3 to its own PML4.
- The Vibio kernel installs an Interrupt Descriptor Table.
- The kernel catches a breakpoint exception through its own IDT.
- The kernel remaps the legacy PIC interrupt controller inside the VM.
- The kernel programs the PIT timer and receives timer interrupts.
- The kernel installs a PS/2 keyboard IRQ handler.
- The kernel keeps a small keyboard scancode ring buffer.
- The kernel draws a real text console area.
- The first built-in command prompt accepts typed input.
- The kernel has a first dynamic heap allocator for smaller allocations.
- The heap supports block splitting, freeing, and simple coalescing.
- The console state is allocated from the heap.
- The kernel has a first task table.
- Task records are allocated from the heap.
- Initial task records are `KERNEL`, `CONSOLE`, `IDLE`, and `INPUT`.
- The kernel can cooperatively context-switch between simple kernel tasks.
- The context switch saves/restores callee-saved registers and stack pointers.
- `CONSOLE` and `IDLE` run on their own stacks and yield back to the scheduler.
- The kernel has a first syscall boundary using interrupt vector `0x80`.
- Syscall number goes in `RAX`, and the return value comes back in `RAX`.
- Current syscall services expose ABI version, timer ticks, task count, and
  context-switch count.
- The timer IRQ now creates scheduler requests every fixed tick interval.
- The kernel responds to those requests by rotating ready kernel tasks.
- This is timer-driven scheduling for simple kernel tasks, not full protected
  user-process preemption yet.
- The kernel now loads its own Global Descriptor Table.
- The kernel defines separate kernel and future-user code/data selectors.
- The kernel creates and loads a Task State Segment.
- The syscall interrupt gate is now marked callable from future user mode.
- The generated VM boot disk includes WAV assets under `\SOUNDS\`.
- The VirtualBox dev VM exposes an AC'97 audio device.
- The kernel can find and initialize the AC'97 PCI audio controller.
- The kernel parses the packaged PCM WAV files and feeds 44.1 kHz 16-bit
  stereo buffers to AC'97 bus-master playback.
- `BOOT.WAV` starts automatically when the OS boots and audio initializes.
- The framebuffer renderer now avoids full-screen clearing during normal timer
  updates to reduce VirtualBox flicker.
- The boot stage now loads the known WAV files read-only before exiting UEFI
  boot services.
- The kernel receives a small in-memory file table for those loaded files.
- The `FILES` command lists loaded read-only files and total bytes.
- Stage 14 does not write to disks.
- The kernel can now find the VM AHCI/SATA controller.
- The kernel can read raw disk sectors through AHCI using the read-only
  `READ DMA EXT` command.
- The disk test reads the MBR and FAT32 boot sector and verifies signatures.
- On hardware without an AHCI SATA boot device, the AHCI/FAT32 status reports
  `N/A` instead of pretending the real PC failed a universal storage test.
- Stage 15 still has no disk write path.
- The kernel now parses FAT32 read-only from the sector reader.
- The FAT32 test finds the root directory, the `SOUNDS` directory, and the five
  WAV files.
- Stage 16 still has no filesystem write path.
- The kernel now has a read-only USB controller probe.
- The USB probe scans PCI for UHCI, OHCI, EHCI, and xHCI controller classes.
- The VM keeps USB disabled, so the probe safely reports zero controllers in
  the VM test.
- Stage 17 does not have a USB mass-storage read path yet.
- Stage 17 still has no USB or disk write path.
- The USB probe now builds a read-only controller table with up to 8 entries.
- Each table entry records type, bus/device/function, vendor/device ID, and the
  register base address read-only from PCI configuration space.
- The probe records a preferred controller type for real hardware, newest first
  (xHCI, then EHCI, OHCI, UHCI), and notes a real laptop will likely use xHCI.
- Stage 18 does not enable any controller, touch controller registers, or
  enumerate USB devices.
- Stage 18 still reports `STORAGE READ: NONE` and `WRITE PATH: NONE`.
- VirtualBox 7.2 needs the Extension Pack for emulated xHCI/EHCI, which is not
  installed, so the VM intentionally shows `CONTROLLERS: 0` cleanly.
- The kernel now has a PS/2 mouse driver on IRQ12 and a real on-screen cursor.
- The cursor uses save-under/restore so it glides over content without trails.
- The PS/2 mouse path uses fixed-point cursor motion and adaptive acceleration
  so small movement stays precise while faster movement crosses the desktop.
- A live `MOUSE x,y` readout and a `MOUSE` command show position, buttons, and
  packet count.
- Stage 19 begins the graphics-and-input path toward a Windows-like desktop.
- Stage 20 adds a draggable window surface: a title bar, border, and body text
  that floats over the screen and can be dragged by its title bar with the mouse.
- Stage 21 adds a window manager with multiple overlapping windows, click-to-
  focus and raise, and title-bar dragging.
- Rendering is now a compositor: a persistent base layer plus a back buffer; each
  frame composites the windows back-to-front and presents one finished frame, so
  overlapping windows are correct and nothing flickers.
- Stage 22 turns the old debug-panel screen into a desktop shell: a Windows-
  like wallpaper-first desktop, left-side desktop shortcuts, a bottom taskbar,
  and real window content kinds.
- The System window renders boot/self-test status inside its clipped content
  area when opened.
- The Terminal window renders the console history and prompt inside a movable
  window, and keyboard input opens/routes there.
- The Welcome window gives the desktop a simple starter surface when opened.
- The taskbar is left-aligned with a Vibio start button, search box, pinned app
  buttons, and live RTC time on the right.
- A small theme struct owns the desktop/window/taskbar colors so the UI can be
  tuned without rewriting the renderer.
- Current built-in commands are `HELP`, `CLEAR`, `MEM`, `HEAP`, `TASKS`,
  `TICKS`, `SCHED`, `USER`, `DISK`, `FS`, `USB`, `USBSTOR` (alias `USBMSD`),
  `USBHOTPLUG`, `USBTEST`, `FILESDBG`, `MOUSE`, `FILES`, `SOUNDS`, `BROWSER`,
  `SYSCALL`, `VERSION`, `SHUTDOWN`, `RESTART`, `FILESAPP` (alias `EXPLORER`),
  `LS`, `RWSTATUS`, `RWTEST`, `RWREAD`, and `RWDISABLE`.
- Typing `BOOT`, `ERROR`, `NOTIFY`, `USBINS`, or `USBRM` starts playback of
  the matching packaged WAV through the AC'97 driver when the audio device is
  available.
- The Stage 22 VM test injects `CLEAR`, `HELP`, and Enter through the virtual
  keyboard to prove input still reaches the Terminal window.
- A round of Stage 22 feedback then refined the shell:
  - Keyboard input is correct: Space no longer opens the Terminal, Caps Lock and
    Shift give real upper/lowercase, the full set of special characters types,
    and the pointer is more sensitive.
  - The desktop now shows only the apps that work (System, Terminal), drawn with
    real PNG icons from `Assets\Icons` (with the search and Wi-Fi UI icons in the
    taskbar). Sounds load from `Assets\Sounds`.
  - The Terminal opens at a sensible size, has 64 lines of scrollback (mouse
    wheel or Page Up/Down), a scrollbar, and a padded title.
  - Terminal input is cursor-aware: Ctrl+A moves to the start of the input line,
    Ctrl+C cancels with `^C`, Ctrl+V pastes the internal Terminal clipboard at
    the cursor, and Left/Right arrows move the cursor.
  - Trackpad/two-finger scroll is treated as wheel input and filtered away from
    pointer movement so scrolling Terminal history does not shove the cursor.
  - Windows open with a Windows 11-style rise-and-fade animation, centered.
  - Visual polish: a smooth gradient wallpaper with soft glows, window drop
    shadows, title-bar controls, and tighter System spacing.
  - Real fonts: Geist for all UI text and JetBrains Mono for the Terminal, built
    from TTFs in `Assets\Fonts` via `tools\make_fonts.py`.
- Stage 23 adds the first real sound driver:
  - VM audio is enabled with an AC'97 controller and output.
  - The kernel initializes the AC'97 mixer and PCM-out bus-master channel.
  - The packaged WAV files are parsed in the kernel; mono notification audio is
    expanded to stereo before playback.
  - The `SOUNDS` command reports `AUDIO: AC97 READY` when the device is ready.
  - Sound-name commands now start real playback instead of placeholder status.
  - Real PCs without AC'97 report audio as `N/A`; most modern hardware needs a
    future Intel HD Audio driver.
  - Intel HD Audio controllers are now detected and reported as `HDA FOUND
    DRIVER NEEDED` instead of being confused with AC'97.
  - USB insert/remove sound aliases (`USB_INSERT`, `USB REMOVE`, etc.) map to
    the packaged USB sounds, but automatic USB hotplug sounds still need a real
    xHCI/EHCI hotplug driver.
- Stage 24 adds read-only FAT32 file reads and a minimal HTML browser:
  - The kernel can now read an arbitrary file's bytes from the FAT32 boot disk
    by name, walking the cluster chain over the read-only AHCI sector path.
  - There is still no FAT, directory, or file write path of any kind.
  - The image builder packs top-level `Assets\*.htm` / `*.html` files into the
    FAT32 root, so `START.HTM` ships on the VM disk.
  - A from-scratch HTML browser app opens local pages: it parses headings,
    paragraphs, `br`, `hr`, links, images, and a few entities, lays out text
    with word wrapping in the Geist font, and scrolls (wheel/keyboard).
  - Links navigate between local files; `http(s)` links are shown as external
    because there is no network stack yet. Missing files and `<img>` show clean
    placeholders.
  - The browser opens automatically at boot showing `START.HTM`, and a desktop
    icon, taskbar button, and `BROWSER` command all open or reload it.
  - The browser home page is embedded directly in `KERNEL.BIN` (generated by
    `tools\make_html.py`), so it works on any machine with no disk and no file on
    the boot medium - including the real laptop where the kernel's own AHCI/FAT32
    driver finds no device (NVMe or GPT). The browser loads the built-in copy
    first, then a UEFI-preloaded copy, then a FAT32 disk read (with retries).
  - `deploy-usb.ps1` refreshes a removable USB with the latest boot files
    (removable-only, never formats). Because the page is in `KERNEL.BIN`, the
    laptop just needs the updated kernel copied over.
  - Window controls are real: minimize, maximize/restore, and close work.
    Opening still animates, but close/minimize hides immediately and forces a
    compositor redraw so stale window pixels do not smear over the desktop.
    Taskbar buttons minimize the focused window and restore it - they never
    close it.
  - The browser scrollbar is clickable and draggable; the mouse wheel / two-finger
    trackpad scroll works on IntelliMouse (ID 3/4) touchpads; and `F11` toggles
    true full screen (no chrome or taskbar), like a browser on Windows.
  - The framebuffer is mapped write-combining (PAT) so the per-frame blit is fast
    on real hardware - dragging and animations stay smooth, not just in the VM.
  - The PS/2 controller is forced to scancode-set-1 translation at init, so the
    keyboard (and F11) works on laptops whose firmware leaves it in set 2.
  - The browser toolbar now has back, forward, reload, home, and an editable
    address field (`Ctrl+L`, type, Enter). External addresses still show the
    no-network page instead of fetching anything.
  - The compositor now uses dirty-rectangle redraws and rectangular presents for
    normal window/cursor/taskbar changes, so it no longer copies the whole screen
    for every mouse move, window open, or small UI update. Fullscreen still
    repaints the full screen when its contents actually change.
- Stage 28 adds a read-only graphical Files app ("Vibio Files"):
  - A normal Vibio window that browses the real files on the FAT32 boot disk
    using the existing read-only file path - nothing is faked.
  - It shows the current path (starting at `/`), lists real folders and files
    with a type marker, type (DIR/FILE), and size, and clearly shows `READ ONLY`.
  - Folders open on Enter / double-click / the Open button; the Up button (or
    Backspace) returns to the parent folder.
  - Opening `.HTM`/`.HTML` launches the existing HTML viewer; opening `.WAV`
    plays through the existing audio driver and shows a file info panel; other
    types show a read-only info panel instead of crashing.
  - It is strictly read-only: there is no delete, rename, copy, move, create,
    save, or format - and no such buttons exist anywhere in the UI.
  - Only FAT32 8.3 short names are supported (no long file names yet); large
    directories show an honest truncation note; with no readable disk it shows an
    honest "FAT32 not available" message rather than fake entries.
  - Launchers: a desktop "FILES" icon, a taskbar button, and the `FILESAPP`
    (alias `EXPLORER`) command. The `LS` command lists the real root directory in
    the Terminal. The existing `FS`/`DISK`/`FILES` commands are unchanged.
- Stage 25 adds a real kernel panic path:
  - CPU exceptions for divide error, invalid opcode, double fault, general
    protection fault, and page fault now enter a panic handler instead of just
    freezing silently.
  - The panic handler draws directly to the framebuffer with a red critical
    problem screen.
  - The panic screen shows fault name, error code, RIP, RSP, RFLAGS, CR2,
    uptime, task, last driver, and last action.
  - After showing the screen for about 18 seconds, Vibio requests a reset using
    PCI reset control, PS/2 controller reset, then a triple-fault fallback.
  - The `PANIC` command intentionally triggers a test page fault at an unmapped
    canonical address so the red screen and restart path can be verified.
- The power menu now really works: Shut down performs a real ACPI soft-off (S5)
    so the VM window closes instead of hanging on a halt screen, and Restart
    cleanly reboots back to the desktop. The boot stage passes the ACPI root
    pointer to the kernel (read-only) so the kernel can find the FADT/`\_S5`
    sleep type. `SHUTDOWN` and `RESTART` terminal commands trigger the same
    paths. Shut down only ever uses ACPI shutdown registers and shutdown-only
    magic ports, never a reset port, and falls back to an honest halt screen if
    the host ignores every method.
- Stage 29 adds the first real disk-write path: a VM-only FAT32 read/write
  sandbox (experimental write support, not for real hardware).
  - Writes are armed ONLY when a disposable scratch disk is positively
    identified by ALL of: it is a separate non-boot AHCI disk, its FAT32 volume
    label is exactly `VIBIORW`, it has the marker file `VIBIO_RW.TXT` whose
    content begins `VIBIO_RW_OK`, and it has the preallocated `RWTEST.TXT`. If
    any gate fails, every write fails safely with a clear message.
  - The boot disk (and every other disk) is always treated as `READ ONLY`. The
    write code never touches the boot disk, USB, host, or internal disks.
  - `RWTEST` overwrites the single preallocated cluster of `RWTEST.TXT` with a
    `VIBIO RW SANDBOX OK SEQ=<n>` line, then verifies it both at the sector level
    and through the normal read-only FAT32 read path; it prints PASS only if both
    match, and never silently ignores a failed write. The written data persists
    across a full VM power cycle (`RWREAD` reads it back).
  - Commands `RWSTATUS`, `RWTEST`, `RWREAD`, and `RWDISABLE` drive and inspect the
    sandbox. `SELFTEST` reports `RW SANDBOX: AVAILABLE/SKIP/DISABLED` but never
    writes. The Files app stays strictly read-only with no write controls.
  - Still unsupported: delete/rename/move/copy, folder/file creation, file
    expansion, long file name writing, formatting, partition resize, bootloader
    install, and any real-hardware/USB/installer writes. No journaling yet.
- Stage 30 fixes the Files app storage backend and adds read-only USB
  storage diagnostics:
  - Bug fix: opening Files from the desktop icon or taskbar (not just the
    `FILESAPP` command) now loads the directory, so it no longer shows a false
    "Read-only FAT32 not available" with a healthy boot disk. The window reloads
    whenever it becomes visible and resets to root when it closes.
  - Files has a real storage-backend selection with priority AHCI boot-disk
    FAT32 -> read-only USB FAT32 -> none, and shows the chosen source in its
    status bar: `SOURCE: AHCI BOOT DISK - READ ONLY`,
    `SOURCE: USB MASS STORAGE - READ ONLY`, or `SOURCE: NONE`. When nothing is
    readable it gives an honest reason and never fakes a file list.
  - A new read-only USB mass-storage diagnostic probe records how far the USB
    path honestly gets: xHCI controller found -> capability registers read
    (read-only) -> root-hub port connect status. Full device enumeration and the
    Bulk-Only Transport SCSI read commands (INQUIRY / TEST UNIT READY /
    READ CAPACITY(10) / READ(10)) are not implemented yet, so USB never becomes a
    real Files backend and is reported honestly as not available. No USB writes.
  - New commands: `USBSTOR` (alias `USBMSD`) shows the layered USB storage
    status; `FILESDBG` shows which backend Files uses and why. `DISK` now shows
    `ROLE: BOOT DISK READ ONLY` / `FAT32 RO: MOUNTED`; `FS` shows its read-only
    AHCI source. `SELFTEST` gained `AHCI BOOT FAT32`, `USB STORAGE RO`, and
    `FILES BACKEND` lines and still performs no writes.
  - In the VM, Files browses the AHCI boot disk read-only; the Stage 29 VIBIORW
    scratch disk stays a separate write sandbox only and is never used by Files.
- Stage 31 adds USB port hotplug event tracking and USB insert/remove event
  sounds:
  - xHCI root-hub port state is polled when an xHCI controller is present and
    its PORTSC registers are safely readable. The first poll records a baseline;
    later disconnected -> connected transitions count as insert events, and
    connected -> disconnected transitions count as remove events.
  - Insert events use the existing `USBINS.WAV` asset. Remove events use
    `USBRM.WAV`. If audio is unavailable or an asset is missing, the event is
    still recorded and the command reports the sound result honestly.
  - `USB` now summarizes controllers plus hotplug status. `USBHOTPLUG` focuses
    on ports, connected bitmap/count, event counters, last event, and last sound.
    `USBTEST INSERT` / `USBTEST REMOVE` are simulated sound-path tests only.
  - USB storage is still unsupported beyond Stage 30 diagnostics. There is no
    USB device enumeration, USB mass-storage BOT/SCSI read path, USB media write,
    FAT write, installer, networking, bootloader change, or host/internal disk
    access. Files still prefers the read-only AHCI boot disk in the VM, and the
    VIBIORW sandbox remains VM-only.
- Stage 32 makes local browser images render:
  - The FAT32 image builder now packs top-level `.PNG` browser assets.
  - The UEFI boot stage preloads `TEST.PNG` so the browser can show the known
    test image even on real hardware where the kernel has no AHCI/NVMe/USB
    storage driver.
  - The browser now has a minimal PNG renderer for a safe first subset:
    8-bit RGB/RGBA, non-interlaced, filter 0, single uncompressed zlib/deflate
    IDAT block. Unsupported PNGs still show an honest placeholder/error.
  - Inline `<img src="TEST.PNG">` renders in `START.HTM`, and opening `.PNG`
    from Files routes to the browser image viewer. This is read-only and adds no
    disk, FAT, USB, or host write path.
- Stage 33 adds a real-hardware USB-boot fallback and broader packaged image
  support:
  - The boot file table now holds up to 32 read-only files, and the UEFI boot
    stage enumerates supported top-level files from the boot volume before
    `ExitBootServices`. On a real machine booted from USB, this gives the kernel
    a read-only `UEFI BOOT FILES` bundle even when the kernel has no AHCI/NVMe
    or xHCI mass-storage driver.
  - Files can now fall back to `SOURCE: UEFI BOOT FILES - READ ONLY`, after AHCI
    and future USB FAT32. This lets the real USB-boot machine browse the files
    that UEFI preloaded. It is not a full post-boot USB mass-storage stack.
  - The image builder keeps common top-level image files and also converts
    `.PNG`, `.JPG`, `.JPEG`, `.BMP`, and `.GIF` assets into `.VIM`, a simple
    RGBA bitmap format the kernel can render reliably. Large images are scaled
    down to fit the first-pass viewer.
  - Browser image rendering now supports direct `.VIM`, the Stage 32 simple PNG
    subset, and generated `.VIM` fallbacks for common image references. The start
    page includes `PHOTO.JPG` to prove the generated bitmap path.
  - Still unsupported: arbitrary post-boot USB directory reads, USB BOT/SCSI
    transfers, USB writes, compressed image decoding inside the kernel for every
    possible file, image saving/editing, network images, and installers.
- Stage 34 adds dedicated Media Viewer and Media Player apps with honest host-side
  conversion:
  - New large default windows (`Media Viewer`, `Media Player`) open at ~78% of
    the work area with fit/100%/zoom controls, pan, maximize, and F11 fullscreen.
  - `Media Viewer` displays `.VIMG`/`.VIM` natively, `.BMP` 24/32-bit natively,
    a safe PNG subset natively, and packaged `.PNG`/`.JPG`/etc. through matching
    `.VIMG` fallbacks generated at build time. Missing fallbacks show a clear
    rebuild/redeploy message (not a tiny browser placeholder).
  - `Media Player` plays Vibio-native `.VIV1` converted video with optional
    matching `.WAV` PCM audio. Real `.MP4` files are demuxed/probed natively
    (Stage 38, see below) but not decoded; actual MP4/WebM/MOV *playback* still
    needs host-side `ffmpeg` conversion. Vibio does not claim in-kernel
    H.264/VP9/AAC decode.
  - `tools/make_media_assets.py` (Pillow required, ffmpeg optional) scans
    `Assets`/`Assets/Media`, writes `.VIMG`/`.VIM` fallbacks, `VIDEOTST.VIV` +
    tone `.WAV`, and `MEDIA.MF` manifest metadata. The build fails clearly if
    Pillow is missing.
  - Files routes images to Media Viewer and `.VIV`/`.MP4` to Media Player.
    Terminal commands: `MEDIA`, `CODECS`, `VIEW`, `PLAY`, `VIMGINFO`, `VIVINFO`,
    `MP4INFO` (Stage 38), `IMGTEST`, `VIDEOTEST`. `SELFTEST` adds honest media
    lines (SKIP when assets missing).
- Stage 35 adds host-side ffmpeg video conversion, native graphics mode selection
  capped at 1920x1080, higher media limits, and VM verification of converted MP4
  playback.
- Stage 36 optimizes the real-hardware graphics path:
  - The boot and kernel builds now use optimized scalar code generation (`-O2`,
    no vectorization) instead of default debug-speed C loops.
  - The framebuffer fill and blit paths use clipped wide stores and fewer
    per-pixel function calls.
  - The compositor tracks multiple dirty rectangles instead of collapsing all
    changed areas into one large bounding rectangle.
  - Static F11 fullscreen surfaces no longer repaint the full screen only
    because the hidden shell clock ticked.
  - Follow-up polish keeps the stage at 36 while fixing clipped percent labels,
    WAV end-of-playback state, Files/Terminal-first taskbar pins, taskbar hover
    previews, desktop icon hover/selection state, maximized-titlebar restore,
    and an extra real-hardware-oriented PS/2 F11 scancode form.
  - A later Stage 36 follow-up adds a Windows-like desktop right-click menu,
    movable desktop icons, icon size/show/auto-arrange options, temporary
    in-memory new folder/text desktop items, and built-in wallpaper choices.
    These new desktop items are not persistent filesystem writes, and external
    `G:\Backgrounds` wallpaper loading is not implemented in the OS.
  - This is verified in VM/QEMU only; real PC/laptop performance still needs
    real-hardware testing by booting the updated image.
  - A later Stage 36 stability/polish pass (still Stage 36) makes the desktop
    feel like a normal OS: desktop icons single-click select / double-click open,
    small-icon labels truncate cleanly, the right-click submenu closes when the
    hovered item changes, the Files app gets a real vertical scrollbar (mouse
    wheel + track click), the Media Player restarts video from frame 0 after it
    finishes and restores audio immediately on unmute, and the wallpaper picker
    now renders the user's REAL photos imported from `G:\Backgrounds` (converted
    on the host into KERNEL.BIN by `tools\make_wallpapers.py`, not faked). The
    boot stage now loads up to 16 MB of `KERNEL.BIN` (was 512 KB) so the embedded
    wallpapers actually load. No new disk-write path; still VM-only and honest.
- Stage 37 adds a real read-only xHCI USB mass-storage stack, so Vibio reads
  files from a USB drive AFTER boot (no UEFI preload):
  - A from-scratch xHCI controller bring-up (reset, command/event rings, DCBAA,
    scratchpads, run), root-hub port reset, Enable Slot / Address Device, device
    + config descriptor reads, and Configure Endpoint for the bulk endpoints.
  - USB Mass Storage Bulk-Only Transport + SCSI, read-only: INQUIRY, TEST UNIT
    READY, READ CAPACITY(10), and READ(10). There is no WRITE(10), no format, no
    FAT write, no partition edit - strictly read-only.
  - The USB block device plugs into the existing read-only FAT32 parser, so the
    drive mounts and the Files app browses it as a live "USB MASS STORAGE"
    source with a Disk/USB toggle. Folders, long file names, and images/audio/
    video/docs open straight from USB.
  - New commands `XHCI`, `STORAGE`/`MOUNT`, and `USBFILES`; `USBSTOR`/`USBMSD`
    now report the real device/MSD/sector/FAT32 status. `SELFTEST` adds honest
    XHCI PCI DETECT / XHCI INIT / USB ENUM / USB MSD READ / USB FAT32 MOUNT lines
    (SKIP when there is no controller/device, never a fake PASS).
  - Verified in QEMU (q35 + qemu-xhci + usb-storage). The boot stage now loads
    the kernel by its exact size (OVMF rejects a fixed 16 MiB reservation), and
    the kernel maps the xHCI's high 64-bit MMIO BAR before use. Not yet tested on
    real hardware.
- Stage 37 follow-up (still Stage 37, bug-fix pass, no new disk-write path):
  - Desktop icon dragging no longer smears (old + new icon cells are redrawn),
    and dropped icons snap onto a clean grid.
  - The Files app lists up to 256 entries per folder and applies no file-size
    filter to the listing (large files appear; size limits only apply on open).
  - The Files scrollbar thumb can be grabbed and dragged (with capture), and a
    track click pages by a screenful.
  - The taskbar hover preview shows live per-app detail (Files path + item count,
    Browser page title, Media file + zoom / transport state) instead of just
    "Open window".
  - Wallpapers render with bilinear scaling, so the imported photos are smooth
    instead of blocky.
  - Media Viewer zoom is fixed: "-" always zooms out, "+" always zooms in, it
    clamps cleanly at 10%, and small zoom/pan changes are far less laggy.
  - AC'97 audio stops the instant the hardware DMA halts, removing the ~0.3 s
    start-of-clip replay blip after a sound finishes.
  - Mouse/drag/hover and audio behaviours are source-verified plus build/boot
    only (the harness cannot drive the guest mouse or capture audio); two-finger
    laptop trackpad scroll remains unsupported pending a real touchpad driver.
- Stage 37 follow-up #2 (still Stage 37):
  - Files now lists EVERY file on the boot volume, including types Vibio cannot
    open (e.g. a long-named `.mp4`). The UEFI boot-file enumeration used to drop
    unknown extensions; it now lists them by name + size (without loading the
    bytes), and opening one shows an honest "convert to .VIV on a PC" message.
  - Selecting multiple desktop icons and dragging now moves the whole group
    (built-in app icons; same-kind temporary items still share a selection).
  - DISK / FAT32 / AHCI BOOT FAT32 self-tests report N/A (not FAIL) on a machine
    with no AHCI SATA disk - the laptop's internal disk is NVMe, which Vibio has
    no driver for, so those tests are honestly N/A there, not failures.
- Stage 38 adds a native MP4 (ISO-BMFF) container demuxer - honest, limited, and
  real (not a VIV rename):
  - New isolated module `src/kernel/mp4.c` / `mp4.h` (its own translation unit,
    `<stdint.h>`-only, no I/O, no allocation) parses ftyp / moov / mvhd / trak /
    tkhd / mdia / mdhd / hdlr / minf / stbl / stsd / stts / stsc / stsz /
    stco / co64 / mdat with strict bounds checking. Vibio has no ELF/user-process
    loader, so it is linked into the kernel but kept fully separate from
    `main.c`; KERNEL.BIN grew only ~19 KB.
  - Codec/profile detection: avc1/avc3 -> H.264 (with the real profile /
    constraint / level read from the avcC record, e.g. "Constrained Baseline,
    Level 3.0"), hvc1/hev1 -> H.265, av01 -> AV1, vp09 -> VP9, mp4v -> MPEG-4,
    mp4a -> AAC, .mp3 -> MP3, ac-3/ec-3 -> AC-3, Opus.
  - New `MP4INFO <file>` command prints size, duration, timescale, track count,
    video/audio codecs, WxH, H.264 profile/level, sample/chunk counts, the stbl
    table presence, whether decode is supported, and the exact unsupported reason.
  - Opening a `.MP4`/`.M4V`/`.MOV` in the Media Player shows a "NATIVE MP4 PROBE"
    surface with those facts and a clear "Decode: NOT supported (demux/probe
    only)" line - corrupt and unsupported-codec files show a clear message and
    never crash. (A host-converted companion `.VIV`, if present, still plays via
    the existing path, honestly labelled as host conversion.)
  - There is still NO full in-kernel H.264 or AAC *decoder*: a real `.mp4`'s
    pixels and audio are not rendered. The honest demuxer + probe layer is done,
    and the H.264 pixel pipeline now reconstructs multiple luma rows from the
    first IDR slice: for `BASE320.MP4` Vibio decodes 78 luma macroblocks and
    exposes 3 complete macroblock rows (320x48 luma, 15360 samples), with
    cross-macroblock CAVLC nC tracking, top-neighbour nC tracking,
    cross/top-macroblock intra prediction, and `I_16x16` luma DC/AC
    reconstruction. MB0 remains BIT-EXACT against ffmpeg (0/256), the first row
    is BIT-EXACT (0/5120), the 320x48 region is BIT-EXACT (0/15360), and MB3
    `I_16x16` remains BIT-EXACT (0/256), verified by
    `tools/h264_verify_mb0.py`. Media Player draws the decoded luma region as
    grayscale and still reports native MP4 decode as unsupported. No chroma
    pixels, no full-frame/multi-slice completion, no playback.
  - Test assets (`tools/make_mp4_assets.py`, no ffmpeg needed): TEST.MP4 (H.264
    Constrained Baseline 320x180 + AAC-LC 44100 stereo, real container + codec
    config, stub coded frames), UNSUP.MP4 (H.265, unsupported), BAD.MP4 (corrupt).
  - **Drop-folder storage (no curation):** anything you put in the project's
    `Disk/` folder is copied VERBATIM onto Vibio's boot disk (any file type,
    subfolders included) and anything in `Usb/` is copied onto the emulated USB
    stick. After `build.ps1` they all appear in the Files app and open by their
    real names - drop your `.mp4` in `Disk/`, rebuild, and Vibio reads it from the
    real storage. `make_fat32_image.py --files <dir>` does the recursive packing
    (proper 8.3 + VFAT long names, nested dirs); the image auto-grows to fit. On
    real hardware just copy files onto the actual USB - no folder needed.
  - Build fix: `tools/make_fat32_image.py` now writes the FAT32 root directory as
    a proper multi-cluster chain, so the disk can hold more than 32 root entries
    (previously extra assets, including VIDEOTST.VIV, were silently dropped).

This proves the basic bootloader/kernel split without touching host disks or
using an existing OS kernel.

Latest screenshot:

- `docs\screenshots\2026-06-24-timer-driven-scheduling.png`
- `docs\screenshots\2026-06-24-vm-boot-fix-stage12.png`
- `docs\screenshots\2026-06-24-stage13-user-boundary.png`
- `docs\screenshots\2026-06-24-stage13-sound-assets.png`
- `docs\screenshots\2026-06-24-flicker-safe-rendering-sound-status.png`
- `docs\screenshots\2026-06-24-stage14-readonly-files.png`
- `docs\screenshots\2026-06-24-stage15-readonly-disk-io.png`
- `docs\screenshots\2026-06-24-stage16-fat32-readonly.png`
- `docs\screenshots\2026-06-24-stage17-usb-readonly-probe.png`
- `docs\screenshots\2026-06-24-stage18-usb-controller-arch.png`
- `docs\screenshots\2026-06-24-stage18-boot-sequence.png`
- `docs\screenshots\2026-06-24-stage19-mouse-cursor.png`
- `docs\screenshots\2026-06-24-stage20-window.png`
- `docs\screenshots\2026-06-24-stage21-window-manager.png`
- `docs\screenshots\2026-06-25-stage22-desktop-shell.png`
- `docs\screenshots\2026-06-25-stage22-windows-like-desktop.png`
- `docs\screenshots\2026-06-25-stage22-terminal-open-check.png`
- `docs\screenshots\2026-06-25-stage22-desktop-polish.png`
- `docs\screenshots\2026-06-25-stage22-clear-toggle-polish.png`
- `docs\screenshots\2026-06-25-stage22-modern-desktop.png`
- `docs\screenshots\2026-06-25-stage22-modern-terminal-check.png`
- `docs\screenshots\2026-06-25-stage22-input-special-chars.png`
- `docs\screenshots\2026-06-25-stage22-png-icons-desktop.png`
- `docs\screenshots\2026-06-25-stage22-terminal-scrollback.png`
- `docs\screenshots\2026-06-25-stage22-open-animation-mid.png`
- `docs\screenshots\2026-06-25-stage22-visual-polish.png`
- `docs\screenshots\2026-06-25-stage22-fonts-geist-jbm.png`
- `docs\screenshots\2026-06-25-stage22-fonts-terminal-mono.png`
- `docs\screenshots\2026-06-25-stage23-ac97-sounds.png`
- `docs\screenshots\2026-06-25-stage23-error-playback.png`
- `docs\screenshots\2026-06-25-stage23-mouse-hardware-status-audio.png`
- `docs\screenshots\2026-06-25-stage23-underscore-icons-bootsound.png`
- `docs\screenshots\2026-06-25-stage23-ctrl-paste.png`
- `docs\screenshots\2026-06-25-stage23-ctrl-a-cursor.png`
- `docs\screenshots\2026-06-25-stage23-ctrl-c-cancel.png`
- `docs\screenshots\2026-06-25-stage23-ctrl-v-cursor.png`
- `docs\screenshots\2026-06-25-stage23-usb-insert-sound-alias.png`
- `docs\screenshots\2026-06-25-stage23-notify-sound.png`
- `docs\screenshots\2026-06-25-stage24-browser-start.png`
- `docs\screenshots\2026-06-25-stage24-browser-links.png`
- `docs\screenshots\2026-06-25-stage24-browser-preloaded.png`
- `docs\screenshots\2026-06-25-stage24-browser-embedded.png`
- `docs\screenshots\2026-06-25-stage24-browser-fullscreen.png`
- `docs\screenshots\2026-06-26-stage24-dirty-rect-browser.png`
- `docs\screenshots\2026-06-26-stage24-dirty-rect-fullscreen.png`
- `docs\screenshots\2026-06-26-stage25-panic-normal.png`
- `docs\screenshots\2026-06-26-stage25-panic-red-screen.png`
- `docs\screenshots\2026-06-26-stage25-close-clean-hide.png`
- `docs\screenshots\2026-06-26-stage25-help-shutdown-restart.png`
- `docs\screenshots\2026-06-26-stage25-restart-rebooted.png`
- `docs\screenshots\2026-06-25-stage28-files-root.png`
- `docs\screenshots\2026-06-25-stage28-files-sounds.png`
- `docs\screenshots\2026-06-25-stage28-files-wav-info.png`
- `docs\screenshots\2026-06-25-stage28-files-open-start.png`
- `docs\screenshots\2026-06-25-stage28-ls.png`
- `docs\screenshots\2026-06-25-stage28-selftest.png`
- `docs\screenshots\2026-06-25-stage28-version.png`
- `docs\screenshots\2026-06-25-stage29-version.png`
- `docs\screenshots\2026-06-25-stage29-selftest.png`
- `docs\screenshots\2026-06-25-stage29-rwstatus.png`
- `docs\screenshots\2026-06-25-stage29-rwtest-pass.png`
- `docs\screenshots\2026-06-25-stage29-rw-persistence.png`
- `docs\screenshots\2026-06-25-stage29-files-readonly.png`
- `docs\screenshots\2026-06-25-stage29-rwstatus-missing.png`
- `docs\screenshots\2026-06-25-stage29-rwtest-refused.png`
- `docs\screenshots\2026-06-25-stage30-files-root-fixed.png`
- `docs\screenshots\2026-06-25-stage30-files-sounds-fixed.png`
- `docs\screenshots\2026-06-25-stage30-files-open-start-fixed.png`
- `docs\screenshots\2026-06-25-stage30-usbstor.png`
- `docs\screenshots\2026-06-25-stage30-filesdbg.png`
- `docs\screenshots\2026-06-25-stage30-selftest.png`
- `docs\screenshots\2026-06-25-stage30-rwstatus.png`
- `docs\screenshots\2026-06-25-stage30-rwtest-pass.png`
- `docs\screenshots\2026-06-25-stage30-files-noscratch.png`
- `docs\screenshots\2026-06-25-stage30-rwtest-refused.png`
- `docs\screenshots\2026-06-25-stage30-version.png`
- `docs\screenshots\2026-06-25-stage31-usb-hotplug-status.png`
- `docs\screenshots\2026-06-25-stage31-usb-sound-test.png`
- `docs\screenshots\2026-06-25-stage32-browser-image.png`
- `docs\screenshots\2026-06-25-stage33-browser-images.png`
- `docs\screenshots\2026-06-27-stage36-browser-fullscreen.png`
- `docs\screenshots\2026-06-27-stage36-video-playback.png`
- `docs\screenshots\2026-06-27-stage36-qemu-boot.png`
- `docs\screenshots\2026-06-26-stage36-stability-default-desktop.png`
- `docs\screenshots\2026-06-26-stage36-stability-wallpaper-image.png`
- `docs\screenshots\2026-06-26-stage36-stability-files-open.png`
- `docs\screenshots\2026-06-26-stage36-stability-restart.png`
- `docs\screenshots\2026-06-27-stage37-qemu-boot.png`
- `docs\screenshots\2026-06-27-stage37-usb-files.png`
- `docs\screenshots\2026-06-27-stage37-usb-docs-folder.png`
- `docs\screenshots\2026-06-27-stage37-usb-open-image.png`
- `docs\screenshots\2026-06-27-stage37-vbox-nousb-restart.png`
- `docs\screenshots\2026-06-27-stage37-fixes-desktop.png`
- `docs\screenshots\2026-06-27-stage37-fixes-files-listing.png`
- `docs\screenshots\2026-06-27-stage37-fixes-media-viewer.png`
- `docs\screenshots\2026-06-27-stage37-fixes-wallpaper-bilinear.png`
- `docs\screenshots\2026-06-27-stage37-fixes-restart.png`

Longer roadmap:

- `docs\ROADMAP_TO_BROWSER.md`
