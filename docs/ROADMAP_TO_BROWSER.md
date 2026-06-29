# Vibio OS Roadmap To A Browser

This is the rough path from the current tiny kernel to something that could run
a browser-like application. It is intentionally plain English.

## Where We Are Now

Vibio can boot in a VM, leave UEFI boot services, manage basic physical pages,
install its own page tables, load an IDT, catch a breakpoint exception, receive
timer interrupts, receive keyboard interrupts, draw a text console, and accept
the first built-in commands. It now also has a first kernel heap that can carve
pages into smaller dynamic allocations. The kernel can create task records and
list them from the prompt. Vibio can also cooperatively switch between simple
kernel tasks on separate stacks. It now has a first syscall boundary using
interrupt vector `0x80`. The timer interrupt now drives scheduler requests, and
the kernel responds by rotating ready kernel tasks. Vibio now also loads its
own GDT, defines kernel and future-user selectors, loads a TSS, and marks the
syscall interrupt gate as callable from future user mode. Stage 14 adds a safe
read-only file-loading step: the boot stage reads known WAV files into memory
and passes a file table to the kernel. Stage 15 adds a kernel-side AHCI read
path that reads raw disk sectors without a write command. Stage 16 parses the
FAT32 boot disk read-only and finds the packaged sound files by directory entry.
Stage 17 adds the first read-only USB controller probe. In the safety VM, USB
is still disabled, so the probe reports zero controllers while confirming there
is no USB storage read path and no write path. Stage 18 turns that probe into a
proper read-only controller architecture: it builds a controller table that
records each controller's type, location, and register base, picks the preferred
real-hardware controller type (xHCI first), and still reports zero controllers
cleanly in the safety VM without enabling USB or touching controller registers.
Stage 19 starts the graphics-and-input path: a PS/2 mouse driver on IRQ12, a
real arrow cursor that glides over content using save-under/restore, drawing
primitives, and a live coordinate readout plus a `MOUSE` command. Stage 20 adds
a draggable window surface. Stage 21 adds the compositor and window manager.
Stage 22 turns the screen into a Windows-like desktop shell with left-side
shortcuts, a bottom taskbar, pinned window buttons, System/Welcome/Terminal
content kinds, and keyboard input rendered inside the Terminal window. Stage 23
adds the first real audio path: the VM exposes AC'97, the kernel initializes the
PCI audio controller, parses packaged PCM WAV files, plays the boot sound at OS
startup, and lets sound-name commands start playback through the driver. The
desktop now treats AHCI/FAT32/AC'97 as hardware-specific probes: unsupported
real hardware reports `N/A` instead of pretending those drivers are universal.
Stage 25 adds a red kernel panic screen for serious CPU faults, including page
faults, with RIP/RSP/RFLAGS/CR2/error-code context and an automatic restart.

That means Vibio has the beginning of:

- Boot
- Screen drawing
- Memory discovery
- Physical page allocation
- Virtual-memory control
- CPU exception handling
- Hardware timer interrupts
- Keyboard interrupt input
- Text console drawing
- First command prompt
- Kernel heap allocation
- Task table
- Cooperative kernel context switching
- Syscall boundary
- Timer-driven scheduling
- User/kernel boundary
- Packaged sound assets
- Read-only boot file table
- Read-only AHCI disk I/O
- Read-only FAT32 filesystem
- Read-only USB controller discovery
- Read-only USB controller table and preferred-controller selection
- USB port connect/disconnect hotplug event detection on readable xHCI root-hub
  ports, with one-shot USB insert/remove WAV sound events
- PS/2 mouse driver, on-screen cursor, and drawing primitives
- First draggable window surface (title bar, border, body), composited over the screen
- Compositor (base layer + back buffer) and a window manager: multiple overlapping windows, click-to-focus/raise, title-bar dragging
- First full desktop shell with taskbar, RTC clock, window buttons, and
  System/Welcome/Terminal content windows
- AC'97 audio initialization, WAV parsing, boot sound playback, and command-
  triggered playback for packaged sounds
- Read-only FAT32 file reads (walking the cluster chain) and a from-scratch
  minimal HTML browser app that opens local `.HTM`/`.HTML` files from the disk
- Kernel panic diagnostics for serious CPU exceptions
- A first read-only browser image path for simple local PNG files
- A UEFI-preloaded boot-file fallback that lets Files/browser use packaged
  read-only files on USB-boot hardware before a full USB storage driver exists
- Build-time conversion of common image assets into a simple kernel-renderable
  bitmap format
- Real ACPI shutdown (S5) and reliable restart from the power menu
- Read-only graphical Files app ("Vibio Files"): browses real FAT32 folders/files
  (8.3 names), opens `.HTM`/`.HTML` in the HTML viewer and plays `.WAV`, with no
  write/delete/rename/create UI of any kind

## Windows-Like End Goal

The long-term UI target is a Windows-like desktop: taskbar, start-menu-style
launcher, movable windows, app surfaces, mouse cursor, keyboard shortcuts, file
manager, settings, and familiar window controls. That does not mean copying
Windows code. Vibio should stay a from-scratch OS with its own kernel and UI
code, just aiming for a familiar desktop shape.

## The Next Steps

1. **USB device enumeration (real-hardware first)**
   - Build on the Stage 18 USB controller table.
   - On a machine that exposes a USB controller, bring one controller up
     read-only and enumerate attached devices.
   - Recognize a USB mass-storage class device, but stop before reading storage.
   - VM testing this needs the Extension Pack and an emulated controller, so
     expect to prove it on real hardware or a disposable VM image.
   - Do not add USB or disk writes yet.

2. **USB mass storage read-only**
   - Build on USB device enumeration.
   - Add a read-only USB mass-storage path for USB boot.
   - Keep device commands limited to inquiry/capacity/read-style commands.
   - Do not add USB or disk writes yet.

3. **Program loading**
   - Load a tiny program file from FAT32.
   - Run it in controlled kernel space first.
   - Later move it to user mode through the Stage 13 boundary.

4. **Stronger command prompt**
   - Keep adding built-in commands.
   - Add command history and editing.
   - Later move the shell into user space.

5. **Graphics stack (done for now)**
   - Done in Stage 19: PS/2 mouse, on-screen cursor, save-under/restore, and
     basic drawing primitives.
   - Done in Stage 20: a single draggable window surface.
   - Done in Stage 21: a compositor (base layer + back buffer) that draws the
     whole scene offscreen and presents it, so overlapping windows work and
     flicker is gone for good.

6. **Window manager (in progress, current path)**
   - Done in Stage 21: multiple overlapping windows, focus, z-order, click-to-
     raise, and title-bar dragging.
   - Done in Stage 22: a full desktop shell with wallpaper, desktop shortcuts,
     left-aligned taskbar buttons, an RTC clock, and
     System/Welcome/Terminal window content kinds.
   - Next: a launcher/start-menu-style entry point plus close/minimize window
     controls.

7. **Audio driver (started)**
   - Done in Stage 23: add a VM AC'97 audio-device driver.
   - Done in Stage 23: parse PCM WAV data.
   - Done in Stage 23: play the packaged boot, error, notification, and USB
     sounds.
   - Next for real PCs: add Intel HD Audio detection and minimal playback.
   - Next: add a reusable kernel audio stream/ring buffer so future apps can
     push continuous PCM instead of one complete WAV at a time.
   - Later: add mixing, volume control, and a syscall/user boundary for audio
     clients.

8. **Networking**
    - Add a network device driver for the VM.
    - Implement Ethernet, ARP, IP, UDP, TCP, and DNS.

9. **Browser path (started)**
    - Done in Stage 24: a basic HTML text viewer. The kernel reads a local
      `.HTM`/`.HTML` file from the FAT32 disk (read-only) and a from-scratch
      browser app parses a useful subset of HTML (headings, paragraphs, `br`,
      `hr`, links, images, entities), wraps text in the Geist font, follows
      local links, and scrolls.
    - Done in Stage 24 Pass 6: back/forward history, reload, home, and an
      editable address bar for local files and no-network external URLs.
    - Next: a wider HTML/CSS subset and a real PNG decoder for `<img>`.
    - Later, once networking exists: a tiny HTTP client, then a small graphical
      HTML/CSS renderer.
    - A modern browser engine is a very large project, so the realistic first
      browser is simple and custom, not Chrome-level.

10. **Debuggability**
    - Done in Stage 25: red kernel panic screen for serious CPU faults.
    - Next: serial logging for panic/fault details, so failures before graphics
      setup are still visible in VM logs.
    - Next: a recent-action ring buffer for driver, filesystem, browser, and UI
      operations.

## Important Reality Check

A browser needs a lot beneath it: files, memory protection, processes, graphics,
input, fonts, networking, TLS, and a rendering engine. So the road to a browser
is long, but each milestone makes Vibio more like a real OS.

## Disk Write Policy

Disk writes should be added only after:

- Read-only disk I/O works reliably.
- A read-only filesystem works reliably.
- Writes are tested first against disposable VM images only.
- There is a clear backup/recovery story.
- Real USB or laptop disks remain read-only until VM write tests are boring.
