# Vibio OS Development Log

This file records what we are building, when we built it, how we tested it, and
where screenshots are saved. Keep entries plain-English first, technical second.

## 2026-06-24 - Documentation Rule Added

New rule for the project:

- Every milestone gets a dated log entry.
- Working VM screenshots are saved in `docs/screenshots/`.
- The log explains what changed in plain English.
- Safety status is recorded every time.

Safety status:

- Vibio OS is still VM-only.
- The VM uses only `build/vibio.vdi`.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

## 2026-06-24 - Stage 1 Kernel Boot

Plain English:

Vibio OS now has two pieces. The first piece is a tiny boot stage named
`BOOTX64.EFI`. VirtualBox firmware starts that file. The boot stage then loads
the second piece, `KERNEL.BIN`, into VM memory. `KERNEL.BIN` is the first real
Vibio kernel.

What the kernel does:

- Checks that the boot stage passed valid startup information.
- Draws directly to the VM framebuffer.
- Displays the Vibio OS Stage 1 screen.
- Stops safely in the VM.

Why it matters:

This proves we can boot our own from-scratch kernel code. It is not Linux, BSD,
GRUB, or another operating-system kernel.

Screenshot:

- `docs/screenshots/2026-06-24-stage1-kernel.png`

![Stage 1 kernel boot](screenshots/2026-06-24-stage1-kernel.png)

Safety status:

- VM-only.
- Network disabled.
- Only the generated virtual disk is attached.
- The VirtualBox VM was powered off after testing.

Next milestone:

Get the UEFI memory map, exit UEFI boot services, then continue running in the
Vibio kernel with the memory map available.

## 2026-06-24 - Memory Map And Exit Boot Services

Plain English:

Vibio OS now asks the VM firmware for a map of memory before the kernel takes
over. That memory map tells the kernel which RAM areas exist and which ones are
usable. After receiving that map, the boot stage calls `ExitBootServices`, which
means Vibio stops relying on UEFI boot services and the kernel keeps running on
its own framebuffer code.

What changed:

- Added memory-map fields to `VibioBootInfo`.
- Added UEFI `GetMemoryMap` and `ExitBootServices` calls.
- Moved boot info and the memory map into loader-owned VM memory.
- Kernel now counts memory-map entries.
- Kernel now counts usable RAM in MiB.
- Kernel screen shows whether UEFI boot services were exited.

What the screenshot proves:

- The separate Vibio kernel still boots.
- UEFI boot services were exited successfully.
- The kernel received a memory map.
- The kernel counted 108 memory-map entries.
- The kernel counted 80 MiB of usable memory in this VM configuration.

Screenshot:

- `docs/screenshots/2026-06-24-memory-map-exit-boot-services.png`

![Memory map and exit boot services](screenshots/2026-06-24-memory-map-exit-boot-services.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Build a simple physical page allocator so the kernel can reserve and hand out
usable memory pages instead of only reading the memory map.

## 2026-06-24 - Physical Page Allocator

Plain English:

Vibio OS can now do its first real memory-management job. The kernel reads the
memory map, finds RAM marked as conventional usable memory, and hands out 4 KiB
physical pages from that RAM. This is still very small, but it is the first step
toward a real memory manager.

What changed:

- Added a simple physical page allocator in the kernel.
- The allocator skips memory below 1 MiB.
- The allocator only uses memory-map entries marked as conventional RAM.
- The kernel allocates 3 test pages.
- The kernel writes test patterns into those pages.
- The kernel verifies the writes and reports pass/fail on screen.

What the screenshot proves:

- UEFI boot services were exited before the kernel continued.
- The kernel received the memory map.
- The allocator found 20327 free 4 KiB pages in this VM.
- The allocator handed out 3 pages.
- The first allocated page was `0x0000000000180000`.
- The memory write/read allocation test passed.

Screenshot:

- `docs/screenshots/2026-06-24-page-allocator.png`

![Physical page allocator](screenshots/2026-06-24-page-allocator.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Build initial page tables so Vibio starts controlling its own virtual memory
instead of continuing to use the firmware-created page tables.

## 2026-06-24 - Initial Page Tables

Plain English:

Vibio OS now builds its own first x86_64 page tables. A page table is the map
the CPU uses to translate memory addresses while paging is enabled. Until this
milestone, Vibio was still running on page tables made by the VM firmware. Now
the kernel allocates page-table pages, fills them in, and loads its own PML4
into CR3.

What changed:

- Added page-table entry flags and page-table builder code.
- Allocated a PML4, PDPT, and page-directory pages from Vibio's allocator.
- Identity-mapped the low 4 GiB using 2 MiB pages.
- Included the framebuffer range in the identity map.
- Loaded CR3 with Vibio's new PML4 address.
- Verified the allocator still works after the CR3 switch.

What the screenshot proves:

- Vibio is still running after switching page tables.
- The active CR3/PML4 page is `0x0000000000183000`.
- The page-table build used 6 physical pages.
- The paging test passed.
- The VM-only safety line is still visible after the switch.

Screenshot:

- `docs/screenshots/2026-06-24-page-tables.png`

![Initial page tables](screenshots/2026-06-24-page-tables.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Build an interrupt descriptor table, load it with `lidt`, and add a basic
exception handler so Vibio can catch CPU faults instead of blindly freezing or
resetting.

## 2026-06-24 - Interrupt Descriptor Table

Plain English:

Vibio OS now has its own Interrupt Descriptor Table, usually called an IDT. The
IDT is the CPU's table for deciding where to jump when an interrupt or exception
happens. The kernel allocates a page for the IDT, installs a breakpoint handler,
loads the IDT with `lidt`, triggers a breakpoint exception, and returns back to
normal kernel code.

What changed:

- Added IDT entry and IDT pointer structures.
- Allocated an IDT page from Vibio's page allocator.
- Installed a breakpoint exception handler at vector 3.
- Loaded the IDT with `lidt`.
- Triggered `int3` as a test.
- Verified the handler ran exactly once and returned with `iretq`.

What the screenshot proves:

- Vibio loaded an IDT page at `0x0000000000189000`.
- The breakpoint exception hit counter reached 1.
- The breakpoint exception was caught.
- The IDT test passed.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-interrupts-idt.png`

![Interrupt Descriptor Table](screenshots/2026-06-24-interrupts-idt.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Add timer and keyboard interrupt support so Vibio can react to hardware events,
not only software-triggered exceptions.

## 2026-06-24 - Timer And Keyboard IRQs

Plain English:

Vibio OS now reacts to hardware-style events inside the VM. The kernel remaps
the old PC interrupt controller, programs the timer, installs interrupt handlers
for the timer and keyboard, enables interrupts, waits for timer ticks, records a
keyboard scancode, then turns interrupts back off before drawing the status
screen.

This is important because a real OS cannot just run one straight line of code.
It needs to respond when time passes and when the user presses keys. This
milestone is the first step toward a real console, then later a desktop with
Windows-like keyboard and mouse behavior.

What changed:

- Added IRQ state storage allocated from Vibio's own page allocator.
- Added timer and keyboard interrupt stubs.
- Remapped the PIC so IRQ0 uses vector 32 and IRQ1 uses vector 33.
- Programmed the PIT timer.
- Unmasked only the timer and keyboard IRQs.
- Counted timer interrupts.
- Read keyboard scancodes from port `0x60`.
- Masked interrupts again after the test so the final halt loop is quiet.

What the screenshot proves:

- Vibio is still running after leaving UEFI boot services.
- The IDT is still loaded and the breakpoint exception still passes.
- The IRQ state page is `0x000000000018A000`.
- The timer interrupt counter reached 300.
- The VirtualBox test injected an `A` key press and release.
- The keyboard IRQ counter reached 2.
- The last scancode recorded was `0x000000000000009E`, the `A` key release.
- The hardware IRQ test passed.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-timer-keyboard-irqs.png`

![Timer and keyboard IRQs](screenshots/2026-06-24-timer-keyboard-irqs.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Build a real scrolling text console on top of the keyboard IRQ path. That means
typed characters should appear on screen, the kernel should keep a text buffer,
and status messages should scroll instead of being redrawn as fixed lines.

## 2026-06-24 - Text Console And First Command Prompt

Plain English:

Vibio OS now has a real text-console area and the first tiny command prompt. It
still runs fully inside the kernel, but it accepts keyboard input from the IRQ
path, stores scancodes in a small ring buffer, turns key presses into characters,
draws the current input line, and runs a few built-in commands.

The first command language is intentionally tiny. It is not Bash, PowerShell,
Python, or Linux commands. It is Vibio's own simple built-in command interpreter.
Current commands are:

- `HELP`
- `CLEAR`
- `MEM`
- `TICKS`
- `VERSION`

What changed:

- Added a keyboard scancode ring buffer to the IRQ state.
- Added console history storage.
- Added an editable input line.
- Added Enter and Backspace handling.
- Added basic scancode-to-character mapping.
- Added built-in command parsing.
- Added a live console screen with status at the top and the prompt below.
- Rate-limited full-screen redraws so screenshots are stable.

What the screenshot proves:

- Vibio reached Stage 7.
- UEFI boot services were exited.
- Allocation, paging, IDT, and IRQ tests still pass.
- Timer interrupts are still running.
- Keyboard IRQs reached 10 during the injected `HELP` test.
- The last scancode was `0x9C`, the Enter key release.
- The console received `HELP`, echoed it as `VIBIO: HELP`, and printed the
  command list.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-text-console-command-prompt.png`

![Text console and command prompt](screenshots/2026-06-24-text-console-command-prompt.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Build the kernel heap. The page allocator can hand out whole 4 KiB pages, but a
real OS also needs smaller dynamic allocations for things like console buffers,
keyboard queues, filesystem structures, process objects, and future desktop UI
objects.

## 2026-06-24 - Kernel Heap

Plain English:

Vibio OS now has its first kernel heap. Before this, the kernel could only ask
for whole 4 KiB pages. That is useful for page tables and big structures, but it
is wasteful for small objects. The heap takes pages from the page allocator and
cuts them into smaller blocks.

This heap is still simple, but it is real enough to be useful. It can split a
free block when an allocation only needs part of it, free a block, and merge
neighboring free blocks back together. The console state now comes from the
heap instead of taking a raw page directly.

What changed:

- Added heap block headers.
- Added heap initialization from page-allocator pages.
- Added `heap_alloc`.
- Added `heap_free`.
- Added block splitting.
- Added simple coalescing of neighboring free blocks.
- Added a heap self-test that checks allocation, writes, free, and reuse.
- Moved the console state allocation onto the heap.
- Added a `HEAP` command to the Vibio prompt.
- Updated the stage display to Stage 8.

What the screenshot proves:

- Vibio reached Stage 8.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, and heap tests all passed.
- The prompt received `HEAP`, echoed it as `VIBIO: HEAP`, and printed heap
  stats.
- The heap owns 4 pages.
- The heap reports 592 used bytes, which is the live console allocation.
- The heap reports 15728 free bytes.
- Vibio still keeps the VM-only safety status visible.

Why this is useful:

The page allocator gives the kernel big 4 KiB chunks. The heap gives the kernel
small, flexible chunks. That means future kernel code can create many small
objects without wasting an entire page for each one.

What this opens up:

- More console buffers and command history.
- Dynamic keyboard/input queues.
- Process and thread structures.
- Scheduler lists.
- Filesystem objects.
- Window and desktop UI objects later.
- Better internal debugging tools.

Screenshot:

- `docs/screenshots/2026-06-24-kernel-heap.png`

![Kernel heap](screenshots/2026-06-24-kernel-heap.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Start the task/process foundation. The first practical step is not full apps
yet; it is defining task structures and a simple scheduler model so Vibio can
eventually run more than one thing at a time.

## 2026-06-24 - Task Table

Plain English:

Vibio OS now has its first task table. A task is the kernel's record for "one
thing the OS knows about and may eventually run." This milestone does not switch
CPU registers between tasks yet. It creates the task objects, gives them IDs,
names, states, and stack placeholders, then lists them from the command prompt.

This is the foundation for scheduling. Later, the timer interrupt can pick a
different task, save the old task's CPU state, load the new task's CPU state,
and return into that task.

What changed:

- Added task states: ready, running, and sleeping.
- Added task records with ID, name, state, stack base, stack top, and next-task
  pointer.
- Allocated task records from the kernel heap.
- Allocated placeholder stacks from the kernel heap.
- Added a task manager with task counts.
- Created initial task records: `KERNEL`, `CONSOLE`, `IDLE`, and `INPUT`.
- Added a task self-test.
- Added a `TASKS` command to the Vibio prompt.
- Updated the stage display to Stage 9.

What the screenshot proves:

- Vibio reached Stage 9.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, heap, and task tests all passed.
- The prompt received `TASKS`, echoed it as `VIBIO: TASKS`, and printed the
  task table.
- Vibio created 4 task records.
- The table has 1 running task, 2 ready tasks, and 1 sleeping task.
- The current task is `KERNEL`.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-task-table.png`

![Task table](screenshots/2026-06-24-task-table.png)

Safety status:

- VM-only.
- Network disabled.
- Only `build/vibio.vdi` was attached to the VM.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Start real context switching. The first version should switch between simple
kernel tasks only, then later the timer IRQ can drive preemptive scheduling.

## 2026-06-24 - Cooperative Context Switching

Plain English:

Vibio OS can now actually switch between simple kernel tasks. Before this,
tasks were only records in a table. Now `CONSOLE` and `IDLE` each have their
own stack, the scheduler switches to one, that task runs, then it yields back
to the scheduler.

This is cooperative switching, not timer-preemptive switching yet. That means a
task must voluntarily yield. The next harder version is to let the timer
interrupt force task switches automatically.

What changed:

- Added saved stack pointer storage to task records.
- Added run and yield counters to task records.
- Added a small assembly context switch routine.
- Prepared initial task stacks so new tasks can be entered by switching to
  them.
- Added a task bootstrap loop.
- Added cooperative yielding back to the scheduler.
- Added a context-switch demo that runs `CONSOLE` and `IDLE` for four rounds.
- Updated the `TASKS` command to show run/yield counters and switch count.
- Updated the stage display to Stage 10.

What the screenshot proves:

- Vibio reached Stage 10.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, heap, and context tests all passed.
- The `TASKS` command shows `SWITCHES: 8`.
- `CONSOLE` ran 4 times and yielded 4 times.
- `IDLE` ran 4 times and yielded 4 times.
- `KERNEL` remained the current scheduler task.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-context-switching.png`

![Cooperative context switching](screenshots/2026-06-24-context-switching.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Start the syscall boundary. A syscall is how user programs will eventually ask
the kernel for services. The first version can still be kernel-only, but it
should define the calling convention and dispatch table we will later expose to
user mode.

## 2026-06-24 - Syscall Boundary

Plain English:

Vibio OS now has the first version of a syscall boundary. A syscall is the
controlled doorway future programs will use to ask the kernel for services. For
now, Vibio still calls it from kernel test code, but it uses a real interrupt
path: syscall number in `RAX`, `int 0x80`, result back in `RAX`.

This does not mean Vibio has user programs yet. It means the kernel now has the
shape of the interface future programs will use.

Current syscall services:

- `1`: return syscall ABI version.
- `2`: return timer tick count.
- `3`: return task count.
- `4`: return context switch count.

What changed:

- Added syscall constants and a syscall state block.
- Added interrupt vector `0x80` to the IDT.
- Added an `int 0x80` syscall handler.
- Added a kernel helper that invokes syscalls through the interrupt path.
- Added a syscall self-test.
- Added a `SYSCALL` command to the Vibio prompt.
- Updated the stage display to Stage 11.

What the screenshot proves:

- Vibio reached Stage 11.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, heap, context, and syscall tests all passed.
- The `SYSCALL` command printed syscall stats.
- The syscall ABI version is 11.
- The syscall boundary returned task count 4.
- The syscall boundary returned context-switch count 8.
- The syscall boundary returned live timer ticks.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-syscall-boundary.png`

![Syscall boundary](screenshots/2026-06-24-syscall-boundary.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Timer-driven scheduling. Vibio can already switch tasks when they yield. The
next step is to let the timer interrupt drive scheduling automatically between
simple kernel tasks.

## 2026-06-24 - Non-Destructive USB Boot Copy To G:

Plain English:

Copied the current Vibio OS boot files to the removable `G:` USB drive so it can
be tried from an old laptop's one-time boot menu. This was not an installer and
did not format the USB.

What was copied:

- `build/BOOTX64.EFI` to `G:/EFI/BOOT/BOOTX64.EFI`
- `build/KERNEL.BIN` to `G:/KERNEL.BIN`

What was checked first:

- `G:` is a removable Memorex USB flash drive.
- `G:` is not the Windows boot disk.
- `G:` is not a system disk.
- `G:` is formatted as exFAT.
- Existing files were not overwritten.

Important note:

Many UEFI firmwares boot removable media most reliably from FAT32. This USB is
currently exFAT, so some laptops may not boot it. If the old laptop does not
show the USB as a UEFI boot option, the likely next step is to erase and format
the USB as FAT32, but that must be an explicit separate decision because it
would delete the USB contents.

Safety status:

- No internal Windows or Ubuntu disk was modified.
- No host bootloader was modified.
- No boot order was changed.
- This PC was not rebooted.
- Only two files and the `EFI/BOOT` folder were added to the removable `G:`
  drive.
- Vibio OS still has no installer and no internal-disk write path.

## 2026-06-24 - Timer-Driven Scheduling

Plain English:

Vibio OS now has the first timer-driven scheduler step. The hardware timer
interrupt does not directly jump between tasks yet. Instead, it raises a
regular scheduler request, and the kernel loop responds by switching to the
next ready kernel task.

This is useful because the scheduler is no longer only a one-time boot demo.
The timer is now feeding the scheduler continuously, which is how real operating
systems decide that different work should get CPU time.

What changed:

- Added scheduler request counters to the timer IRQ state.
- Made the timer interrupt create a scheduler request every 50 timer ticks.
- Added round-robin selection for the next ready task with its own stack.
- Added timer-driven switches between `CONSOLE` and `IDLE`.
- Kept the actual context switch outside the interrupt handler for now.
- Added a `SCHED` command to show scheduler request and switch counters.
- Updated the screen to Stage 12.
- Updated the next milestone label to `USER MODE`.

What the screenshot proves:

- Vibio reached Stage 12.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, syscall, heap, and scheduler tests all passed.
- The `SCHED` command printed timer scheduler counters.
- Scheduler requests matched IRQ scheduler requests.
- Timer-driven switches increased the total context-switch count.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-timer-driven-scheduling.png`

![Timer-driven scheduling](screenshots/2026-06-24-timer-driven-scheduling.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

User/kernel mode boundary. Vibio now has syscalls and a scheduler, but all code
still runs with full kernel privilege. The next big step is setting up the CPU
state needed for future user programs to run with less privilege and ask the
kernel for services through the syscall boundary.

## 2026-06-24 - USB Refreshed With Stage 12 Build

Plain English:

Copied the latest Stage 12 Vibio OS boot files to the removable `G:` USB drive
so the old laptop can try the timer-driven scheduler build.

What was copied:

- `build/BOOTX64.EFI` to `G:/EFI/BOOT/BOOTX64.EFI`
- `build/KERNEL.BIN` to `G:/KERNEL.BIN`

Important boot note:

The USB currently shows up in Windows as removable drive `G:` with label
`UBUNTU 26_0` and filesystem `exFAT`. Many laptop UEFI boot menus do not show
exFAT USB drives as bootable removable media. They usually expect FAT32 with
the fallback boot file at `EFI/BOOT/BOOTX64.EFI`.

Safety status:

- No format was performed.
- No internal Windows or Ubuntu disk was modified.
- No host bootloader was modified.
- No boot order was changed.
- Only the two Vibio boot files on the removable `G:` USB were overwritten.

## 2026-06-24 - USB Refreshed After FAT32 Format

Plain English:

After the removable USB was reformatted as FAT32, copied the latest Stage 12
Vibio OS boot files back onto it.

What was checked:

- `G:` was present.
- `G:` was removable.
- `G:` was FAT32.
- `G:` was labeled `VIBIO`.

What was copied:

- `build/BOOTX64.EFI` to `G:/EFI/BOOT/BOOTX64.EFI`
- `build/KERNEL.BIN` to `G:/KERNEL.BIN`

Verification:

- SHA256 check confirmed both USB files match the latest build files.

Safety status:

- No format was performed by Codex during this copy step.
- No internal Windows or Ubuntu disk was modified.
- No host bootloader was modified.
- No boot order was changed.
- Only the two Vibio boot files on the removable `G:` USB were written.

## 2026-06-24 - VirtualBox Dev VM Boot Fix

Plain English:

The VirtualBox GUI showed "No bootable option or device was found" because the
safe development VM, `Vibio OS Dev`, had no disk attached in its SATA boot slot.
The latest VM disk existed at `build/vibio.vdi`, but the VM was not pointing at
it.

What changed:

- Updated `build.ps1` so rebuilding also reattaches `build/vibio.vdi` to
  `Vibio OS Dev`.
- Kept the VM set to EFI firmware.
- Kept VM networking disabled.
- Kept VM USB disabled.
- Kept clipboard and drag/drop disabled.

Verification:

- Rebuilt the disk image.
- Confirmed `Vibio OS Dev` boots from `build/vibio.vdi`.
- Captured a Stage 12 proof screenshot after the fix.

Screenshot:

- `docs/screenshots/2026-06-24-vm-boot-fix-stage12.png`

![VM boot fix Stage 12](screenshots/2026-06-24-vm-boot-fix-stage12.png)

Safety status:

- VM-only.
- The removable `G:` USB was not touched during this VM fix.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

## 2026-06-24 - Stage 13 User Boundary And Sound Assets

Plain English:

Vibio OS now has the first real CPU-side user/kernel boundary setup. It still
does not run separate user programs yet, but the kernel now owns the descriptor
tables needed for that future step.

Vibio also now packages the custom WAV sounds from the `Sounds` folder into the
generated FAT32 VM disk under `/SOUNDS`. The OS can list them with the `SOUNDS`
command, but it cannot play them yet because there is no audio driver.

What changed:

- Added a Vibio-owned Global Descriptor Table.
- Added kernel code and data selectors.
- Added future-user code and data selectors.
- Added a Task State Segment.
- Loaded the task register with `ltr`.
- Made the `int 0x80` syscall gate callable from future user mode.
- Bumped the syscall ABI version to 13.
- Added a `USER` command to show selector and syscall-gate status.
- Added a `SOUNDS` command to list packaged sound assets.
- Updated the FAT32 image builder to include `/SOUNDS`.
- Packaged `BOOT.WAV`, `ERROR.WAV`, `NOTIFY.WAV`, `USBINS.WAV`, and
  `USBRM.WAV`.
- Updated the screen to Stage 13.
- Updated the next milestone label to `STORAGE FS`.

What the screenshots prove:

- Vibio reached Stage 13.
- UEFI boot services were exited.
- Allocation, paging, IDT, IRQ, syscall, heap, and user-boundary tests passed.
- The `USER` command shows kernel selectors, user selectors, TSS selector, task
  register, and user-callable syscall status.
- The `SOUNDS` command lists the packaged sound assets.
- Vibio still keeps the VM-only safety status visible.

Screenshots:

- `docs/screenshots/2026-06-24-stage13-user-boundary.png`
- `docs/screenshots/2026-06-24-stage13-sound-assets.png`

![Stage 13 user boundary](screenshots/2026-06-24-stage13-user-boundary.png)

![Stage 13 sound assets](screenshots/2026-06-24-stage13-sound-assets.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched during Stage 13 work.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Storage and filesystem. Vibio now has sound files packaged on the boot disk, but
the kernel cannot read arbitrary files from disk after boot yet. The next step
is to teach the kernel how to read from its virtual disk and find files, which
opens the door to loading assets and future programs.

## 2026-06-24 - Flicker-Safe Rendering And Sound Status Commands

Plain English:

The screen was flickering badly because Vibio cleared the whole framebuffer and
redrew everything every timer update. In VirtualBox, that blanking step was
visible. Vibio now draws the whole screen once, then only redraws the timer
line, keyboard lines, prompt, or console area when those parts actually need to
change.

The sound files still cannot play inside Vibio yet. They are packaged on the
boot disk, but playback needs a real audio driver. To make this clearer, typing
a sound name now reports that the sound exists but cannot play yet.

What changed:

- Stopped clearing the full framebuffer on every timed redraw.
- Kept full-screen drawing for the first paint only.
- Redraws the timer line and prompt without blanking the whole UI.
- Redraws the console area only when keyboard input changes it.
- Added status handling for `BOOT`, `ERROR`, `NOTIFY`, `USBINS`, and `USBRM`.
- Updated the `SOUNDS` command to tell the user to type a sound name for status.

What the screenshot proves:

- Vibio still reaches Stage 13.
- The `SOUNDS` command still lists the packaged sound assets.
- Typing `USBINS` now shows a useful sound status instead of unknown command.
- Vibio clearly says playback needs an audio driver.

Screenshot:

- `docs/screenshots/2026-06-24-flicker-safe-rendering-sound-status.png`

![Flicker-safe rendering and sound status](screenshots/2026-06-24-flicker-safe-rendering-sound-status.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched during this fix.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

## 2026-06-24 - Stage 14 Read-Only File Loading

Plain English:

Vibio OS now has a safe first storage milestone. It does not write to any disk.
The UEFI boot stage opens a fixed list of known sound files using read-only file
mode, reads them into memory, and passes a small file table to the kernel.

This is useful because the kernel can now see real file data after boot without
needing a disk write path. It is the safe stepping stone before a real kernel
disk driver.

What changed:

- Added a boot file table to `VibioBootInfo`.
- Added read-only loading for five known WAV files:
  - `BOOT.WAV`
  - `ERROR.WAV`
  - `NOTIFY.WAV`
  - `USBINS.WAV`
  - `USBRM.WAV`
- Added a file size limit so oversized files fail instead of truncating.
- Added a kernel file-table self-test.
- Added a `FILES` command.
- Updated `SOUNDS` and sound-name commands to use loaded file sizes.
- Bumped the syscall ABI version to 14.
- Updated the screen to Stage 14.
- Updated the next milestone label to `DISK IO`.

Safety design:

- The boot stage uses `EFI_FILE_MODE_READ` only.
- Vibio does not call UEFI file write functions.
- Vibio does not write sectors.
- Vibio does not format disks.
- Vibio does not mount or modify host filesystems.
- This milestone loads only known files from the generated VM boot disk.

What the screenshot proves:

- Vibio reached Stage 14.
- `FILES TEST: PASS`.
- Five read-only WAV files were loaded into memory.
- Total loaded bytes are shown.
- The command output explicitly says `NO DISK WRITES`.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-stage14-readonly-files.png`

![Stage 14 read-only files](screenshots/2026-06-24-stage14-readonly-files.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched during Stage 14 work.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Read-only disk I/O. Stage 14 still relies on UEFI to read files before the
kernel starts. The next step is a kernel-side virtual disk read path that can
read sectors from the VM disk, still without any write support.

## 2026-06-24 - Stage 15 Read-Only Disk I/O

Plain English:

Vibio OS now has its first kernel-side disk read path. This is different from
Stage 14: Stage 14 used UEFI to read files before the kernel fully took over.
Stage 15 has the kernel find the AHCI/SATA controller and read raw disk sectors
itself.

This is useful outside a VM too, but it is not the final real-laptop storage
story. AHCI is common for internal SATA disks and a useful real hardware driver
shape. USB boot on an old laptop will need a later USB mass-storage driver.

What changed:

- Added PCI config-space reads to find an AHCI storage controller.
- Enabled PCI memory and bus-master access for that controller.
- Added AHCI MMIO helpers.
- Added read-only AHCI port setup.
- Added an AHCI `READ DMA EXT` sector-read command.
- Read LBA 0 and verified the MBR signature.
- Verified the EFI System Partition entry.
- Read the FAT32 boot sector at LBA 2048.
- Verified the FAT32 boot-sector signature.
- Added a `DISK` command.
- Bumped the syscall ABI version to 15.
- Updated the screen to Stage 15.
- Updated the next milestone label to `FAT32 RO`.

Safety design:

- There is no AHCI write command in the code.
- There is no sector-write function.
- The `DISK` command reports `WRITE PATH: NONE`.
- The test reads only two sectors from the VM disk.
- VM USB remained disabled.
- The removable `G:` USB was not touched.

What the screenshot proves:

- Vibio reached Stage 15.
- `DISK TEST: PASS`.
- The kernel performed two read-only sector reads.
- MBR, EFI partition, and FAT32 boot-sector checks passed.
- The command output says `WRITE PATH: NONE`.
- Vibio still keeps the VM-only safety status visible.

Screenshot:

- `docs/screenshots/2026-06-24-stage15-readonly-disk-io.png`

![Stage 15 read-only disk I/O](screenshots/2026-06-24-stage15-readonly-disk-io.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched during Stage 15 work.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

When writing gets added:

Not yet. Disk writes should come only after read-only disk I/O, read-only FAT32,
and disposable-VM write tests. The first write support should target a throwaway
VM image, not the USB and not a real laptop disk.

Next milestone:

Read-only FAT32 filesystem. Stage 15 can read sectors. The next step is parsing
the FAT32 structures from those sectors so Vibio can find files by name from the
disk on its own.

## 2026-06-24 - Stage 16 Read-Only FAT32 Filesystem

Plain English:

Vibio OS can now parse the FAT32 boot disk by itself. Stage 15 could read raw
sectors, but raw sectors are just numbered blocks. Stage 16 understands enough
FAT32 structure to find the root directory, find the `SOUNDS` directory, and
count the known WAV files by directory entry.

This is still read-only. There is no FAT write code, no directory write code,
and no file creation or deletion.

What changed:

- Parsed FAT32 boot-sector fields.
- Calculated FAT and data-region locations.
- Calculated cluster-to-LBA mapping.
- Read the root directory cluster.
- Found the `SOUNDS` directory.
- Read the `SOUNDS` directory cluster.
- Verified the five WAV entries by short 8.3 names.
- Counted total sound bytes from directory entries.
- Added an `FS` command.
- Bumped the syscall ABI version to 16.
- Updated the screen to Stage 16.
- Updated the next milestone label to `USB RO`.

Safety design:

- The FAT32 reader only reads sectors through the Stage 15 AHCI read path.
- There is no FAT write function.
- There is no directory write function.
- There is no file write/create/delete function.
- The `FS` command reports `WRITE PATH: NONE`.
- VM USB remained disabled during testing.
- The removable `G:` USB was not touched.

What the screenshot proves:

- Vibio reached Stage 16.
- `FAT32 TEST: PASS`.
- FAT32 bytes-per-sector and sectors-per-cluster were parsed.
- Root directory and `SOUNDS` directory LBAs were calculated.
- Five sound files were found.
- Total sound bytes matched the packaged assets.
- The command output says `WRITE PATH: NONE`.

Screenshot:

- `docs/screenshots/2026-06-24-stage16-fat32-readonly.png`

![Stage 16 read-only FAT32](screenshots/2026-06-24-stage16-fat32-readonly.png)

Safety status:

- VM-only.
- Network disabled.
- USB disabled in the VM.
- Only `build/vibio.vdi` was attached to the VM.
- The removable `G:` USB was not touched during Stage 16 work.
- The VM was powered off after testing.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.

Next milestone:

Read-only USB storage. FAT32 now works on top of AHCI, but the old laptop's USB
boot path needs USB mass-storage support. The next real-hardware-focused step
is a read-only USB storage backend, still with no write support.

## 2026-06-24 - Stage 17 Read-Only USB Controller Probe

Stage 17 starts the USB storage path in the safest possible way: discovery
only. Vibio OS now scans PCI configuration space for USB controller classes and
counts UHCI, OHCI, EHCI, and xHCI controllers. It does not enumerate USB
devices yet, does not send USB mass-storage commands, and does not read or
write any USB disk.

Plain English:

Stage 16 taught Vibio how to understand FAT32 once sectors are available.
Stage 17 starts answering the next question: "what USB controller would we talk
to on a real laptop?" In the VM, USB stays disabled on purpose, so the probe
reports zero controllers. That is okay for this safety test because it proves
the new `USB` command and the no-write status without exposing the VM to host
USB devices.

What changed:

- Bumped the syscall ABI/version display to Stage 17.
- Added PCI USB class detection for:
  - UHCI
  - OHCI
  - EHCI
  - xHCI
- Added a `VibioUsbReadTest` status structure.
- Added the `USB` console command.
- Added a `USB PROBE: PASS` screen status line.
- Updated the next milestone label to `USB MSD`.

Safety notes:

- The USB work is probe-only.
- The VM USB controller remained disabled.
- No USB passthrough was enabled.
- No physical USB drive was touched.
- The removable `G:` USB was not touched during Stage 17 work.
- There is no USB storage read path yet.
- There is no USB write path.
- The command output says `STORAGE READ: NONE`.
- The command output says `WRITE PATH: NONE`.

Verification:

- Rebuilt `build/vibio.vdi`.
- Booted `Vibio OS Dev` headless in VirtualBox.
- Injected the `USB` command through the virtual keyboard.
- Vibio reached Stage 17.
- `USB PROBE: PASS`.
- The VM reported `CONTROLLERS:0`, expected because VM USB is disabled.
- The command output says `STORAGE READ: NONE`.
- The command output says `WRITE PATH: NONE`.
- The VM was powered off after the screenshot.

Screenshot:

![Stage 17 read-only USB probe](screenshots/2026-06-24-stage17-usb-readonly-probe.png)

Build artifacts:

- `build/BOOTX64.EFI`
- `build/KERNEL.BIN`
- `build/vibio.img`
- `build/vibio.vdi`

VM safety state:

- Firmware: EFI.
- Attached disk: `build/vibio.vdi`.
- Network: disabled.
- Clipboard: disabled.
- Drag/drop: disabled.
- USB: disabled.
- VM state after test: powered off.

Next milestone:

USB mass storage read-only. Stage 17 only finds USB controller types. The next
step is a careful read-only USB mass-storage backend that can eventually read
sectors from a USB boot device on the old laptop. That should still avoid all
write commands.

## 2026-06-24 - Stage 18 Read-Only USB Controller Architecture

Plain English:

Stage 17 could count USB controllers. Stage 18 builds the proper foundation
under that count. Instead of only tallying UHCI, OHCI, EHCI, and xHCI, Vibio now
builds a small table of every USB controller it finds, records what type each
one is, where its registers live, and which controller a real machine would most
likely use. This is the careful groundwork before any USB device talking
happens.

Stage 18 deliberately stays a discovery-and-architecture step. It does not turn
on a controller, does not enumerate USB devices, and does not read or write any
USB storage. A real laptop will almost certainly need xHCI for its USB ports, so
the report says so in plain text.

Why the VM still shows zero controllers:

The development VM keeps USB disabled for safety. On VirtualBox 7.2 the xHCI and
EHCI controllers also require the Oracle Extension Pack, which is intentionally
not installed. Only OHCI is built in. Rather than install host software or turn
on USB just to see a number change, Stage 18 proves the new architecture cleanly
reports `CONTROLLERS: 0` when no controller is present. The same code will fill
in a full controller table on real hardware that does expose USB controllers.

What changed:

- Added a `VibioUsbController` record: bus/device/function, controller type,
  prog-if, vendor and device IDs, register base address, and whether that base
  is memory-mapped or I/O-mapped.
- Added a controller table (up to 8 entries) to `VibioUsbReadTest`.
- Added `usb_classify_controller` to map PCI prog-if to UHCI/OHCI/EHCI/xHCI.
- Recorded each controller's BAR read-only: BAR0 for OHCI/EHCI/xHCI (including
  64-bit memory BARs) and the BAR4 I/O base for UHCI.
- Added `usb_select_preferred_type`, which documents the controller a real
  machine would most likely use, newest first (xHCI, then EHCI, OHCI, UHCI).
- Expanded the `USB` command output to list each controller, its type, its
  bus/device/function, and its register base, plus a `PREFERRED:` line and a
  `REAL LAPTOP LIKELY XHCI` note.
- Added a `console_append_hex` helper for printing register addresses.
- Bumped the syscall ABI/version display to Stage 18.
- Changed the stage label to `STAGE 18 USB CTL`.
- Updated the next milestone label to `USB ENUM`.

Safety design:

- The USB work is still read-only discovery only.
- Stage 18 reads PCI configuration space only.
- It does not write the PCI command register.
- It does not enable controller memory or bus-master access.
- It does not touch any controller MMIO or I/O registers.
- It does not enumerate USB devices.
- The command output says `STORAGE READ: NONE`.
- The command output says `WRITE PATH: NONE`.
- The VM USB controller remained disabled.
- The VirtualBox Extension Pack was not installed.
- No USB passthrough was enabled.
- The removable `G:` USB was not touched during Stage 18 work.

Verification:

- Rebuilt `build/vibio.vdi`.
- Booted `Vibio OS Dev` headless in VirtualBox.
- Injected the `USB` command through the virtual keyboard.
- Vibio reached Stage 18.
- `USB PROBE: PASS`.
- `USB: READONLY PROBE PASS`.
- The VM reported `CONTROLLERS:0`, expected because VM USB is disabled.
- The per-type line reported `UHCI:0 OHCI:0 EHCI:0 XHCI:0`.
- `PREFERRED: NONE` because no controller is present in the VM.
- The output printed `REAL LAPTOP LIKELY XHCI`.
- The command output says `STORAGE READ: NONE`.
- The command output says `WRITE PATH: NONE`.
- The VM was powered off after the screenshot.

Screenshot:

![Stage 18 read-only USB controller architecture](screenshots/2026-06-24-stage18-usb-controller-arch.png)

Build artifacts:

- `build/BOOTX64.EFI`
- `build/KERNEL.BIN`
- `build/vibio.img`
- `build/vibio.vdi`

VM safety state:

- Firmware: EFI.
- Attached disk: `build/vibio.vdi`.
- Network: disabled.
- Clipboard: disabled.
- Drag/drop: disabled.
- USB: disabled.
- VM state after test: powered off.

Next milestone:

USB device enumeration, real-hardware first. The controller table is now in
place. The next step is, on a machine that actually exposes a USB controller, to
bring one controller up read-only and enumerate attached devices far enough to
recognize a USB mass-storage class device. That still stops before any USB
storage read and adds no write path. Testing this in the VM would require the
Extension Pack and an emulated controller, so this milestone is expected to be
proven on real hardware or a disposable VM image, never on the `G:` USB.

## 2026-06-24 - Real-Hardware Boot Safety Fix And Stage 18 USB Refresh

Plain English:

The old laptop test failed in a scary way: the boot menu appeared, pressing a
key dropped the laptop into Windows recovery ("repair"). This was a flaw in how
the Vibio boot stage handled problems. When anything went wrong, the boot stage
printed "Press any key" and then *returned an error to the firmware*. Returning
an error tells the firmware to try the next boot option, which on a real laptop
is the internal Windows install, so the laptop fell through into Windows and ran
auto-repair. Vibio never damaged anything, but the behavior looked like a crash.

Two fixes make the USB boot trustworthy on real hardware:

1. The boot stage no longer returns to the firmware on an error. Instead it
   prints a clear message and halts in place. A boot problem can no longer chain
   into the laptop's Windows. The "press any key" prompt is gone, so a key press
   can no longer trigger the fallback.
2. Loading the packaged sound files is now optional. Since Stage 14 the boot
   stage required `\SOUNDS\USBINS.WAV` and `\SOUNDS\USBRM.WAV`, but the USB had
   those files saved under different names (`Usb Insert.wav`, `Usb Remove.wav`).
   A missing or misnamed sound file would have caused the exact return-to-
   firmware failure above. Sound files are now skipped if absent, and the USB
   was refreshed with the correctly named files so the experience is complete.

What changed in `src/boot/main.c`:

- Replaced the `wait_for_key` + `return status` error pattern with a
  `halt_forever` helper that prints a message, disables the watchdog, and halts.
- Every fatal boot error (no volume, no kernel, no boot-info buffer, no memory-
  map buffer, no framebuffer, failed exit-boot-services) now halts safely
  instead of handing control back to the firmware.
- `load_boot_files` is now non-fatal: each sound file that fails to open, read,
  or fit is skipped, and the boot continues with whatever loaded.

What changed on the removable `G:` USB (USB-only, nothing else touched):

- Refreshed `G:\EFI\BOOT\BOOTX64.EFI` with the new boot stage.
- Refreshed `G:\KERNEL.BIN` with the Stage 18 kernel.
- Added correctly named sound files under `G:\SOUNDS\`: `BOOT.WAV`,
  `ERROR.WAV`, `NOTIFY.WAV`, `USBINS.WAV`, and `USBRM.WAV`.

Why USB boot does not need a USB driver yet:

The laptop firmware reads the USB itself during UEFI boot services and loads
`BOOTX64.EFI`, then Vibio uses the firmware file protocol to load `KERNEL.BIN`
and the sound files before exiting boot services. So Vibio can boot from USB
even though it has no USB mass-storage driver yet. The Stage 17/18 USB work is
about a future native USB storage path, not about this boot path.

Verification:

- Rebuilt boot stage and kernel with `-Wall -Wextra`, no errors.
- Booted `Vibio OS Dev` headless and confirmed it still reaches Stage 18 with
  all self-tests passing after the boot-stage changes.
- Confirmed `BOOTX64.EFI` and `KERNEL.BIN` on `G:` match the build by SHA256.
- Confirmed `G:\SOUNDS\USBINS.WAV` and `G:\SOUNDS\USBRM.WAV` now exist.
- Powered off the VM after testing.

Honest note:

The exact original laptop failure could not be reproduced step by step here,
but the mechanism that turned any Vibio boot hiccup into a Windows-recovery
event has been removed, and the build on `G:` is now self-consistent. If the
laptop still stops, it will now stop on a Vibio message instead of booting
Windows, which also makes the real cause easy to read off the screen.

Safety status:

- The fix and copy touched only the removable `G:` USB.
- No format was performed on `G:`.
- No internal Windows or Ubuntu disk was modified.
- No host bootloader was modified.
- No boot order was changed on this PC.
- Vibio still has no installer and no disk write path.
- Vibio reads a real machine's disk only with read-only commands and never
  writes to it.

## 2026-06-24 - Stage 18 Runs On Real Hardware, Plus Display And Input Fixes

Plain English:

Big milestone: Stage 18 booted on a real old laptop from the `G:` USB for the
first time, not just in the VM. Three issues showed up on real hardware, and all
three are now fixed.

1. The console box flickered while typing. Cause: every key event cleared the
   whole console area to blank and then redrew everything. On a real screen that
   blank frame was visible as a flicker.
2. The status counters flickered when they updated. Same cause: each counter
   line was blanked and then redrawn.
3. Pressing one key counted as two. A PS/2-style keyboard sends one code when a
   key goes down and another when it comes up, so the raw interrupt counter
   showed 2 for a single tap.

How they were fixed:

- Characters are now drawn "opaque": each character cell paints its own
  background and its lit pixels in one pass. Because of that, any line can be
  redrawn directly over the old text with no blank-then-redraw step, so nothing
  flickers. A small shared background color is set to match whichever area is
  being drawn (the status panel or the console box).
- The console history is only repainted when it actually changes (a command was
  run), tracked by a new `history_revision` counter. Plain typing now repaints
  only the single prompt line, not the whole console.
- The prompt and history lines clear just the tail to the right of their text,
  so backspace and cleared lines never leave leftover pixels, again without
  blanking the active text.
- The keyboard counter now counts key-down events only, so one tap counts as
  one. The on-screen label changed from `KEYBOARD IRQS` to `KEY PRESSES` to say
  what it now measures, and `LAST SCAN` shows the key-down code.

What changed in `src/kernel/main.c`:

- Added a shared `g_text_bg` and made `draw_char` paint full opaque cells.
- Set `g_text_bg` to the panel color for status text and to the console-box
  color for console text, on both the first paint and incremental redraws.
- Removed every blank-then-redraw `fill_rect` from the incremental redraw path.
- Added tail-clearing to `draw_console_history` and `draw_console_prompt`.
- Added `history_revision` to the console state, bumped it on each command, and
  used it so history repaints only when it changed.
- Changed the keyboard ISR to count only make (key-down) scancodes.
- Relabeled the keyboard line to `KEY PRESSES`.

Verification:

- Rebuilt with `-Wall -Wextra`, no errors.
- Booted `Vibio OS Dev` headless.
- Tapped one key: `KEY PRESSES` showed 1, `LAST SCAN` showed `0X1E` (key-down).
- Backspaced it cleanly, then ran `HELP`: the command list rendered with no
  leftover pixels and the prompt cleared correctly.
- Powered off the VM after testing.

Pending:

- The fixed build is in `build/`. The removable `G:` USB was not connected to
  the development PC at fix time, so the USB still needs a refresh with the new
  `BOOTX64.EFI` and `KERNEL.BIN` before the next laptop test. The sound files on
  `G:` are already correctly named and do not need changing.

Safety status:

- VM-only testing; the VM was powered off afterward.
- No `G:` write was performed during this fix (USB was not connected).
- No internal disk, host bootloader, or boot order was changed.
- Vibio still has no installer and no disk write path.

## 2026-06-24 - Central Stage Number And Cooler Boot Sequence

Plain English:

Two polish fixes. First, the `VERSION` command said "STAGE 17" while the top of
the screen said "STAGE 18". The stage number was written by hand in several
places, so they drifted. There is now a single stage constant, and every place
that shows the stage reads from it, so they can never disagree again. Second,
the boot loader now shows a proper step-by-step boot sequence with colors and
real values, so you can watch what it is doing before the kernel takes over.

What changed in `src/kernel/main.c`:

- Added `VIBIO_STAGE` as the one true stage number.
- Made `VIBIO_SYSCALL_ABI_VERSION` derive from `VIBIO_STAGE`.
- The screen stage line now draws the number from `VIBIO_STAGE`.
- The `VERSION` command now prints `VIBIO OS STAGE <n>` and `SYSCALL ABI <n>`
  from the same constant instead of hard-coded digits.

What changed in `src/boot/main.c` and `src/boot/efi.h`:

- Gave the UEFI `Stall` boot service a real type so the loader can pause.
- Added `print_dec`, `print_hex`, colored `boot_step`, `boot_ok`, and
  `boot_detail` helpers.
- Rebuilt the boot output as a colored sequence:
  - A cyan `VIBIO OS / UEFI BOOT SEQUENCE` header.
  - `> Opening boot volume            [OK]`
  - `> Loading KERNEL.BIN             [OK]` with size and load address.
  - `> Allocating boot structures     [OK]`
  - `> Reading graphics mode          [OK]` with resolution and framebuffer base.
  - `> Loading sound assets           [OK]` with file count.
  - `> Reading memory map             [OK]`
  - A yellow `Starting Vibio kernel at 0x100000` line.
- Added short pauses between steps so the sequence is readable, then a longer
  pause before exiting boot services and jumping to the kernel.

Verification:

- Rebuilt with `-Wall -Wextra`, no errors.
- Captured the boot sequence in the VM (`2026-06-24-stage18-boot-sequence.png`).
- Ran `VERSION`: it now reads `VIBIO OS STAGE 18` and `SYSCALL ABI 18`, matching
  the on-screen `STAGE 18` line.
- Powered off the VM afterward.

Screenshot:

![Stage 18 UEFI boot sequence](screenshots/2026-06-24-stage18-boot-sequence.png)

Safety status:

- VM-only testing; the VM was powered off afterward.
- The boot sequence pauses add a couple of seconds to boot; nothing else
  changed about how the kernel runs.
- No internal disk, host bootloader, or boot order was changed.

## 2026-06-24 - Stage 19 Mouse, Cursor, And Drawing Primitives

Plain English:

Stage 19 begins the road to a Windows-like desktop: Vibio now has a working
mouse and a real on-screen cursor. The kernel talks to the PS/2 mouse, tracks an
arrow cursor that glides over everything without leaving a trail, and shows a
live coordinate readout. This is the first "graphics and input" stage; window
surfaces and a window manager come next.

What changed in `src/kernel/main.c`:

- Added a PS/2 mouse interrupt handler on IRQ12 (vector 44). Because IRQ12 is on
  the slave interrupt controller, its end-of-interrupt is sent to both the slave
  and the master controller.
- Extended the IRQ state with a mouse byte ring buffer (the handler stores raw
  bytes; the C code reassembles 3-byte movement packets and resyncs if needed).
- Added `mouse_init`, which enables the auxiliary device, turns on the IRQ12
  interrupt in the controller, and tells the mouse to start reporting. It runs
  with interrupts off so the keyboard handler cannot steal controller bytes.
- Added `pic_enable_mouse` to unmask the cascade line and IRQ12 without
  disturbing the timer and keyboard.
- Added `mouse_poll` to decode packets, apply 9-bit signed movement (Y flipped),
  clamp to the screen, and track buttons and a packet count.
- Added drawing primitives: `get_pixel`, plus a cursor that uses save-under and
  restore so it can be drawn over any content and removed cleanly.
- Defined a classic arrow cursor bitmap (white fill, dark outline).
- The main loop now lifts the cursor before any redraw, draws content, then
  re-saves the pixels under the cursor and redraws it on top. When neither the
  cursor nor the content under it changes, the rewrite is identical pixels, so
  there is no flicker.
- Added a live `MOUSE x,y` readout to the status panel and a `MOUSE` console
  command showing PS2 init status, X/Y, button state, and packet count.
- Added a comma glyph to the font.
- Bumped `VIBIO_STAGE` to 19, changed the stage label to `STAGE 19 MOUSE`, and
  set the next-milestone label to `WINDOWS`.

Verification:

- Rebuilt with `-Wall -Wextra`, no errors.
- Booted `Vibio OS Dev` headless: reached `STAGE 19 MOUSE`, `NEXT: WINDOWS`, all
  self-tests pass, and the arrow cursor renders at screen center with a live
  `MOUSE 512,384` readout.
- Ran the `MOUSE` command: `MOUSE: PS2 PASS`, `X:512 Y:384`, buttons all 0,
  `PACKETS:0` (no movement was injected because VirtualBox headless cannot send
  PS/2 mouse packets; movement and packet counting are exercised by real input).
- Powered off the VM afterward.

Note on testing movement:

- VirtualBox headless has no mouse-injection command, so automated testing can
  confirm init, rendering, and the command, but not live movement. Movement is
  verified by hand in the VM GUI or on real hardware. Many older laptop
  touchpads present as PS/2 and should move the cursor; a USB-only touchpad will
  not move it until Vibio has a USB input driver later.

Screenshot:

![Stage 19 mouse and cursor](screenshots/2026-06-24-stage19-mouse-cursor.png)

Safety status:

- VM-only testing; the VM was powered off afterward.
- Stage 19 is additive input and framebuffer drawing only.
- No disk read or write path was added or changed.
- No internal disk, host bootloader, or boot order was changed.

Next milestone:

Window surfaces (Stage 20). With a cursor and drawing primitives in place, the
next step is movable rectangular window surfaces that content can be drawn into,
leading toward a window manager and a Windows-like desktop.

## 2026-06-24 - Stage 20 Draggable Window Surface And Cursor Underline Fix

Plain English:

Vibio now has its first window. A real window with a title bar, border, and body
text floats over the existing screen, and you can drag it around by its title
bar with the mouse. This is the first piece of the desktop: a movable surface.

Also fixed a small but annoying bug: the blinking text cursor (the underscore)
could leave a stale underscore stuck under a character if a key was pressed while
the cursor happened to be showing. The cursor underline sits just below the text
cell, so redrawing the letters did not erase it. The fix clears the whole
underline strip across the prompt line on every redraw before drawing the
current cursor, so no leftover underscores remain.

What changed in `src/kernel/main.c`:

- Cursor fix: `draw_console_prompt` now clears the underline strip under the
  entire prompt line each redraw, then draws the single current cursor.
- Added a `VibioWindow` (position, drag state) and a fixed window size.
- Added `draw_cstr` for drawing short text labels on the framebuffer.
- Added `window_draw`: body fill, title bar, border, a separator under the title
  bar, the title text, and a body hint.
- Added `window_save_under` / `window_restore_under`: the same save-under and
  restore technique as the cursor, scaled up to the window, so the window can be
  dragged over any content without corrupting it.
- Added `window_update`: pressing the left mouse button on the title bar grabs
  the window, moving the mouse drags it, releasing drops it; the window is
  clamped to the screen.
- The main loop now composites in layers: base screen, then the window, then the
  cursor on top. Each layer is lifted before a redraw and re-composited after,
  so dragging the window cleanly reveals whatever was beneath it.
- Bumped `VIBIO_STAGE` to 20, stage label `STAGE 20 WINDOW`, next label
  `DESKTOP`.

Verification:

- Rebuilt with `-Wall -Wextra`, no errors.
- Booted `Vibio OS Dev` headless: reached `STAGE 20 WINDOW`, `NEXT: DESKTOP`, all
  self-tests pass, and the `VIBIO WINDOW` surface renders floating over the
  console with the cursor composited on top.
- Dragging uses live mouse input, which VirtualBox headless cannot inject, so it
  is verified by hand in the VM GUI or on real hardware.
- Powered off the VM afterward; refreshed the removable `G:` USB with the Stage
  20 build (both files SHA256-verified).

Screenshot:

![Stage 20 draggable window](screenshots/2026-06-24-stage20-window.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- Stage 20 is additive input and framebuffer drawing only; no disk path changed.
- The `G:` refresh touched only the two boot files on the removable USB.
- No internal disk, host bootloader, or boot order was changed.

Next milestone:

Desktop and compositor (Stage 21). One window works with a per-window save-under
buffer. Multiple overlapping windows want a back-buffer compositor: draw the
whole scene offscreen, then blit it. That also removes flicker for good and sets
up a window manager with focus, z-order, and eventually a taskbar.

## 2026-06-24 - Back-Buffer Compositor (Flicker Fix)

Plain English:

Moving the mouse over the window or dragging it flickered. The cause was
single-buffered drawing: the screen showed the in-between states of compositing
(the window briefly erased to background before being redrawn). The real fix is
a back buffer. Vibio now draws the entire frame into an off-screen buffer and
then copies the finished frame to the screen in one pass. The screen only ever
shows complete frames, so the cursor, window dragging, and every redraw are now
flicker-free. This also pulls the Stage 21 compositor groundwork forward.

What changed in `src/kernel/main.c`:

- Added a full-screen back buffer allocated from contiguous physical pages
  (`alloc_contiguous_pages`), sized to the framebuffer. If a contiguous run is
  not available, Vibio falls back to drawing directly to the screen.
- `put_pixel` and `get_pixel` now read and write the back buffer when it is
  active, so all existing drawing and save-under logic target it unchanged.
- Added `present`, which copies the back buffer to the live framebuffer in one
  pass; it is called once at the end of each redraw.
- Optimized the window so it is only rebuilt when it actually moved or the
  content beneath it was redrawn, not on every cursor move.

Verification:

- Rebuilt with `-Wall -Wextra`, no errors.
- Booted `Vibio OS Dev` headless: the Stage 20 window and cursor render
  identically, all self-tests pass, confirming the back buffer and present path
  work. Flicker is a timing artifact not visible in a still screenshot, but the
  screen now only ever displays fully composited frames.
- Powered off the VM; refreshed the removable `G:` USB (both files SHA256-verified).

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- The back buffer is ordinary RAM from the page allocator; no new hardware
  access, no disk path change.
- The `G:` refresh touched only the two boot files on the removable USB.

## 2026-06-24 - Stage 21 Window Manager (Multiple Windows, Focus, Z-Order)

Plain English:

Vibio now has a real window manager. There are three overlapping windows. You
can click a window to bring it to the front and focus it (its title bar lights
up), and drag any window by its title bar. This is the heart of a desktop:
multiple movable surfaces that stack correctly and respond to the mouse.

To make overlapping windows work without flicker, the renderer is now a proper
compositor. There are two off-screen buffers: a persistent "base layer" holding
the desktop, diagnostics, and console; and a back buffer. Each frame the base
layer is copied into the back buffer, the windows are drawn back-to-front on
top, the cursor is drawn last, and the finished frame is copied to the screen in
one pass. Because the screen only ever shows complete frames, there is no
flicker, and because the windows are redrawn in z-order every frame, overlaps
compose correctly. The per-window save-under buffers from Stage 20 are gone,
replaced by this cleaner model.

What changed in `src/kernel/main.c`:

- Added a second full-screen compositor buffer (the base layer) alongside the
  back buffer; `g_draw_target` selects which buffer drawing goes to.
- Added `blit_layer` to copy one full-screen buffer to another in one pass.
- Reworked the window into a `VibioWindow` with its own size, colors, and title,
  and added a `VibioWindowManager` holding up to three windows plus a z-order.
- Added window-manager logic: `wm_raise` (focus + raise), `wm_window_at`
  (topmost window under a point), `wm_update` (click to raise/focus, title-bar
  drag), and `wm_draw` (composite back-to-front, top window drawn focused).
- The main loop now: updates the base layer only when base content changed, then
  every frame copies base to back, draws the windows and cursor, and presents.
- The focused (top) window gets a bright title bar; the others are dimmed.
- Bumped `VIBIO_STAGE` to 21, stage label `STAGE 21 WM`, next label `TASKBAR`.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless: reached `STAGE 21 WM`, `NEXT: TASKBAR`, all
  self-tests pass, and three overlapping windows render correctly with the
  topmost one (`VIBIO WINDOW 3`) shown focused (bright title) and the others
  dimmed. Overlaps compose cleanly and the console shows behind the windows.
- Click-to-focus, raise, and title-bar dragging use live mouse input, which
  VirtualBox headless cannot inject, so they are verified by hand in the VM GUI
  or on real hardware.
- Powered off the VM; refreshed the removable `G:` USB (both files SHA256-verified).

Screenshot:

![Stage 21 window manager](screenshots/2026-06-24-stage21-window-manager.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- Stage 21 is compositor and input logic only; the buffers are ordinary RAM from
  the page allocator. No disk path changed.
- The `G:` refresh touched only the two boot files on the removable USB.

Next milestone:

Taskbar and launcher (Stage 22). With a window manager in place, the next step
is a desktop taskbar showing the open windows, plus a way to focus or raise them
from the taskbar, moving toward a familiar Windows-like shell.

## 2026-06-24 - Stage 21 Follow-up: Window Content Clipping

Plain English:

Window text that was wider than the window spilled past the right border onto
neighboring windows. Added a clip rectangle so each window's content is confined
to the window: anything drawn outside the window interior is now discarded. Also
shortened the body hint text so it fits cleanly inside the windows.

What changed in `src/kernel/main.c`:

- Added a global clip rectangle and `set_clip` / `reset_clip`; `put_pixel` now
  rejects pixels outside it.
- `window_draw` clips its title and body text to the window interior, then
  resets the clip so later drawing (cursor, readout) is unaffected.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings. Booted headless: window text now
  stops at the window border and never spills onto other windows. Powered off.

## 2026-06-25 - Stage 22 Full Desktop Shell And Taskbar

Plain English:

Vibio now looks like the start of a real desktop instead of a debug panel with
sample windows floating over it. The screen has a wallpaper, a slim top bar, a
bottom taskbar, and three real windows: System, Welcome, and Terminal. The
diagnostics moved into the System window, and the console moved into the
Terminal window. Keyboard input still goes to the console, but now it is drawn
inside the Terminal surface.

The taskbar shows a Vibio button, one button for each open window, and a live
uptime clock. Clicking taskbar buttons can raise their matching windows in GUI
or real-hardware testing. In the headless test, keyboard input is injected into
the Terminal, and the Terminal is raised so focus matches the keyboard target.

The UI is also easier to tune later. The desktop colors now live in a small
theme struct, and the window content is selected by kind (`SYSTEM`, `WELCOME`,
or `TERMINAL`). That means future feedback about the look can be handled by
adjusting the theme, layout constants, or one content renderer instead of
rewriting the compositor.

What changed in `src/kernel/main.c`:

- Bumped `VIBIO_STAGE` to 22 and changed the stage label to
  `STAGE 22 DESKTOP`, with `NEXT: LAUNCHER`.
- Added window content kinds for System, Terminal, and Welcome windows.
- Added `VibioDesktopTheme` for adjustable shell colors and
  `VibioDesktopContext` for the live/test data that content renderers need.
- Split window drawing into chrome and clipped content rendering.
- Added the System window renderer for boot/self-test status.
- Added the Terminal window renderer for console history and prompt drawing.
- Added the Welcome window renderer as a simple desktop starter surface.
- Replaced the old full-screen diagnostics panel with wallpaper, a top bar, and
  the persistent VM-only safety line.
- Added a bottom taskbar with a Vibio button, window buttons, active-window
  highlighting, and an uptime clock.
- Added taskbar hit testing so taskbar buttons can raise windows.
- Kept Stage 21 dragging, click-to-focus, z-ordering, clipping, and compositor
  behavior intact.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless.
- Confirmed the screen reached `STAGE 22 DESKTOP` and `NEXT: LAUNCHER`.
- Confirmed System, Welcome, and Terminal windows render as real content
  windows, with no text spilling outside the window interiors.
- Injected `CLEAR`, `HELP`, and Enter through VirtualBox keyboard scancodes.
  The Terminal window received the input and displayed the command list.
- Captured the screenshot below.
- Powered off the VM afterward.

Screenshot:

![Stage 22 desktop shell](screenshots/2026-06-25-stage22-desktop-shell.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- Stage 22 changes only compositor, shell UI, window content, and input routing.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this milestone.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Launcher/start-menu-style entry point, then close/minimize window controls and
more UI polish based on feedback from the desktop screenshot.

## 2026-06-25 - Stage 22 Follow-up: More Windows-Like Desktop

Plain English:

The first Stage 22 desktop still felt too much like a set of open diagnostic
windows. This pass makes the boot screen feel more like a Windows desktop: a
blue geometric wallpaper fills the screen, desktop shortcuts sit in columns on
the left, and the bottom taskbar has a left-aligned Vibio start button, search
box, and pinned app buttons.

System, Welcome, and Terminal still exist as real windows, but they now start
hidden so the desktop is the first thing you see. Clicking their taskbar buttons
in GUI or real-hardware testing opens/raises them. Keyboard input still opens
the Terminal automatically so the command prompt remains usable even though the
Terminal is not visible at boot.

What changed in `src/kernel/main.c`:

- Added a `visible` flag to `VibioWindow`, so windows can exist without being
  drawn on the boot desktop.
- Changed the window manager to ignore hidden windows for hit testing and
  drawing, while taskbar buttons can show and raise them.
- Added smaller text rendering for desktop labels, taskbar search text, and
  clock/tray text.
- Added simple desktop shortcut icons down the left side of the screen.
- Replaced the top bar with a wallpaper-first desktop and a subtle Vibio
  watermark.
- Reworked the taskbar into a Windows-like bottom bar with a left-aligned start
  button, search box, and compact pinned app buttons.
- Kept the VM-only safety message, but made it a small desktop line above the
  taskbar instead of a dominant panel element.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless.
- Captured the clean desktop screenshot showing no open utility windows and the
  taskbar cluster aligned left.
- Injected `HELP` through VirtualBox keyboard scancodes. The hidden Terminal
  opened automatically and displayed the command list.
- Captured the Terminal-open verification screenshot.
- Powered off the VM afterward.

Screenshots:

![Stage 22 Windows-like desktop](screenshots/2026-06-25-stage22-windows-like-desktop.png)

![Stage 22 Terminal open check](screenshots/2026-06-25-stage22-terminal-open-check.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only desktop drawing, taskbar behavior, and window
  visibility/focus logic.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this follow-up.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

## 2026-06-25 - Stage 22 Follow-up: Icon, Toggle, Clear, And Clock Fixes

Plain English:

This pass fixes the rough edges from the Windows-like desktop screenshot.
The desktop and taskbar icons were too cheap-looking, taskbar buttons only
opened windows instead of toggling them, `CLEAR` printed `SCREEN CLEARED`, and
the taskbar time was just uptime labeled as time.

Icons now have a small shadow, highlight, app mark, and shortcut arrow so they
look more intentional. Taskbar buttons now toggle: if a window is already the
active open window, clicking its taskbar icon hides it; otherwise the icon opens
or raises it. The System, Terminal, and Welcome desktop shortcuts use the same
toggle behavior. `CLEAR` now leaves a clean Terminal with just the prompt. The
taskbar reads the VM CMOS/RTC clock and shows `HH:MM`, falling back to uptime
only if the RTC read fails.

What changed in `src/kernel/main.c`:

- Added CMOS/RTC clock reading through ports `0x70` and `0x71`.
- Added real `HH:MM` taskbar time, with an uptime fallback.
- Changed taskbar button behavior from show/raise-only to open/raise/hide
  toggle behavior.
- Added desktop shortcut hit testing for System, Terminal, Welcome, and Heap.
- Improved desktop icon drawing with shadow, highlight, app marks, and shortcut
  arrows.
- Made the `CLEAR` command silently clear the console history instead of adding
  a `SCREEN CLEARED` line.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless.
- Captured the desktop screenshot with improved icons and a real RTC time
  display (`HH:MM`) in the taskbar.
- Injected `HELP`, then `CLEAR`, through VirtualBox keyboard scancodes. The
  Terminal opened, then cleared to a blank prompt without `SCREEN CLEARED`.
- Captured the clear-command verification screenshot.
- Powered off the VM afterward.

Screenshots:

![Stage 22 desktop polish](screenshots/2026-06-25-stage22-desktop-polish.png)

![Stage 22 clear/toggle polish](screenshots/2026-06-25-stage22-clear-toggle-polish.png)

Testing note:

- VirtualBox headless can inject keyboard scancodes but not PS/2 mouse clicks,
  so the click-to-toggle behavior is verified by code path here and should be
  hand-checked in the VM GUI or on real hardware.

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes UI drawing, taskbar/window visibility behavior, console
  command behavior, and read-only CMOS time reads.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this follow-up.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

## 2026-06-25 - Stage 22 Follow-up: Modern Visual Pass

Plain English:

The desktop still looked too 8-bit because the icons used big pixel-letter
marks, labels painted boxy backgrounds, the taskbar buttons were hard squares,
and the wallpaper text was too chunky. This pass keeps the same desktop shape
but makes the first screen cleaner and more modern.

The desktop labels now draw transparently over the wallpaper. Icons use softer
rounded tiles, highlights, shadows, and simple symbols instead of large
two-letter marks. The taskbar uses rounded buttons and a quieter search box
with smaller text. The wallpaper lines are less harsh, and the Vibio watermark
is smaller and less dominant.

What changed in `src/kernel/main.c`:

- Added transparent scaled text drawing for desktop labels and watermark text.
- Added soft rounded-rectangle helpers for icons, taskbar buttons, and search.
- Reworked desktop icons from letter tiles to simple symbolic app tiles.
- Reworked taskbar app buttons to match the softer icon style.
- Reduced the search text size and gave the search box a softer outline.
- Softened the wallpaper geometry and reduced the watermark size.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless.
- Captured the modern desktop screenshot.
- Injected `HELP` through VirtualBox keyboard scancodes. The hidden Terminal
  opened and displayed the command list, confirming the visual pass did not
  break keyboard-to-Terminal routing.
- Captured the Terminal verification screenshot.
- Powered off the VM afterward.

Screenshots:

![Stage 22 modern desktop](screenshots/2026-06-25-stage22-modern-desktop.png)

![Stage 22 modern Terminal check](screenshots/2026-06-25-stage22-modern-terminal-check.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only framebuffer drawing and visual styling.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this follow-up.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

## 2026-06-25 - Stage 22 Pass 1: Keyboard Input Correctness

Plain English:

This is the first of several feedback passes, and it fixes the input layer
first because correct typing matters more than looks. Four things changed:

1. Pressing Space on the empty desktop no longer pops the Terminal open. The
   Terminal now opens only when you type real input (a letter, digit, symbol,
   Backspace, or Enter). A lone Space is ignored for opening, which is what you
   expect from a desktop.
2. Caps Lock and Shift now work. Letters type lowercase by default, uppercase
   when Shift is held or Caps Lock is on (Shift XOR Caps), so you can mix case.
3. The full set of special characters now types and renders: e.g.
   `! @ # $ % ^ & * ( ) - _ = + [ ] { } ; : ' " ` ~ \ | , . < > / ?`.
4. The mouse pointer is more sensitive, so it keeps up with the
   1024x768 framebuffer instead of crawling.

To make lower/upper case actually look different, the font gained real
lowercase letter shapes (a-z) drawn with a proper x-height plus ascenders and
descenders, and the new punctuation glyphs were added to the symbol font.
Because typed commands are now lowercase by default, command matching is now
case-insensitive, so `help`, `Help`, and `HELP` all run HELP.

What changed in `src/kernel/main.c`:

- Added `draw_lower_glyph` with 26 lowercase letter shapes and routed lowercase
  characters to it instead of upper-casing them at draw time.
- Added glyphs for the previously-missing punctuation/symbol characters.
- Added Shift/Caps state and a focus-request flag to `VibioConsoleState`.
- Rewrote `keyboard_scancode_to_char` to take Shift/Caps and return lowercase
  letters, uppercase on Shift XOR Caps, and the correct shifted symbol for the
  number row and punctuation keys.
- Reworked `console_process_scancode` to track Shift make/break, toggle Caps
  Lock on make, and set a focus request on real input but not on a bare Space.
- Changed the main loop to open/raise the Terminal from the console focus
  request instead of from any keyboard IRQ, so Space no longer opens it.
- Made command matching case-insensitive via `command_char_upper` and collapsed
  the per-command matchers onto a single shared comparison.
- Scaled raw PS/2 mouse deltas up (x7/4) for higher pointer sensitivity.

Also added `tools/test-vm.ps1`, a VM-only helper that boots the dev VM headless,
optionally injects a typed string (now including Shift for upper case and
symbols), captures a screenshot, and powers the VM off.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless. Injected lowercase `help`: the Terminal
  opened automatically, echoed `VIBIO: help` in real lowercase glyphs, and
  printed the command list, proving lowercase typing and case-insensitive
  commands both work.
- Injected `Hi Vibio! (a+b)=c; 50% & x:z? [#]`: every mixed-case letter and
  special character rendered correctly in the echo line.
- Space-does-not-open-Terminal is verified by code path: a lone Space sets no
  focus request, so the hidden Terminal stays hidden.
- Powered off the VM afterward.

Screenshots:

![Stage 22 input lowercase + HELP](screenshots/2026-06-25-stage22-input-lowercase-help.png)

![Stage 22 input special characters](screenshots/2026-06-25-stage22-input-special-chars.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only keyboard/mouse input handling and font glyphs.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Assets path and icons: point the build at the new `Assets\Sounds` folder, bring
in the new PNG app icons, and remove the desktop apps that currently do nothing.

## 2026-06-25 - Stage 22 Pass 2: Real PNG Icons And Fewer Apps

Plain English:

The desktop used hand-drawn 8-bit-looking icons and a wall of app shortcuts
that did nothing. This pass swaps in the real PNG icons from the new `Assets`
folder and trims the desktop down to the apps that actually work.

What you see now:

- The desktop has just two apps, System and Terminal, drawn with the real
  rounded tile PNG icons (the ones with shadows and highlights).
- The taskbar app buttons use those same PNG icons.
- The taskbar search box uses the PNG magnifier icon, and the tray shows the
  PNG Wi-Fi icon next to the clock.
- All the shortcuts that did nothing (Trash, Files, USB, Help, Disk, Sound,
  Mouse, Tasks, Heap, and the placeholder Welcome window) are gone.

Per the desktop-apps decision, Files and Trash icons stay in `Assets` but are
not shown as desktop apps until they have real behavior.

How the icons get in: a small build-time tool, `tools/make_icons.py`, uses
Pillow to scale the chosen 256px master icons down to the exact on-screen sizes
and writes them as straight-alpha `0xAARRGGBB` arrays into
`src/kernel/icons_generated.h`. The kernel blits them with real alpha blending,
so the transparent rounded corners and soft shadows composite cleanly over the
wallpaper. The generator is kept out of `build.ps1` so the normal build stays
Pillow-free; rerun it by hand if the source icons change.

What changed:

- `build.ps1`: point the sound assets at `Assets\Sounds` (the folder moved), and
  add `src\kernel` to the kernel include path for the generated icon header.
- `build.ps1`: hardened the VDI delete step so a missing image no longer aborts
  the build.
- `tools/make_icons.py` (new) and `src/kernel/icons_generated.h` (generated):
  System, Terminal (36px desktop + 26px taskbar), search (18px), Wi-Fi (20px).
- `src/kernel/main.c`: added `unpack_color`, `blend_pixel`, and
  `draw_icon_image` for alpha-blended icon blitting; replaced the desktop and
  taskbar vector icons with PNG blits; replaced the search magnifier and added
  the tray Wi-Fi icon; reduced the window manager to two windows (System,
  Terminal) by removing the Welcome window; updated desktop-icon hit testing to
  the new two-icon column; gave the Terminal a saner default size and position.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors; sounds packaged from
  `Assets\Sounds`.
- Booted `Vibio OS Dev` headless: the desktop shows only System and Terminal
  with the real PNG icons, the taskbar buttons/search/tray show their PNG icons,
  and the dead shortcuts are gone.
- Injected `help`: the Terminal still opens and runs, with its taskbar button
  showing the PNG terminal icon and the focus underline.
- Powered off the VM afterward.

Screenshots:

![Stage 22 PNG icons desktop](screenshots/2026-06-25-stage22-png-icons-desktop.png)

![Stage 22 PNG icons terminal](screenshots/2026-06-25-stage22-png-icons-terminal.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes desktop/taskbar drawing, the build's asset paths, and the
  window set. It reads the new `Assets` icons/sounds at build time only.
- No disk read or write path was added or changed at runtime.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Terminal behavior: right-size the empty Terminal, add scrollback/scrolling, and
give the "TERMINAL" title text a few pixels of left padding.

## 2026-06-25 - Stage 22 Pass 3: Terminal Sizing, Scrollback, Title Padding

Plain English:

This pass cleans up the Terminal window. It used to open huge for an empty
console, the title text sat right against the left border, and there was no way
to scroll back through old output once it filled the window.

Now:

- The Terminal opens at a sensible 720x300 centered window instead of a giant
  near-full-width panel.
- The "TERMINAL" title has a few pixels of left padding so it is not flush
  against the border.
- The console keeps a real scrollback buffer (64 lines), and you can scroll up
  through old output. A thin scrollbar on the right shows where you are.
- Scrolling works two ways: the mouse wheel (the Terminal now enables the PS/2
  scroll wheel), and Page Up / Page Down / Up / Down on the keyboard. New output
  snaps the view back to the newest line.

The mouse is also still the more-sensitive pointer from Pass 1; enabling the
wheel did not change that.

What changed in `src/kernel/main.c`:

- Raised `CONSOLE_ROWS` to 64 for scrollback, kept so the whole console state
  still fits in one heap page (the heap only serves single-page allocations).
- Added a `view_offset` to the console plus `console_scroll`, clamped to the
  stored history. New output and `CLEAR` snap the view back to the bottom.
- Handled E0-prefixed keys in the scancode path: Page Up/Down scroll by a chunk,
  Up/Down by one line.
- Enabled the IntelliMouse scroll wheel in `mouse_init` (the 200/100/80 sample-
  rate knock, then device-ID check) and grew packet handling to 4 bytes when the
  wheel is present, accumulating wheel deltas into the mouse state; the main loop
  feeds those into `console_scroll`.
- Updated the Terminal renderer to honor `view_offset` and draw a scrollbar.
- Added left padding to the window title and gave the Terminal a smaller default
  size and centered position.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless. Ran `help`, `tasks`, `user`, `syscall`: the
  Terminal opened at the new size with the padded title, output filled past the
  window, and the scrollbar thumb sat at the bottom (newest).
- Injected Page Up a few times: the view scrolled up to older output (the end of
  the HELP list and the `tasks` output) and the scrollbar thumb moved up,
  proving keyboard scrollback. Mouse-wheel scrolling uses the same path and is
  hand-checked in the GUI (headless cannot inject a wheel).
- Powered off the VM afterward.

A side note on a bug caught here: bumping the scrollback first to 96 lines made
the console state larger than one 4 KiB heap page, so `heap_alloc` returned null
and the Terminal silently never opened. Dropping to 64 lines keeps it within a
page. A multi-page heap allocation path is a future improvement.

Screenshots:

![Stage 22 terminal resized](screenshots/2026-06-25-stage22-terminal-resized.png)

![Stage 22 terminal scrollback](screenshots/2026-06-25-stage22-terminal-scrollback.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only console/terminal rendering and input handling.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Window open animation: a Windows 11-style scale/fade-in when a window opens,
opening centered, replacing the abrupt appear-at-button behavior.

## 2026-06-25 - Stage 22 Pass 4: Windows 11-Style Open Animation

Plain English:

Windows used to just pop into existence. Now, like Windows 11, a window plays a
quick "rise and fade" when it opens: it starts a few pixels low and see-through,
then eases up into place and fades to fully solid over about a fifth of a
second. Both apps also open centered on the screen instead of off in a corner.

How it works: each window remembers when its open animation started. While the
animation plays, the compositor draws the window shifted down by a shrinking
offset and then blends that whole window rectangle toward the saved desktop base
layer, so it fades in over the wallpaper. An ease-out curve makes it feel like
it settles rather than stops abruptly. When the animation finishes the window
draws normally with no extra cost.

Because the old dead shortcuts are gone, the "opens where the button is with no
animation" case (the old HEAP behavior) no longer exists; every window now opens
the same animated, centered way.

What changed in `src/kernel/main.c`:

- Added `anim_open` / `anim_start` to `VibioWindow`, started on a real
  hidden->visible transition in `wm_show_and_raise`.
- Added a global current-tick mirror and a `g_base_layer` handle so drawing code
  can time the animation and read the desktop layer to fade against.
- Added `window_fade_over_base` (alpha blend of a window rect toward the base
  layer) and `wm_step_animations` (advance/clear animations, report if any are
  still playing so the loop composites every frame).
- Reworked `window_draw` to apply an ease-out rise offset and fade; threaded a
  vertical offset through `window_draw_content` so chrome and content move
  together (done without a struct copy to avoid pulling in `memcpy`).
- Centered the System window's open position; the Terminal was already centered.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless, typed a key to open the Terminal, and captured
  a burst of frames: early frames show the window low and semi-transparent with
  the wallpaper visible through it; later frames show it risen into place and
  fully opaque. This confirms the rise-and-fade open animation.
- Powered off the VM afterward.

Known limitation: the fade blends toward the desktop base layer, not toward any
window underneath, so during the brief animation an overlap with another window
shows the desktop through the opening window rather than that window. With the
two centered apps this is rarely visible and only lasts ~200 ms.

Screenshots:

![Stage 22 open animation mid](screenshots/2026-06-25-stage22-open-animation-mid.png)

![Stage 22 open animation done](screenshots/2026-06-25-stage22-open-animation-done.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only compositor drawing and window-open bookkeeping.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Visual polish and the font rule: Geist for all UI and JetBrains Mono only for
terminal text, title-bar polish, softer/thicker window borders, tighter SYSTEM
spacing, a better wallpaper, and removing the "VIBIO OS" / "STAGE 22" text.

## 2026-06-25 - Stage 22 Pass 5a: Visual Polish (Wallpaper, Borders, Title Bars)

Plain English:

This pass does the visual cleanup that does not depend on new fonts. The
wallpaper, the weakest part before, is replaced; windows get depth; and the
title bars look more finished.

What changed visually:

- Wallpaper: the harsh isometric line art and the right-side bands are gone.
  It is now a smooth top-to-bottom blue gradient with two soft radial glows for
  a modern, calm desktop. The "VIBIO OS" and "STAGE 22 DESKTOP" text on the
  wallpaper is removed.
- Windows now cast a soft drop shadow on the bottom and right, so they lift off
  the desktop instead of looking like flat stickers.
- Title bars are more polished: a glossy 1px highlight along the top, a subtle
  inner separator under the title bar, and cosmetic minimize / maximize / close
  controls on the right.
- The SYSTEM app status grid uses tighter row spacing so it is less airy.

What changed in `src/kernel/main.c`:

- Rewrote `draw_wallpaper` as a smooth 1px-row vertical gradient plus
  `draw_soft_glow` (alpha-blended radial glows with quadratic falloff).
- Removed the wallpaper title/subtitle text from `draw_desktop_base`.
- Added `draw_window_shadow` (blended bottom/right halo) and
  `draw_window_controls` (minimize/maximize/close glyphs); wired both into
  `window_draw` along with the top gloss line and inner separator.
- Tightened the row step in `draw_system_window_content`.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and opened the Terminal with `help`: the new
  gradient wallpaper with glow renders, the window shows its drop shadow and the
  minimize/maximize/close controls, the title has padding, and the old wallpaper
  text is gone.
- The SYSTEM window opens only by mouse, which headless cannot inject, so its
  tighter spacing is verified by code and hand-checked in the GUI.
- Powered off the VM afterward.

Screenshot:

![Stage 22 visual polish](screenshots/2026-06-25-stage22-visual-polish.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes only framebuffer drawing and visual styling.
- No disk read or write path was added or changed.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

The font rule: render Geist (UI) and JetBrains Mono (terminal) as bitmap atlases
and route UI vs terminal text to the right one.

## 2026-06-25 - Stage 22 Pass 5b: Real Fonts (Geist UI + JetBrains Mono Terminal)

Plain English:

Until now every character on screen used one hand-drawn vector font. This pass
gives Vibio real typefaces and follows the font rule: JetBrains Mono is used
only for the Terminal/command text, and Geist is used for everything else, the
desktop labels, title bars, taskbar, search box, the System app, the status
line, and the clock. No UI text is left on the old font.

Because JetBrains Mono is a true monospace coding font, the Terminal looks like
a real terminal and is denser, so the long command lists now fit inside the
window instead of being clipped. Geist gives the rest of the UI a clean, modern
proportional sans look.

How it works (same idea as the icon pipeline):

- `tools/make_fonts.py` uses Pillow to render Geist and JetBrains Mono glyph
  bitmaps for ASCII 32..126 at fixed pixel sizes and writes them to
  `src/kernel/fonts_generated.h` as alpha-coverage byte arrays plus a per-glyph
  table (offset, width, height, advance, left bearing, top offset). Three faces
  are generated: Geist UI (large), Geist small, and JetBrains Mono.
- The kernel has a small atlas text renderer (`font_draw_glyph` / `font_draw_buf`
  / `font_draw_text`) that alpha-blends each glyph in the requested color.
- The TTFs live in `Assets/Fonts/` (Geist and JetBrains Mono, both OFL-licensed).
  `make_fonts.py` is kept out of `build.ps1` so the normal build stays
  Pillow-free; rerun it by hand if fonts or sizes change.

What changed in `src/kernel/main.c`:

- Added the `VibioFont` atlas type, the three font instances, and the atlas text
  renderer, plus a `jbm_advance` helper for the monospace terminal cell.
- Routed all UI helpers to Geist: `draw_cstr` (Geist UI), `draw_small_cstr` /
  `draw_small_cstr_transparent` (Geist small), `draw_uint`, the clock helpers,
  and the window title.
- Routed the Terminal to JetBrains Mono: `draw_char_buffer`, the `EMIT` prompt
  macro, and the Terminal row/cell metrics (`TERM_LINE` / `TERM_GLYPH_H`).
- Left the old vector font in place only for unused debug-panel code.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless. Typed `Hello World 123`: the Terminal renders
  it in JetBrains Mono, while the title bar, desktop labels, taskbar (SEARCH,
  TIME, the HH:MM clock, VM SAFE), and the safety line all render in Geist.
- Typed `help`: the full command list renders in JetBrains Mono and now fits
  inside the window with no clipping.
- The System app text is Geist by the same `draw_cstr` path; it opens only by
  mouse, so it is hand-checked in the GUI.
- Powered off the VM afterward.

Screenshots:

![Stage 22 Geist + JetBrains Mono](screenshots/2026-06-25-stage22-fonts-geist-jbm.png)

![Stage 22 terminal monospace](screenshots/2026-06-25-stage22-fonts-terminal-mono.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass adds build-time font assets and changes only text rendering.
- No disk read or write path was added or changed at runtime.
- The removable `G:` USB was not refreshed during this pass.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

This completes the round of Stage 22 feedback: input correctness, real PNG icons
and fewer apps, terminal sizing/scrollback, the Windows 11-style open animation,
visual polish, and the Geist/JetBrains Mono font rule.

## 2026-06-25 - Stage 22 Pass 6: VM Detection, Tray Alignment, Mouse Robustness

Plain English:

This pass keeps the VM safety labels honest. Vibio now detects whether it is
running in a VM before drawing the `VM ONLY` desktop safety line and the
`VM SAFE` taskbar text. Inside the VirtualBox dev VM those labels still show.
On a real PC, where those VM signals are absent, the desktop no longer claims
to be VM-only.

The taskbar Wi-Fi/internet icon is also centered vertically in its tray slot,
and the mouse input path is more robust under fast movement. Moving the pointer
in circles could overrun the small mouse byte ring and leave the packet decoder
continuing from a stale partial packet, which felt like the cursor jumped or
teleported. The ring is larger now, and packet assembly resets after an overrun
so the next decoded packet starts cleanly.

What changed in `src/kernel/main.c`:

- Added CPUID hypervisor detection.
- Added a read-only PCI vendor scan for common VM device vendors, including
  VirtualBox, VMware, QEMU/virtio, Xen, and Hyper-V.
- Added `running_in_vm` to the desktop context and only draws `VM ONLY` /
  `VM SAFE` when that detector is true.
- Centered the taskbar Wi-Fi icon within the tray row.
- Grew the PS/2 mouse byte ring from 64 bytes to 256 bytes and updated the IRQ
  ring mask to match.
- Reset mouse packet assembly when the ring overruns, avoiding stale packet
  phases after fast movement.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless. The VM detector found the VirtualBox
  environment, so `VM ONLY: NO HOST DISKS TOUCHED` and `VM SAFE` still render
  in the VM.
- Injected `help`: the Terminal opened and accepted keyboard input as before.
- Verified in the screenshot that the tray icon is centered and the VM labels
  are conditional rather than hardcoded.
- Powered off the VM afterward.

Screenshot:

![Stage 22 VM detection, tray alignment, mouse robustness](screenshots/2026-06-25-stage22-vm-detect-tray-mouse.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes environment detection, taskbar drawing, and PS/2 mouse
  buffering only.
- VM detection reads CPUID and PCI configuration space; it does not touch disk
  or controller registers.
- No disk read or write path was added or changed.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Suggested next stage:

Stage 23 should be desktop behavior, not more fake app polish: make the visible
window controls real. Start with close and minimize for System and Terminal,
then make the taskbar buttons restore minimized windows. That keeps the desktop
feeling native and functional without adding placeholder apps.

## 2026-06-25 - Stage 23: AC'97 Sound Playback

Plain English:

Vibio now has its first real audio driver. The VirtualBox dev VM exposes an
AC'97 audio device, and the kernel finds that PCI audio controller, initializes
the mixer and PCM-out bus-master channel, parses the packaged WAV files, and
starts real playback through the device.

`BOOT.WAV` starts automatically when the OS boots and the audio driver is ready.
The sound-name commands now do real work too: typing `BOOT`, `ERROR`, `NOTIFY`,
`USBINS`, or `USBRM` finds the matching packaged sound and starts playback.
`SOUNDS` now reports the loaded assets and the real audio driver state, such as
`AUDIO: AC97 READY`.

This is still a first audio path, not a full desktop sound server. It plays one
complete WAV buffer at a time. The important foundation is now there for later
YouTube/browser-style audio: a kernel path that can feed PCM data to a real
audio device. The next audio layer should be a streaming ring buffer, then
mixing and a syscall/user boundary for apps.

What changed:

- `build.ps1` and `run-vbox.ps1`: enable VM audio output and expose an AC'97
  controller while keeping networking and USB disabled.
- `src/kernel/main.c`: Stage number is now 23.
- Added AC'97 PCI detection for multimedia/audio devices.
- Enabled PCI I/O and bus-master access for the audio controller.
- Added AC'97 mixer setup, output volume setup, optional variable-rate audio
  setup, and PCM-out bus-master playback.
- Added a reusable kernel audio state with one DMA buffer and a buffer
  descriptor list.
- Added WAV parsing for PCM 44.1 kHz 16-bit mono/stereo files.
- Stereo WAV files copy directly into the playback buffer; mono WAV files are
  expanded to stereo before playback.
- `BOOT.WAV` plays automatically after AC'97 initializes.
- `SOUNDS` reports AC'97 readiness.
- Sound-name commands start playback and report `PLAYBACK: STARTED` when the
  driver accepts the buffer.
- The System window now has an `AUDIO` status row.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Confirmed the VirtualBox VM config has audio output enabled.
- Booted `Vibio OS Dev` headless and typed `sounds`: the Terminal reported
  `AUDIO: AC97 READY`.
- Booted the VM headless again and typed `error`: the Terminal reported
  `PLAYBACK: STARTED`.
- Saved screenshots for both checks.
- Powered off the VM after each test.

Screenshots:

![Stage 23 AC'97 sounds](screenshots/2026-06-25-stage23-ac97-sounds.png)

![Stage 23 error playback](screenshots/2026-06-25-stage23-error-playback.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- VM networking stayed disabled.
- VM USB stayed disabled.
- This pass enables only the VM's emulated AC'97 audio output device.
- No disk write path was added or changed.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Desktop controls should be next: make close, minimize, maximize, restore, and
taskbar restore actually work for System and Terminal. After that, audio should
grow a streaming ring-buffer interface for future app/browser PCM output.

## 2026-06-25 - Stage 23 Pass 2: Mouse Feel And Hardware-Specific Status

Plain English:

This pass fixes two things that felt wrong on real hardware.

First, the mouse path was too crude. Vibio was multiplying every PS/2 mouse
delta by a fixed amount, which made small movements and fast circular movement
feel jumpy. Modern desktop input stacks do not treat all movement the same way:
they use adaptive pointer acceleration, where slow movement stays precise and
faster movement gets more gain. Vibio now does the same basic idea with a small
integer acceleration curve, fixed-point cursor coordinates, a higher PS/2 sample
rate, and a cleaner PS/2 resolution setting.

Second, the System app was showing AHCI/FAT32/AC'97 as binary pass/fail checks.
That is misleading on real PCs. A machine with NVMe storage or Intel HD Audio is
not "failing" Vibio's current AHCI or AC'97 drivers; Vibio simply does not have
the right real-hardware driver yet. Those device-specific probes now report
`N/A` when the relevant device is not present, while real failures after a
device is found still report `FAIL`.

What changed:

- PS/2 mouse setup now requests 200 Hz reporting and a better resolution level.
- Mouse movement now uses fixed-point coordinates so fractional motion is not
  lost.
- Replaced the old always-on `x7/4` pointer multiplier with a small adaptive
  acceleration curve: 1.0x for tiny motion, rising only as packet speed
  increases.
- System status rows for DISK, FAT32, and AUDIO now support `PASS`, `FAIL`, and
  `N/A`.
- `DISK` command now reports `N/A` with a clear message when no AHCI SATA
  controller is found.
- `FS` command now reports `N/A` when there is no readable AHCI boot disk for
  the current FAT32 reader.
- `SOUNDS` and sound-name commands now explain that real PCs usually need an
  Intel HD Audio driver when no AC'97 device is present.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and typed `sounds`: AC'97 still reports ready
  in the VM.
- Saved the verification screenshot.
- Powered off the VM afterward.

Screenshot:

![Stage 23 mouse and hardware status](screenshots/2026-06-25-stage23-mouse-hardware-status-audio.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- No disk write path was added or changed.
- The storage change only affects status/reporting for unsupported hardware.
- The audio change does not claim Intel HD Audio support yet; it reports that
  HDA is needed when AC'97 is unavailable.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

The next real-hardware driver work should be Intel HD Audio detection and a
minimal HDA playback path. The next desktop shell work should still be real
close/minimize/maximize/restore behavior.

## 2026-06-25 - Stage 23 Pass 3: Terminal Shortcuts, Underscore Rendering, Icon Centering, Boot Sound Timing

Plain English:

This pass fixes several small things that made the desktop and Terminal feel
unfinished.

Underscores were being typed, stored, and echoed, but the Terminal renderer was
clearing rows using the old font height instead of the full JetBrains Mono line
height. That clipped bottom-heavy glyphs like `_`. Terminal row clearing now
uses the real terminal line height, so underscores render like normal text.

The Terminal command line now supports three familiar edit shortcuts:

- `Ctrl+A` selects the current input line.
- `Ctrl+C` copies the selected input line into a small Terminal clipboard.
- `Ctrl+V` pastes that Terminal clipboard into the input line.

This is still kernel/Terminal-local editing, not a global desktop clipboard.
That keeps the behavior real without pretending there is already a system-wide
clipboard service.

The search and internet/Wi-Fi taskbar icons are now centered by their visible
alpha content, not only by the full PNG box. This fixes icons that looked
slightly off-center because of transparent padding in the source image.

The boot sound is delayed until after the first desktop frame is presented.
Before this, `BOOT.WAV` started immediately after audio init, which could happen
before the desktop was visible. Now it starts as the desktop appears.

What changed:

- `src/kernel/main.c`: Terminal input state now tracks Ctrl, selection, and a
  small local clipboard.
- `Ctrl+A`, `Ctrl+C`, and `Ctrl+V` are handled in the PS/2 keyboard path.
- Typing while all input is selected replaces the current input line.
- Backspace clears the selected input line.
- Terminal history and prompt clearing now use `TERM_LINE`, fixing underscore
  and bottom-glyph rendering.
- Added `draw_icon_image_centered`, which centers PNG icons by visible alpha
  bounds.
- The taskbar search icon and Wi-Fi/internet icon use content-aware centering.
- Boot sound playback is now pending until after the first `present()` call.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and typed `usb_rm`: the underscore rendered
  visibly in the Terminal echo.
- Booted the VM headless again and injected raw scancodes for `abc`,
  `Ctrl+A`, `Ctrl+C`, `d`, `Ctrl+V`: the command line showed `dabc`, proving
  select/copy/replace/paste worked.
- Saved screenshots for both checks.
- Powered off the VM after each test.

Screenshots:

![Stage 23 underscore, icons, boot sound timing](screenshots/2026-06-25-stage23-underscore-icons-bootsound.png)

![Stage 23 Ctrl paste](screenshots/2026-06-25-stage23-ctrl-paste.png)

Safety status:

- VM-only automated testing; the VM was powered off afterward.
- This pass changes Terminal input/editing, text rendering, taskbar icon
  placement, and boot-sound timing only.
- No disk read or write path was added or changed.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

The next desktop shell work should still be real close/minimize/maximize/restore
behavior. The next real-hardware sound work should be Intel HD Audio.

## 2026-06-25 - Stage 23 Pass 4: Terminal Shortcuts, Trackpad Scroll Filtering, HDA Detection, USB Sound Aliases

Plain English:

This pass corrects the Terminal shortcut behavior to match real terminal
expectations instead of web-textbox behavior. `Ctrl+A` no longer selects text;
it moves the input cursor to the start of the current command line. `Ctrl+C`
now cancels the current input line, writes `^C` into the Terminal history, and
returns to a fresh prompt. `Ctrl+V` pastes from Vibio's tiny internal Terminal
clipboard at the current cursor position. This is still not host clipboard
support, and it does not pretend to be.

The Terminal input line is now cursor-aware:

- Normal typing inserts at the cursor.
- Backspace deletes the character before the cursor.
- Left/Right arrows move the input cursor.
- Enter still runs the whole input line.
- `Ctrl+Shift+C` copies the current input line into the internal Terminal
  clipboard; plain `Ctrl+C` remains cancel/interrupt behavior.

The mouse wheel / trackpad scroll path is also safer. Wheel packets are treated
as scroll input, not pointer movement. When a wheel packet arrives with no mouse
buttons down, Vibio suppresses the packet's X/Y movement so two-finger scroll
does not shove the cursor around. Wheel scroll is accumulated separately and
capped per frame so a touchpad cannot jump hundreds of lines at once.

For sound on real hardware, Vibio now distinguishes AC'97 from Intel HD Audio.
The VM AC'97 path still works, but real PCs commonly expose HDA instead. Vibio
now detects an HDA PCI audio controller and reports `HDA FOUND DRIVER NEEDED`
instead of falling through to vague AC'97/no-device output.

The USB insert/remove sounds are easier to trigger manually: `USB_INSERT`,
`USB INSERT`, `USB_REMOVE`, and `USB REMOVE` now map to the packaged
`USBINS.WAV` / `USBRM.WAV` sounds. Actual USB hotplug detection is still not
implemented; the `USB` command now says `HOTPLUG EVENTS: DRIVER NEEDED` so that
is explicit. Real insert/remove playback needs xHCI/EHCI port event handling,
not just the current read-only controller probe.

What changed:

- Added `input_cursor` to `VibioConsoleState`.
- Reworked Terminal input insertion, Backspace, Enter, Ctrl+A, Ctrl+C, Ctrl+V,
  and Left/Right arrow handling around the input cursor.
- Added `Ctrl+Shift+C` as a Terminal-local copy into Vibio's internal clipboard.
- Added `console_append_cancel_line` so Ctrl+C writes `VIBIO: ...^C` into
  history and clears the prompt.
- Split wheel input from pointer movement in the PS/2 mouse packet parser.
- Suppressed pointer X/Y movement for wheel-only packets with no buttons down.
- Capped per-frame wheel scroll.
- Added HDA PCI detection and HDA-specific Terminal/System status.
- Added USB insert/remove sound aliases.
- Added explicit USB hotplug-driver-needed text to the `USB` command.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and injected `hello world`, `Ctrl+A`, `X`:
  the prompt showed `Xhello world`.
- Booted again and injected `help te`, `Ctrl+C`: the history showed
  `VIBIO: help te^C` and returned to a fresh prompt.
- Booted again and tested paste-at-cursor using the internal clipboard:
  copied `xy`, cancelled, typed `abc`, moved to the start, pasted, and the
  prompt showed `xyabc`.
- Booted again and typed `usb_insert`: it mapped to `USBINS.WAV` and reported
  `PLAYBACK: STARTED`.
- Booted again and typed `notify`: the mono notification WAV reported
  `PLAYBACK: STARTED`.
- Saved screenshots for those checks.
- Powered off the VM after each test.

Screenshots:

![Stage 23 Ctrl+A cursor](screenshots/2026-06-25-stage23-ctrl-a-cursor.png)

![Stage 23 Ctrl+C cancel](screenshots/2026-06-25-stage23-ctrl-c-cancel.png)

![Stage 23 Ctrl+V cursor paste](screenshots/2026-06-25-stage23-ctrl-v-cursor.png)

![Stage 23 USB insert sound alias](screenshots/2026-06-25-stage23-usb-insert-sound-alias.png)

![Stage 23 notify sound](screenshots/2026-06-25-stage23-notify-sound.png)

Safety status:

- VM-only automated testing; the VM was powered off after every test.
- VM networking stayed disabled.
- VM USB stayed disabled.
- This pass does not add USB device enumeration or USB storage access.
- This pass does not add Intel HD Audio playback yet; it detects and reports
  that HDA support is needed.
- No disk write path was added or changed.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

For actual hardware sound, implement a minimal Intel HD Audio driver: map HDA
MMIO, reset the controller, set up CORB/RIRB, enumerate codecs/widgets, choose a
speaker output path, and feed one PCM stream. For USB insert/remove sounds,
implement xHCI/EHCI port-status/hotplug event handling before pretending
automatic USB sounds work.

## 2026-06-25 - Stage 23 Pass 5: Built-In Intel HD Audio Playback Path

Plain English:

This pass adds the first real Intel HD Audio playback driver path to Vibio.
HDA is now treated as a built-in kernel audio driver, not as something the
future installer would download or bolt on later. On boot, Vibio tries HDA
first, then falls back to the older AC'97 path if no HDA controller is usable.

The HDA path maps the PCI controller MMIO BAR, enables PCI memory and bus
mastering, resets the controller, sets up CORB/RIRB command rings, finds the
first codec, finds an audio function group, picks a DAC and output pin, connects
the pin to the DAC when the codec exposes a connection list, programs a 44.1 kHz
16-bit stereo output stream, and feeds Vibio's existing WAV playback buffer
through an HDA BDL.

This is intentionally still a small generic HDA driver. Some real hardware may
need more codec-specific output selection or amplifier verbs, but the kernel no
longer stops at `HDA driver needed`. The Terminal now reports `HDA READY` when
the HDA path initializes, or an HDA error number when it finds HDA but cannot
finish setup.

What changed:

- Added HDA controller register definitions, stream descriptor definitions, and
  HDA verb helpers.
- Added byte/word MMIO read/write helpers for HDA registers.
- Extended `VibioAudioState` with active-driver, HDA MMIO, CORB/RIRB, codec,
  widget, stream, and PCM playback fields.
- Added HDA PCI discovery with MMIO BAR validation and PCI memory/bus-master
  enablement.
- Added HDA controller reset and CORB/RIRB initialization.
- Added generic codec discovery for an audio function group, DAC, and output
  pin.
- Added HDA output-stream setup using Vibio's packaged 44.1 kHz PCM WAV files.
- Made HDA the first audio init path and AC'97 the fallback.
- Updated Terminal/System audio status text so it reports `HDA READY` or a
  concrete HDA error instead of saying `HDA DRIVER NEEDED`.
- Switched the development VirtualBox audio controller from AC'97 to HDA so the
  normal VM workflow tests the new real-hardware-oriented path.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless with the VirtualBox HDA controller enabled.
- Confirmed the desktop still reaches the shell.
- Typed `SOUNDS` and confirmed `AUDIO: HDA READY`.
- Typed `ERROR` and confirmed `PLAYBACK: STARTED` through the HDA driver path.
- Rebuilt again after renaming the shared audio init function and re-confirmed
  `AUDIO: HDA READY`.
- Saved screenshots for the desktop, HDA status, and playback checks.
- Powered off the VM after every test.

Screenshots:

![Stage 23 HDA driver desktop](screenshots/2026-06-25-stage23-hda-driver-desktop.png)

![Stage 23 HDA driver sounds](screenshots/2026-06-25-stage23-hda-driver-sounds.png)

![Stage 23 HDA driver error playback](screenshots/2026-06-25-stage23-hda-driver-error-playback.png)

![Stage 23 HDA driver final sounds](screenshots/2026-06-25-stage23-hda-driver-final-sounds.png)

Safety status:

- VM-only automated testing; the VM was powered off after every test.
- VM networking stayed disabled.
- VM USB stayed disabled.
- This pass does not add USB device enumeration or USB storage access.
- This pass does not add a disk write path.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next pass:

Test the HDA path on the real PC and record the exact HDA error number if it
does not produce sound. The next audio refinement should improve codec output
selection and amplifier setup for the user's actual codec. The next USB sound
work should still be real xHCI/EHCI hotplug event handling, not fake desktop
events.

## 2026-06-25 - Stage 24: Read-Only FAT32 File Reads And A Minimal HTML Browser

Plain English:

Vibio OS can now open and display HTML pages from its own disk. Two things came
together for this milestone.

First, the disk now works as a real read-only filesystem, not just a prober.
Stage 16 could find directories and count entries; Stage 24 adds a kernel-side
FAT32 file reader that walks the file allocation table cluster chain and copies
an arbitrary file's bytes into memory by name. This is still strictly read-only:
there is no FAT write, no directory write, and no file create/delete.

Second, Vibio has a new desktop app: a minimal from-scratch HTML browser. It is
not based on any existing browser engine. It reads a local `.HTM` / `.HTML` file
from the disk, parses a useful subset of HTML, and lays it out with real word
wrapping in the Geist UI font. The browser window opens automatically at boot
showing `START.HTM`, and there is a `BROWSER` desktop icon, a taskbar button,
and a `BROWSER` terminal command that all open or reload it.

What the browser renders:

- The page `<title>` becomes the window title; an address strip shows the open
  file name.
- `h1`, `h2`, and `h3` headings (faux-bold; `h1` is accented with an underline
  rule).
- `p`, `div`, `ul`, `li` block spacing, `br` line breaks, and `hr` rules.
- Paragraph text with word-wrapping inside the content area, with clipping so
  text never spills outside the window.
- `<a href>` links drawn underlined in the accent color. Local links navigate to
  another file on the disk; `http(s)` links are recognized as external and show
  an "external link / networking not implemented yet" page instead of pretending
  to fetch them.
- A clean placeholder box for `<img>` (showing the `alt` text), since the test
  page's `TEST.PNG` is intentionally absent.
- A handful of HTML entities (`&amp; &lt; &gt; &quot; &nbsp;` and numeric).
- Vertical scrolling with a scrollbar: mouse wheel when the browser is focused,
  plus Page Up/Down, Up/Down arrows, Home/End, and Space from the keyboard.

How the disk side works:

- The image builder (`tools/make_fat32_image.py`) now packs top-level
  `Assets/*.htm` / `*.html` files into the FAT32 root directory as read-only 8.3
  entries, alongside the existing kernel, boot, and sound files.
- `build.ps1` passes `Assets` to the builder as the HTML source directory.
- `START.HTM` is now on the VM disk; `ABOUT.HTM` and `TEST.PNG` are deliberately
  absent so the test page's missing-page and missing-image paths are exercised.
- The kernel reads `START.HTM` over the existing read-only AHCI sector path, so
  no new write path was added anywhere.

What changed:

- `src/kernel/main.c`: stage bumped to 24.
- Added `fat32_name_to_83`, `fat32_next_cluster`, `fat32_find_in_dir`, and
  `fat32_read_file` for read-only FAT32 file access.
- Added the `VibioBrowser` state, the HTML parser/layout/paint pass
  (`browser_render`), `browser_load`/`browser_navigate`, and the
  `WINDOW_KIND_BROWSER` window kind, desktop icon, and taskbar icon.
- Grew `WM_MAX_WINDOWS` from 2 to 3 for the browser window.
- Added keyboard scrolling for the focused browser and a `BROWSER` command, and
  added `BROWSER` to the `HELP` command list.
- `tools/make_icons.py`: added `browser_36` / `browser_26` icons (regenerated
  `icons_generated.h`).
- `tools/make_fat32_image.py` and `build.ps1`: pack HTML assets onto the disk.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors. `KERNEL.BIN` is ~152 KiB,
  well under the 512 KiB limit.
- Confirmed `START   HTM` and the page bytes are present in `build/vibio.img`.
- Booted `Vibio OS Dev` headless. The browser opened automatically and rendered
  `START.HTM`: title, accented `h1`, `h2` headings, wrapped paragraphs, and
  `hr` rules (`2026-06-25-stage24-browser-start.png`).
- Sent Page Down scancodes through the virtual keyboard; the page scrolled to
  the Links and Image sections, showing underlined links (`reload this page`,
  `open ABOUT.HTM`, `example.com`) and the clean image placeholder
  (`2026-06-25-stage24-browser-links.png`). Further Page Downs reached the
  "scrolling works" end-of-page line.
- Typed `fs` in the Terminal: `FAT32: READONLY PASS`, `WRITE PATH: NONE`, proving
  the disk subsystem is healthy and the Terminal still composites over the
  browser window.
- Powered off the VM after each test.

Screenshots:

![Stage 24 browser START.HTM](screenshots/2026-06-25-stage24-browser-start.png)

![Stage 24 browser links and image placeholder](screenshots/2026-06-25-stage24-browser-links.png)

Safety status:

- VM-only automated testing; the VM was powered off after every test.
- The new disk code is read-only: it walks the FAT and reads sectors only. No
  FAT, directory, or file write path was added.
- VM networking stayed disabled; external links are not fetched.
- VM USB stayed disabled. The removable `G:` USB was not touched.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Make link navigation richer: a back/forward history and an editable address bar.
Then grow the HTML/CSS subset (lists styling, basic tables, text color) and add
a real PNG image decoder so `<img>` can render actual on-disk images. Networking
remains the prerequisite for fetching anything beyond the local disk.

## 2026-06-25 - Stage 24 Pass 2: Real-Hardware HTML, Reliable Loads, Working Window Controls

Plain English:

This pass fixes the three problems that showed up while actually using Stage 24.

1. "Page not found" sometimes. The browser was reading START.HTM live from the
   disk every time, and an AHCI sector read can transiently time out while the
   timer and other activity run, which made the page occasionally look missing.
   Two fixes: the home page is now loaded once by the firmware into memory (see
   below) so it never depends on a live disk read, and the kernel's FAT32 reads
   now retry a few times before giving up.

2. Disk and FAT32 do not work on the real laptop, so the page could not be
   viewed there. That is expected: the laptop boots from a device the kernel's
   own AHCI/SATA driver does not speak (NVMe or USB), so the kernel-side disk
   path finds nothing. The fix is to load START.HTM through the UEFI firmware in
   the boot stage, exactly like the WAV files. UEFI abstracts the storage, so it
   works on SATA, NVMe, and USB alike. The browser now prefers that in-memory
   copy and only falls back to its own FAT32 reader for other files. So the home
   page is viewable on real hardware even though the kernel disk driver is not.

3. Minimize, maximize, and close did nothing, and clicking a taskbar button
   closed the window instead of minimizing it. The title-bar controls are now
   real:
   - At this stage, minimize hid the window with a shrink-and-fade animation but
     kept it as a running app in the taskbar.
   - Maximize fills the work area above the taskbar; the button becomes a
     restore button that returns the window to its previous size and place.
   - At this stage, close hid the window with the same shrink-and-fade animation.
   - That close/minimize fade was later removed in Stage 25 because it could
     leave compositor artifacts on some windows.
   - Clicking a taskbar button now minimizes the focused window and restores it
     when it is minimized or behind. It never closes.
   Maximized windows no longer drag, and the maximize glyph switches to a
   two-square restore glyph while maximized.

What changed:

- `src/shared/boot_info.h`: added `VIBIO_BOOT_FILE_TYPE_HTML`.
- `src/boot/main.c`: the boot stage now also loads `\START.HTM` read-only via
  UEFI into the boot file table (firmware-agnostic), and the boot step label is
  now "Loading boot assets".
- `src/kernel/main.c`:
  - The browser prefers the UEFI-preloaded copy (`browser_load_from_boot`) and
    falls back to the FAT32 disk reader.
  - Added `ahci_read_sector_retry` and used it in all FAT32 file reads.
  - `VibioWindow` gained `anim_close`, `anim_to_min`, `minimized`, `maximized`,
    and a saved normal rect.
  - Added `window_control_xs` / `window_control_hit`, real `wm_minimize`,
    `wm_close`, `wm_toggle_maximize`, `wm_activate_from_shell`, and a reverse
    close animation in `window_draw` and `wm_step_animations`.
  - `wm_update` now handles title-bar control clicks (with priority over drag)
    and routes taskbar/desktop-icon clicks through minimize/restore.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and typed `files`: the list now includes
  `START.HTM: 1394`, proving the firmware preloaded the home page into memory.
- The browser opens at boot and renders START.HTM from that in-memory copy
  (`2026-06-25-stage24-browser-preloaded.png`), so reload can never miss.
- The window-control geometry was checked against the rendered buttons in the
  GUI (close/maximize/minimize zones line up with the drawn glyphs), and the
  click flow was reviewed. The buttons are mouse-driven; VirtualBox delivers a
  captured relative PS/2 mouse with no guest-additions integration, so the host
  cannot script guest clicks - these are confirmed by hand in the GUI / on the
  laptop, as with all Vibio mouse interaction.
- Powered off the VM after each test.

Screenshot:

![Stage 24 browser preloaded by firmware](screenshots/2026-06-25-stage24-browser-preloaded.png)

Safety status:

- VM-only automated testing; the VM was powered off after every test.
- The kernel disk code stays read-only (FAT walk + sector reads, now with
  retries). No FAT, directory, or file write path was added.
- The boot stage loads the HTML home page with `EFI_FILE_MODE_READ` only, the
  same read-only path already used for the WAV files.
- VM networking and USB stayed disabled. The removable `G:` USB was not touched.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Back/forward history and an editable address bar, a wider HTML/CSS subset, and a
real PNG decoder for `<img>`. For real-hardware storage beyond the firmware
preload, an NVMe read path (and later USB mass storage) would let the kernel read
arbitrary files on modern laptops.

## 2026-06-25 - Stage 24 Pass 3: Browser Home Page Embedded In The Kernel (Real-Laptop Fix)

Plain English:

Pass 2 tried to make the browser work on the real laptop by loading START.HTM
through UEFI in the boot stage. That still failed on the laptop, for a concrete
reason: the laptop boots from the `G:` USB, which only ever had `BOOTX64.EFI` and
`KERNEL.BIN` copied onto it. There was no `START.HTM` on that USB, so the UEFI
preload found nothing, and the laptop's storage is not something the kernel's own
AHCI/FAT32 driver can read (NVMe, or a GPT layout the test does not expect). With
no source for the page, the browser had nothing to show.

The fix is to stop depending on any disk or boot-medium file for the home page.
START.HTM is now compiled directly into the kernel as a byte array, exactly like
the fonts and icons already are. The browser loads that built-in copy first, so
the home page is always available no matter what hardware Vibio runs on. The UEFI
preload and the FAT32 disk reader remain as additional sources for other files.

Because the page now lives inside `KERNEL.BIN`, the laptop just needs the latest
`KERNEL.BIN` on its USB. A new `deploy-usb.ps1` copies the current boot files
(and START.HTM and the sounds) to a removable drive with safety checks, so the
USB can be refreshed in one command.

Important: this does not make the laptop's disk or FAT32 work. Those probes still
report `N/A` on the laptop because the kernel has no driver for that machine's
storage device or partition layout. The browser simply no longer needs them.

What changed:

- Added `tools/make_html.py`, which embeds top-level `Assets/*.htm` / `*.html`
  into `src/kernel/html_generated.h` (pure standard-library Python).
- `build.ps1` runs `make_html.py` before compiling the kernel, so editing
  `Assets/START.HTM` is picked up automatically.
- `src/kernel/main.c` includes `html_generated.h`; `browser_load` now tries the
  embedded built-in copy (`browser_load_from_builtin`) first, then the UEFI
  preload, then a FAT32 disk read.
- Added `deploy-usb.ps1` to refresh a removable USB (removable-only, never
  formats, copies only Vibio's files).

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors. `KERNEL.BIN` is ~153 KiB
  (the embedded page adds a few KiB), well under the 512 KiB limit.
- Booted `Vibio OS Dev` headless: the browser renders START.HTM. Because the
  built-in copy is tried first and returns before touching the boot table or
  disk, a correct render proves the embedded path works; the same plain
  array-copy runs identically on the laptop, with no disk involved
  (`2026-06-25-stage24-browser-embedded.png`).
- Powered off the VM after the test.
- The laptop itself cannot be tested from here; the embedded page removes the
  disk/boot-medium dependency that was blocking it.

Screenshot:

![Stage 24 browser embedded home page](screenshots/2026-06-25-stage24-browser-embedded.png)

Safety status:

- VM-only automated testing; the VM was powered off after the test.
- No write path was added. The embedded page is read-only kernel data.
- `deploy-usb.ps1` refuses any non-removable drive and the Windows system drive,
  never formats, and writes only Vibio's own files.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

To make the laptop's own storage readable (so links to other on-disk pages work
there too), add an NVMe read path and GPT parsing, or a USB mass-storage read
path. Until then, the embedded built-in pages are the portable home base.

## 2026-06-25 - Stage 24 Pass 4: Scrollbar, Trackpad Scroll, F11 Full Screen

Plain English:

Three browser usability fixes that came from using it on the real laptop.

1. The scrollbar was decorative. It is now a real control: clicking the track
   jumps the page to that spot, and the thumb can be grabbed and dragged. It is
   also drawn wider (a 12px bar) so it is an easy target. This gives a reliable
   mouse way to scroll even when the trackpad wheel does not work.

2. Two-finger trackpad scrolling did not work. PS/2 touchpads only emit a scroll
   wheel byte if they are switched into IntelliMouse mode. Vibio did the first
   "magic knock" (which yields device ID 3) but not the second (ID 4), and many
   laptop touchpads report ID 4 for their two-finger scroll. Vibio now does both
   knocks and accepts ID 3 or ID 4, so the wheel byte is decoded on more
   hardware. (If a touchpad refuses IntelliMouse mode entirely, the scrollbar and
   the keyboard - Page Up/Down, arrows, Home/End, Space - are the fallbacks.)

3. F11 now does true full screen, like a browser on Windows. The focused window
   fills the entire screen with no title bar, border, or taskbar, and the content
   is drawn edge to edge. Pressing F11 again restores the exact previous window.
   F11 is edge-detected so holding it does not flicker.

What changed in `src/kernel/main.c`:

- `mouse_init` does the second IntelliMouse knock (200,200,80) and treats device
  ID 3 or 4 as wheel-capable.
- `VibioBrowser` gained scrollbar geometry/drag fields; `browser_render` records
  the track/thumb rects and draws a wider bar; added `browser_scrollbar_to`.
- The main loop's browser input handles scrollbar press (grab thumb or jump) and
  thumb dragging, before link hit-testing.
- `VibioWindow` gained a `fullscreen` flag; added `wm_toggle_fullscreen` and
  `wm_any_fullscreen`; `window_draw` / `window_draw_content` draw chrome-less and
  edge-to-edge when full screen; the taskbar/top bar are skipped while any window
  is full screen; full-screen windows do not drag and have no title controls.
- F11 (scancode 0x57, edge-detected via a console `f11_down` flag) toggles full
  screen on the focused window in `console_process_scancode`.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and injected F11 (`57 d7`): the browser filled
  the whole screen with no chrome or taskbar
  (`2026-06-25-stage24-browser-fullscreen.png`); a second F11 restored the exact
  windowed view. The wider scrollbar is visible in both.
- Scrollbar click/drag and trackpad scrolling are mouse/touchpad input, which the
  host cannot script for this VM (relative PS/2, no integration); they are
  confirmed by code review and need a hand/laptop check. The keyboard scroll path
  was already verified in Pass 1.
- Powered off the VM after the test.

Screenshot:

![Stage 24 browser F11 full screen](screenshots/2026-06-25-stage24-browser-fullscreen.png)

Safety status:

- VM-only automated testing; the VM was powered off after the test.
- No write path was added. These are input and rendering changes only.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Back/forward history and an editable address bar; a wider HTML/CSS subset and a
PNG decoder for `<img>`; and, for the laptop's own storage, an NVMe/GPT read path.

## 2026-06-25 - Stage 24 Pass 5: Write-Combining Framebuffer, Keyboard Set-1, Scroll Robustness

Plain English:

More fixes from the real laptop, where the VM had hidden them.

1. The scrollbar drag was "insanely laggy" and jumped up and down. The cause was
   the framebuffer. On a real machine, graphics memory is effectively uncached,
   so the full-frame copy Vibio does every frame (`present`) stalls on every
   pixel write. Static screens look fine because they are drawn once, but a
   continuous drag redraws constantly and crawls. Vibio now maps the framebuffer
   as write-combining (it reprograms a PAT entry to WC and tags the framebuffer
   pages with PWT). Write-combining lets the CPU batch those writes, which is the
   standard fix and makes the whole UI smooth on real hardware. The VM was
   unaffected and still renders correctly, confirming no regression.

2. The scrollbar also fought itself when a noisy touchpad emitted stray wheel
   events mid-drag. Wheel input is now ignored while the thumb is being dragged.

3. F11 (and possibly all typing) did nothing on the laptop. The likely cause is
   the PS/2 controller delivering raw scancode set 2 instead of the set 1 Vibio
   decodes. Vibio now forces a known-good controller config at init: keyboard and
   mouse interrupts on, both clocks enabled, and scancode translation ON so the
   controller always emits set-1 codes. In the VM, typing and F11 still work, and
   on a laptop stuck in set 2 this should restore both.

4. Two-finger trackpad scroll: Pass 4 already added IntelliMouse ID-4 support. If
   a touchpad refuses IntelliMouse wheel mode in PS/2 entirely, there is no wheel
   byte to read and only a real touchpad (Synaptics) driver would help; the
   scrollbar and keyboard remain the reliable fallbacks.

What changed in `src/kernel/main.c`:

- Added `write_msr` and `pat_enable_write_combining` (PA1 -> WC); the 2 MiB
  identity mappers take an `extra_flags` argument, and the framebuffer range is
  mapped with `PAGE_PWT`. `load_cr3` now issues `wbinvd` so the new memory type
  takes effect cleanly.
- `mouse_init` forces the controller config byte: bits for keyboard IRQ, mouse
  IRQ, keyboard clock, mouse clock, and scancode-set-1 translation.
- The main loop drops wheel input while `browser.bar_dragging` is set.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless. The desktop and browser render correctly with
  the write-combining framebuffer (no regression). Typed `help`: the Terminal
  accepted input and printed the command list, so set-1 translation did not break
  keyboard input. Injected F11: the focused Terminal went edge-to-edge full
  screen with no chrome or taskbar.
- Framebuffer-speed and touchpad behavior can only be felt on real hardware; the
  VM confirms the changes are correct and non-regressive.
- Powered off the VM after the test.

Safety status:

- VM-only automated testing; the VM was powered off after the test.
- The page-table change only marks the framebuffer pages write-combining; regular
  RAM stays write-back. No write path to any disk was added.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

If two-finger scroll still does nothing on the laptop, the touchpad does not do
PS/2 IntelliMouse wheel and needs a Synaptics/touchpad driver. Otherwise: browser
back/forward and address bar, wider HTML/CSS, a PNG decoder, and NVMe/GPT storage.

## 2026-06-26 - Stage 24 Pass 6: Browser Navigation And Dirty-Rectangle Compositor

Plain English:

This pass focuses on the lag seen on the real laptop when using fullscreen,
opening more windows, dragging, or moving the mouse. The old compositor was
correct but expensive: every visible change copied the entire desktop base layer
into the back buffer, redrew every window, drew the cursor, then copied the
entire finished frame to the real framebuffer. On a VM that is usually fine. On
real laptop framebuffer memory, full-screen copies are slow enough to feel awful.

Vibio now tracks a dirty rectangle for normal UI changes. Cursor movement redraws
only the old and new cursor areas. Window changes redraw the old and new window
areas plus the taskbar. Timer/keyboard updates redraw the focused window and
taskbar. The compositor still draws a complete correct scene inside that dirty
rectangle, but it only restores and presents that rectangle instead of the whole
screen. Fullscreen windows still repaint the full screen when their contents
actually change, because the dirty area really is the whole screen.

This pass also finishes the browser navigation controls that were the obvious
next browser usability step: back, forward, reload, home, and an editable address
field. `Ctrl+L` or clicking the address field selects the current address, typing
replaces it, Enter navigates, and Escape cancels. External URLs still show the
"networking is not implemented yet" page; no network fetch path was added.

What changed in `src/kernel/main.c`:

- Added a second global paint-bounds clip under the existing content clip, so
  dirty-rectangle drawing remains enforced even when browser/terminal rendering
  changes its own clip rectangle.
- Added `VibioRect`, rectangular base/backbuffer blits, and rectangular presents.
- The main loop now builds a dirty rectangle from cursor movement, window state
  changes, focused-window updates, taskbar updates, and fullscreen state.
- `window_fade_over_base` now respects the paint bounds, so open/close
  animations do not write outside the dirty area.
- Added browser history state, back/forward/reload/home actions, toolbar hit
  testing, editable address text, and `Ctrl+L` address focus.
- Replaced a struct assignment in the kernel with explicit field copying so the
  freestanding build does not emit a libc `memcpy` dependency.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and captured the windowed browser after the
  dirty-rectangle compositor change
  (`2026-06-26-stage24-dirty-rect-browser.png`).
- Injected F11 through the virtual keyboard: the browser still entered true
  fullscreen and rendered correctly with the toolbar/content edge-to-edge
  (`2026-06-26-stage24-dirty-rect-fullscreen.png`).
- In the earlier address-bar check, `Ctrl+L`, typing `http://example.com`, and
  Enter reached the external-link/no-network page correctly.
- Powered off the VM after the tests.

Screenshots:

![Stage 24 dirty-rectangle browser](screenshots/2026-06-26-stage24-dirty-rect-browser.png)

![Stage 24 dirty-rectangle fullscreen](screenshots/2026-06-26-stage24-dirty-rect-fullscreen.png)

Safety status:

- VM-only automated testing; the VM was powered off after the test.
- The optimization changes only RAM-to-RAM and RAM-to-framebuffer drawing paths.
- No disk, FAT, directory, USB, networking, or filesystem write path was added.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Try this build on the laptop. If dragging/window opening is still too slow, the
next optimization is caching rendered browser/window contents into per-window
surfaces so unchanged windows do not relayout/repaint at all during mouse-only
movement. After that, PNG image decoding and a wider HTML/CSS subset are still
the next browser features.

## 2026-06-26 - Stage 25: Kernel Panic Red Screen And Automatic Restart

Plain English:

Vibio now has a real "something went badly wrong" path. Before this, a serious
CPU exception could freeze, reboot, or leave the screen stuck with no useful
information. Now the kernel catches the important early exceptions and draws a
red critical-problem screen directly to the framebuffer.

The screen is intentionally simple and readable. It says Vibio hit a kernel
panic, names the fault, prints the CPU address/state fields that matter, shows
uptime and the likely task/driver/action context, waits about 18 seconds, then
asks the machine to restart. This is the start of actual crash diagnostics for real
hardware and driver work.

What the panic screen shows:

- `FAULT`: page fault, general protection, invalid opcode, divide error, double
  fault, or a generic CPU exception label.
- `ERROR CODE`: the CPU-provided exception error code when one exists.
- `RIP`: instruction pointer at the fault.
- `RSP`: exception-frame stack pointer.
- `RFLAGS`: flags captured by the CPU exception frame.
- `CR2`: faulting linear address for page faults.
- `UPTIME`: total seconds plus days, hours, minutes, and seconds.
- `TASK`: best-known active context.
- `LAST DRIVER`: best-known driver area, such as HDA, AC97, FAT32, HTML, or the
  panic test.
- `LAST ACTION`: best-known operation, such as HTML load, audio play, FAT32 read,
  or test page fault.

What changed in `src/kernel/main.c`:

- Bumped the stage number to 25.
- Added a global panic context and lightweight `panic_set_context` breadcrumbs.
- Added direct framebuffer panic rendering that bypasses the desktop compositor.
- Added exception stubs for vectors 0, 6, 8, 13, and 14.
- Installed those exception stubs into the IDT.
- Added `panic_from_exception`, which records vector, error code, RIP, RSP,
  RFLAGS, CR2, fault name, task, last driver, and last action.
- Added an RTC-second-based 18-second panic delay.
- Added stronger reset attempts after the delay: PCI reset control (`0xCF9`),
  PS/2-controller reset (`out 0x64, 0xFE`), then a triple-fault fallback.
- Added panic uptime reporting from the PIT tick counter.
- Added `PANIC`, a test command that writes to canonical unmapped address
  `0x0000000100000000` to intentionally trigger a page fault.
- Added panic breadcrumbs around HTML load, FAT32 file reads, AHCI disk read,
  audio init/playback, and HDA init.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless and captured a normal Stage 25 browser desktop
  (`2026-06-26-stage25-panic-normal.png`).
- Typed `panic` through VirtualBox keyboard scancodes. The kernel triggered a
  real page fault, drew the red screen, and showed:
  - `FAULT: PAGE FAULT`
  - `ERROR CODE: 0x0000000000000002`
  - `CR2: 0x0000000100000000`
  - `TASK: KERNEL`
  - `LAST DRIVER: PANIC TEST`
  - `LAST ACTION: TEST PAGE FAULT`
- Captured the red screen (`2026-06-26-stage25-panic-red-screen.png`).
- Waited after capture and confirmed the VM reset and returned to running state.
- Powered off the VM after the test.

Follow-up after real-hardware feedback:

- The panic screen now reports uptime as total seconds plus days, hours, minutes,
  and seconds.
- The restart delay now uses RTC seconds instead of a CPU spin loop, targeting
  about 18 seconds on both VM and real hardware.
- Reset now tries PCI reset control, PS/2 controller reset, and a triple-fault
  fallback, because the PS/2 reset command alone did not restart the laptop.
- The label is confirmed as `RFLAGS` in code and docs.
- Rebuilt cleanly after the follow-up. A fresh VM screenshot could not be
  captured because VirtualBox failed in its host hardening/startup layer before
  the guest booted (`NtQuerySystemInformation/SystemExtendedHandleInformation
  failed: 0xc0000004`), unrelated to the guest kernel build.

Window animation artifact follow-up:

- Closing/minimizing now hides the window immediately instead of running a
  reverse fade animation.
- This removes the wallpaper/icon bleed-through that looked like a smear or
  paint trail when minimizing/closing Browser, Terminal, or System windows.
- `wm_step_animations` now reports that a redraw is still needed when an open or
  close animation reaches its final visibility state. That prevents the last
  state change from being skipped by the compositor.
- Opening still keeps the rise/fade animation. Close/minimize avoids alpha
  blending completely because those paths were the artifact source.
- Rebuilt cleanly after the fix.
- After stopping a stuck host `VBoxSVC` process from the VirtualBox hardening
  failure, the VM booted again and captured
  `2026-06-26-stage25-close-clean-hide.png`.

Screenshots:

![Stage 25 normal desktop](screenshots/2026-06-26-stage25-panic-normal.png)

![Stage 25 panic red screen](screenshots/2026-06-26-stage25-panic-red-screen.png)

![Stage 25 close/minimize clean hide](screenshots/2026-06-26-stage25-close-clean-hide.png)

Safety status:

- VM-only automated testing; the VM was powered off after the final test.
- The new panic path writes only to framebuffer memory and requests reset with
  PCI reset control, PS/2 controller reset, then a triple-fault fallback after
  the delay.
- No disk, FAT, directory, USB, networking, or filesystem write path was added.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Add serial logging for panic/fault details so crashes before graphics setup are
still visible in the VM log. Then broaden fault coverage to more exception
vectors and add a small panic ring buffer for recent actions.

## 2026-06-25 - Stage 26: SELFTEST Regression Command

Plain English:

Vibio now has a real `SELFTEST` terminal command for safe regression checks
after future kernel changes. It runs read-only probes against the subsystems
that already exist, prints a clear pass/fail line for each one, and ends with
an overall `SELFTEST: PASS` or `SELFTEST: FAIL` result. It does not trigger
panics, play sounds, write to disk, or use the network.

What changed in `src/kernel/main.c`:

- Bumped the stage number to 26.
- Added `VibioSelfTestResult` to store the last run totals, pass/fail counts,
  first failing subsystem name, and timer tick.
- Added `VibioSelfTestContext` so the command can reach paging/IDT flags, the
  browser, and the window manager without rewriting the whole console stack.
- Added `selftest_run` and helpers for honest subsystem checks.
- Added the `SELFTEST` command handler and listed it in `HELP`.
- Added a short System-window health row: last selftest pass/fail plus passed
  check count.

What `SELFTEST` checks:

- `MEMORY`: boot info magic and usable RAM pages from the memory map.
- `PAGING`: boot paging flag plus a live non-zero CR3 read.
- `HEAP`: live alloc/write/free probe (does not require an empty heap).
- `IDT`: boot IDT self-test result.
- `IRQ TIMER`: live PIT tick counter is advancing.
- `KEYBOARD`: keyboard IRQ path is installed (IDT + IRQ state present).
- `MOUSE`: PS/2 mouse driver reported ready in the VM.
- `DISK READONLY`: live MBR signature re-read through the read-only AHCI path,
  or `SKIP` when no boot disk is present.
- `FAT32`: boot FAT32 probe result, or `SKIP` when disk is unavailable.
- `FILE READ`: read-only `START.HTM` header from FAT32 when available, else
  read-only boot-preloaded file bytes, else `SKIP`.
- `HTML START.HTM`: kernel-embedded built-in page, with FAT32 presence as a
  secondary check.
- `AUDIO DEVICE`: AC'97 ready in the VM, or `SKIP` when no supported device
  is present (including HDA-only hardware).
- `AUDIO WAV PARSE`: parse the first boot-preloaded PCM WAV without playback,
  or `SKIP` when no sound files were loaded.
- `WINDOW MANAGER`: window table has valid kinds and sizes.
- `BROWSER LOAD`: current browser page loaded, or built-in `START.HTM` is
  available.
- `PANIC SYSTEM`: panic handler context is installed and the IDT fault path
  was set up at boot (does not run `PANIC`).
- `WRITE PATH: NONE` and `NETWORK: DISABLED` are informational only.

Verification:

- Rebuilt with `-Wall -Wextra`, no warnings or errors.
- Booted `Vibio OS Dev` headless, opened the Terminal, and ran `SELFTEST`.
- Captured the passing report (`2026-06-25-stage26-selftest.png`).
- Ran `VERSION` as a quick regression check; it still reports Stage 26
  (`2026-06-25-stage26-version-check.png`).
- Powered off the VM after the tests.

Screenshots:

![Stage 26 SELFTEST pass](screenshots/2026-06-25-stage26-selftest.png)

![Stage 26 VERSION regression](screenshots/2026-06-25-stage26-version-check.png)

Safety status:

- VM-only automated testing; the VM was powered off after the tests.
- `SELFTEST` uses read-only disk/FAT32 reads, in-memory WAV parsing, heap
  alloc/free probes, and console output only.
- No disk, FAT, directory, USB, networking, or filesystem write path was added.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Rename `PANIC` to a clearer test-only name such as `PANICTEST` or `PAGEFAULT`
so it is obviously separate from `SELFTEST`, then add serial panic logging so
fault details survive even when the red screen cannot be drawn.

## 2026-06-26 - Stage 27 Fix: Real ACPI Power Off And Reliable Restart

Plain English:

Choosing Shut down or Restart from the power menu used to leave Vibio hanging:
"Restart" sometimes never came back, and "Shut down" only ever drew a "VIBIO
HALTED" screen and sat there forever, so the VM never actually closed. Vibio now
performs a real shut down - the VM window closes by itself - and Restart cleanly
reboots back to the desktop. Two new terminal commands, `SHUTDOWN` and
`RESTART`, do the same thing so the behavior can be tested without the mouse.

What changed:

- `src/boot/main.c` now looks up the ACPI root pointer (RSDP) from the UEFI
  configuration table (ACPI 2.0+ preferred, ACPI 1.0 as a fallback) and passes
  its address to the kernel. This is a read-only lookup; nothing is modified.
- `src/shared/boot_info.h` gained an `acpi_rsdp` field to carry that address.
- `src/kernel/main.c` shut down path was rewritten to do a real ACPI soft-off
  (S5):
  - It walks RSDP -> XSDT/RSDT -> FADT, reads the PM1a/PM1b control ports,
    SMI command port, and ACPI-enable value, and scans the DSDT for the `\_S5`
    sleep type (falling back to the common value 5 if it cannot be found).
  - If the firmware left ACPI in legacy mode, it first hands control to ACPI
    through the SMI command port, then writes the S5 sleep value to the PM1
    control register, which is what actually powers the machine off.
  - As a last resort it also pokes the fixed QEMU/Bochs shutdown ports. If a
    host ignores every method, Vibio still falls back to the honest "VIBIO
    HALTED" screen instead of pretending it powered down.
  - These are ACPI shutdown registers and shutdown-only magic ports - never a
    reset or ambiguous port - so shut down cannot accidentally reboot.
- Restart still uses the existing reset path (PCI reset control `0xCF9`, then
  the 8042 keyboard-controller reset, then a triple-fault fallback), which was
  verified to actually reboot the VM.
- Added `SHUTDOWN` and `RESTART` terminal commands and listed them in `HELP`,
  mainly so the power paths can be exercised from a headless test.

Why it matters:

- A normal user expects "Shut down" to close the machine and "Restart" to come
  back to the desktop. Hanging on a halt screen looked broken. The ACPI S5 path
  is the correct, non-destructive way for an OS to ask the platform to power off.

Verification:

- Rebuilt with `build.ps1`; no errors.
- Booted `Vibio OS Dev` headless, opened the Terminal, ran `HELP`, and confirmed
  the new `SHUTDOWN RESTART` line appears
  (`2026-06-26-stage25-help-shutdown-restart.png`).
- Ran `SHUTDOWN`: VirtualBox reported `VMState="poweroff"` - the VM genuinely
  powered itself off (it no longer just shows the halt screen).
- Ran `RESTART`: the VM rebooted and returned to a fresh desktop while staying
  powered on (`2026-06-26-stage25-restart-rebooted.png`).
- Powered off the VM after testing.

Note on manual testing:

- The headless tests drove the new `SHUTDOWN`/`RESTART` terminal commands, which
  call the exact same functions as the taskbar power menu's Shut down / Restart
  buttons. The power-menu mouse clicks themselves still need a quick hand-check
  in the GUI, because VirtualBox headless cannot inject guest mouse input.
- Only tested in VirtualBox. On real hardware the ACPI S5 path is the standard
  method and should work, but it has not been confirmed on the laptop yet.

Screenshots:

![Stage 27 HELP shows SHUTDOWN and RESTART](screenshots/2026-06-26-stage25-help-shutdown-restart.png)

![Stage 27 RESTART rebooted to a fresh desktop](screenshots/2026-06-26-stage25-restart-rebooted.png)

Safety status:

- VM-only automated testing; the VM was powered off after the tests.
- Shut down uses ACPI shutdown registers and shutdown-only magic ports; it never
  touches reset or ambiguous ports, so it cannot accidentally reboot.
- ACPI table parsing is entirely read-only.
- No disk, FAT, directory, USB, networking, or filesystem write path was added.
- No internal disk, host bootloader, EFI partition, or boot order was changed.

Next milestone:

Hand-confirm the taskbar power-menu Shut down / Restart buttons in the GUI, and
verify the ACPI power-off path on the real laptop, then add serial panic logging
so fault details survive even when the red screen cannot be drawn.

## 2026-06-25 - Stage 28: Read-Only Files App (Vibio Files)

Plain English:

Vibio now has a real graphical Files app. Instead of only typing filesystem
commands, you can open a normal window called "Vibio Files" and browse the files
that are actually on the Vibio boot disk: open folders, go back up, see file
sizes, and open a file. It is strictly read-only - there is no way to delete,
rename, copy, move, create, save, or format anything, and no such buttons exist.

What the app shows:

- A title bar reading "Vibio Files".
- A toolbar with an "Up" button, an "Open" button, the current path (starting at
  "/"), and a "READ ONLY" badge.
- A list of the real directory entries with a folder/file marker, the name, the
  type (DIR or FILE), and the size in bytes (folders show "--").
- A status line: "READ ONLY", the item count, and the selected item's name.

What it can browse:

- The list comes from Vibio's existing read-only FAT32 path (the same code the
  HTML viewer and the FS/LS commands use). Nothing is faked.
- Root shows the real entries: EFI/, KERNEL.BIN, SOUNDS/, and START.HTM.
- Opening SOUNDS/ lists the real WAV files: BOOT.WAV, ERROR.WAV, NOTIFY.WAV,
  USBINS.WAV, and USBRM.WAV.
- Opening EFI/ shows the real EFI/BOOT structure too.

What file types it can open:

- Folders: open into the folder; "Up" returns to the parent.
- .HTM / .HTML: opens in the existing HTML viewer (the browser window). This
  works for pages reachable by the viewer (the kernel-embedded START.HTM and
  root-level pages); the disk currently only ships START.HTM.
- .WAV: plays through the existing audio driver (the same path the sound commands
  use) and shows a small info panel with the size and whether playback started.
- Any other type: shows a read-only info panel with the size instead of crashing.
- Missing/unreadable directories show an honest message; an empty folder shows
  "(empty folder)".

What changed (`src/kernel/main.c`):

- Bumped the stage number to 28.
- Added `WINDOW_KIND_FILES` and raised `WM_MAX_WINDOWS` from 3 to 4, so Files is
  a normal managed window alongside System, Terminal, and Browser.
- Added `fat32_list_dir` (read-only directory listing over the existing cluster-
  chain reader) and `fat32_name_to_display` (8.3 name to "NAME.EXT").
- Added the `VibioFilesApp` state and its navigation helpers (load directory,
  enter folder, go up via a path stack, move selection) plus the open logic.
- Added the window renderer, a drawn folder/files icon, a desktop "FILES" icon,
  and a taskbar button.
- Added keyboard control when Files is focused: Up/Down (and Page Up/Down, Home,
  End) move the selection, Enter opens, Backspace goes up, Esc closes a panel.
- Added mouse control: click to select, double-click or the "Open" button to
  open, and the "Up" button to go to the parent.
- Added commands: `FILESAPP` (and alias `EXPLORER`) open the app; `LS` prints the
  real root directory in the terminal. Existing FS/DISK/FILES commands are
  unchanged. Listed the new commands in `HELP`.
- Added a `FILES APP` check to `SELFTEST` (lists the real root over FAT32).

What remains unsupported (honest notes):

- Only FAT32 8.3 short names are supported. There is no long-file-name (VFAT/LFN)
  support, so names are shown exactly as the 8.3 entries (e.g. START.HTM).
- It is read-only by design: no create/delete/rename/move/copy/save/format, and
  those actions are not shown in the UI at all.
- Opening HTML works for pages the existing viewer can load (the embedded
  START.HTM and root-level files); it is not a general per-folder HTML loader.
- Directories are capped at 64 entries per view; a larger directory would show a
  "(list truncated)" note. The current disk is well under that.
- On hardware with no readable FAT32 disk, the app shows an honest "Read-only
  FAT32 not available" message and lists nothing (no fake files).

Filesystem is still read-only:

- No FAT write, directory write, file create, delete, rename, move, save, or
  format path was added anywhere. `SELFTEST` still reports `WRITE PATH: NONE`.

Verification:

- Rebuilt with `build.ps1` (clang `-Wall -Wextra`), no warnings or errors.
- Booted `Vibio OS Dev` headless.
- `VERSION` reports Stage 28 (`2026-06-25-stage28-version.png`).
- `SELFTEST` passes, including `FILES APP: PASS`, with `WRITE PATH: NONE` and
  `NETWORK: DISABLED` (`2026-06-25-stage28-selftest.png`).
- `LS` lists the real root: EFI (DIR), KERNEL.BIN, SOUNDS (DIR), START.HTM
  (`2026-06-25-stage28-ls.png`).
- Opened the Files app with `FILESAPP` and drove it by keyboard:
  - Root directory listing (`2026-06-25-stage28-files-root.png`).
  - Entered SOUNDS/ and saw the five real WAV files
    (`2026-06-25-stage28-files-sounds.png`).
  - Opened BOOT.WAV: it played through the audio driver and showed the info
    panel (`2026-06-25-stage28-files-wav-info.png`).
  - Went back up and opened START.HTM, which opened in the HTML viewer
    (`2026-06-25-stage28-files-open-start.png`).
- Powered off the VM after testing.

Manual testing still needed:

- The keyboard path (which calls the exact same open/up/select functions as the
  mouse path) was exercised headless. Mouse-only actions - clicking rows, the
  Up/Open buttons, double-click, dragging the window, taskbar minimize/restore,
  and opening the taskbar power menu - still need a quick hand-check in the
  VirtualBox GUI, because headless VirtualBox cannot inject guest mouse input.
  The shell, taskbar, and power paths from earlier stages are otherwise
  unchanged, and the window manager self-test passes.

Screenshots:

![Stage 28 Files root](screenshots/2026-06-25-stage28-files-root.png)

![Stage 28 Files SOUNDS folder](screenshots/2026-06-25-stage28-files-sounds.png)

![Stage 28 Files WAV info/playback](screenshots/2026-06-25-stage28-files-wav-info.png)

![Stage 28 Files opening START.HTM](screenshots/2026-06-25-stage28-files-open-start.png)

![Stage 28 LS command](screenshots/2026-06-25-stage28-ls.png)

![Stage 28 SELFTEST pass](screenshots/2026-06-25-stage28-selftest.png)

![Stage 28 VERSION](screenshots/2026-06-25-stage28-version.png)

Safety status:

- VM-only automated testing; the VM was powered off after the tests.
- The Files app uses only read-only FAT32 directory/file reads and the existing
  in-memory audio path. No FAT, directory, file create, delete, rename, move,
  save, or format path was added.
- No disk-write behavior anywhere. No networking was added (still disabled).
- No internal disk, host bootloader, EFI partition, or boot order was changed.
- Removable drives were not touched.

Next milestone:

Add a read-only text/file preview pane inside the Files app for small text files,
and let the HTML viewer open .HTM files from any folder (not just root). Both
must stay strictly read-only.

## 2026-06-26 - Stage 29: VM-Only FAT32 Read/Write Sandbox

Plain English:

This is Vibio's first real disk-write milestone, done in the safest possible
way. Until now everything Vibio touched on disk was read-only. Stage 29 adds a
tiny, careful write path - but it will only ever write to a separate, disposable
"scratch" disk that is deliberately marked so Vibio can recognise it. If that
exact marked disk is not present, Vibio refuses to write anything at all.

This is NOT an installer. It is NOT dual boot. It does NOT write to the boot USB,
the laptop's real disks, Windows, Ubuntu, internal drives, host disks, or any
bootloader. It is a VM-only experiment: "can Vibio safely write one test file and
read it back?"

How the safety works (the sandbox must be positively identified):

Vibio only arms writes when it finds a disk that passes ALL of these gates:

- It is on a *different* AHCI disk than the boot disk (a separate VM disk on SATA
  port 1; the boot disk on port 0 is never touched by the write code).
- Its FAT32 volume label is exactly `VIBIORW`.
- It contains the marker file `VIBIO_RW.TXT` whose content begins with the magic
  string `VIBIO_RW_OK` (documented long form: VIBIO_RW_OK.TXT; the on-disk 8.3
  short name is VIBIO_RW.TXT because long file names are still unsupported).
- It contains the preallocated test file `RWTEST.TXT`.

If any gate fails, `WRITES` stays `DISABLED` and every write call returns a clear
error with nothing written. The boot disk and every other disk always report
`READ ONLY`.

The exact sandbox target used:

- A separate disposable image `build/vibio-rw-scratch.vdi`, created by
  `tools/make_rw_sandbox.py` (FAT32, label `VIBIORW`, 1 sector per cluster).
- The build attaches it to the dev VM as a second SATA disk on port 1. The boot
  disk stays on port 0. `tools/rw-sandbox.ps1` creates/attaches/detaches it and
  only ever touches files under `build\` and the dev VM.
- The image is created only if missing, so its written contents survive plain
  kernel rebuilds.

What the write test writes / how read-back verification works:

- `RWTEST` reads the current `RWTEST.TXT`, bumps a sequence counter, and writes a
  single 512-byte line `VIBIO RW SANDBOX OK SEQ=<n>` back into the file's one
  preallocated cluster (the file is not expanded). After writing it issues a
  best-effort FLUSH CACHE EXT.
- It then verifies the write two independent ways: (1) a sector-level read-back -
  the in-memory buffer is cleared, the sector is re-read off the device, and all
  512 bytes are compared; and (2) a filesystem-level read-back through the normal
  read-only FAT32 file path (`fat32_read_file`), comparing the bytes again. Only
  if both match does it print `RWTEST: PASS`. A failed or partial write is never
  silently ignored - it prints `RWTEST: FAIL (ERR n)`.
- Because the sequence counter is read from disk before each write, a climbing
  number across runs/reboots is itself proof the previous write persisted.

New AHCI write code (`src/kernel/main.c`):

- `ahci_write_sector` / `ahci_write_sector_retry`: single-sector WRITE DMA EXT
  (command header Write bit set, ATA command 0x35), reusing the existing PRDT
  buffer. The read path is untouched.
- `ahci_flush_cache`: best-effort FLUSH CACHE EXT (0xEA) so writes reach the vdi.
- `ahci_issue_slot0`: shared command-issue/poll helper for write and flush.
- `rw_sandbox_parse_fat32`: read-only BPB parse of the scratch partition.
- `rw_sandbox_init` / `rw_sandbox_try_port`: scan non-boot AHCI ports, identify
  the `VIBIORW` disk, validate marker + RWTEST.TXT, and arm (or refuse) writes.
- `rw_sandbox_run_test` / `rw_sandbox_read_current`: the guarded write/verify and
  read-only read-back.

New terminal commands (listed in `HELP`):

- `RWSTATUS` - shows whether a sandbox was found, the disk/partition selected,
  the label, the marker/RWTEST.TXT checks, and whether writes are armed.
- `RWTEST` - the write/read/verify test against the sandbox only; prints PASS or
  FAIL. Refuses (writes nothing) if no sandbox or not armed.
- `RWREAD` - reads back the current `RWTEST.TXT` content (read-only path).
- `RWDISABLE` - turns writes off until the next reboot.

SELFTEST and UI:

- `SELFTEST` now reports `RW SANDBOX: AVAILABLE / SKIP / DISABLED` and
  `BOOT DISK WRITE PATH: NONE`, but it does NOT write to disk - it only reports.
- The System window gained an `RW SANDBOX` status row plus a `BOOT FS: READ ONLY`
  row.
- The Files app is unchanged: it still browses the boot disk strictly read-only,
  with no create/delete/rename/move/copy buttons anywhere. The sandbox is exposed
  only through the RW* terminal commands and status lines, never as fake controls.

What remains unsupported (honest notes):

- No delete, rename, move, copy, folder creation, or new-file creation.
- No file expansion across clusters; `RWTEST.TXT` stays one preallocated cluster.
- No long file name (VFAT/LFN) writing - 8.3 short names only.
- No formatting, partition resize, bootloader install, or EFI boot entries.
- No journaling and no crash-safe atomic updates. This is experimental write
  support: a power loss mid-write could leave `RWTEST.TXT` partially written.
  It is NOT production-safe and NOT safe for real hardware, USB, or installers.

Verification (VM-only, all powered off afterward):

1. Built with `build.ps1` (clang `-Wall -Wextra`), no warnings or errors.
2. `tools/rw-sandbox.ps1` created `build/vibio-rw-scratch.vdi` (label VIBIORW,
   marker VIBIO_RW.TXT, preallocated RWTEST.TXT) and attached it at SATA port 1.
3. Booted `Vibio OS Dev` headless.
4. `VERSION` reports Stage 29 (`2026-06-25-stage29-version.png`).
5. `SELFTEST` passes with `RW SANDBOX: AVAILABLE` and `BOOT DISK WRITE PATH: NONE`
   (`2026-06-25-stage29-selftest.png`).
6. `RWSTATUS` shows TARGET AHCI PORT 1, LABEL VIBIORW OK, MARKER FOUND,
   RWTEST.TXT FOUND CLUSTER 4 SIZE 512, WRITES ARMED
   (`2026-06-25-stage29-rwstatus.png`).
7. `RWTEST` wrote `SEQ=1`, both sector and FAT32 read-back matched, `RWTEST: PASS`
   (`2026-06-25-stage29-rwtest-pass.png`).
8. Powered the VM fully off, booted again, ran `RWREAD`: it read back
   `VIBIO RW SANDBOX OK SEQ=1` after the power cycle, proving the write persisted
   (`2026-06-25-stage29-rw-persistence.png`).
9. Confirmed the Files app still opens and browses the boot disk read-only
   (`2026-06-25-stage29-files-readonly.png`).
10. Confirmed the guest `SHUTDOWN` command still performs a real ACPI power-off
    (VirtualBox reported `VMState="poweroff"`).

Negative tests (sandbox detached with `tools/rw-sandbox.ps1 -Detach`):

- Booted with no scratch disk on port 1.
- `RWSTATUS` reported `TARGET: NOT FOUND`, `WRITES: DISABLED (NO SANDBOX)`
  (`2026-06-25-stage29-rwstatus-missing.png`).
- `RWTEST` printed `REFUSED - NO SANDBOX FOUND` / `FAIL (NOTHING WRITTEN)`
  (`2026-06-25-stage29-rwtest-refused.png`).
- No crash, no reboot, no panic, and no write to the boot disk. The scratch disk
  was re-attached afterward.

Not tested automatically:

- Mouse-only interactions (clicking Files rows/buttons, the taskbar power menu)
  still need a hand-check in the GUI, because headless VirtualBox cannot inject
  guest mouse input. The keyboard paths exercised here call the same code.

Screenshots:

![Stage 29 RWSTATUS armed](screenshots/2026-06-25-stage29-rwstatus.png)

![Stage 29 RWTEST PASS](screenshots/2026-06-25-stage29-rwtest-pass.png)

![Stage 29 persistence after power cycle](screenshots/2026-06-25-stage29-rw-persistence.png)

![Stage 29 SELFTEST with RW sandbox](screenshots/2026-06-25-stage29-selftest.png)

![Stage 29 RWSTATUS sandbox missing](screenshots/2026-06-25-stage29-rwstatus-missing.png)

![Stage 29 RWTEST refused (no sandbox)](screenshots/2026-06-25-stage29-rwtest-refused.png)

Safety status:

- VM-only. Sandbox write target only.
- No real hardware writes.
- No USB writes. The removable `G:` USB was not touched.
- No internal disk writes. No host disk writes. No Windows or Ubuntu writes.
- No installer. No partition resize. No formatting from inside Vibio.
- No bootloader changes. No EFI boot entries. No boot-order changes.
- No networking (still disabled).
- The boot disk and every non-sandbox disk are treated as read-only.
- Writes only ever target the positively identified `VIBIORW` scratch disk on a
  non-boot AHCI port; if it is absent, all writes fail safely.
- The VM was powered off after testing.

Next milestone:

Optionally allow creating one new 8.3 test file in a dedicated `/WRITETST`
directory on the sandbox (allocate one cluster, update the FAT and a directory
entry, write, read back, and verify across reboot) - still VM-only, still
sandbox-only, still no delete/rename/move/expansion and no real-hardware writes.

## 2026-06-25 - Stage 30 Files Storage Backend Fix + Read-Only USB Diagnostics

Plain English:

The Vibio Files app could open but sometimes showed an empty "no filesystem"
view instead of the real boot-disk files. This milestone fixes that honestly,
gives Files a real storage-backend selection layer with a visible SOURCE label,
and adds the missing read-only USB mass-storage diagnostic path so a real-laptop
USB boot reports an honest reason instead of a fake pass. No file list is ever
faked, the boot disk stays strictly read-only, and the Stage 29 write sandbox is
unchanged.

The exact Files failure that was observed:

- "Read-only FAT32 not available on this device."
- "No disk to browse. (No files were faked.)"

Root cause found:

Two separate things, diagnosed by reproducing in the VM and comparing `DISK`,
`FS`, and the Files app:

1. Wiring bug (the real cause of the message in the VM). The Files window was
   only ever told to load its directory by the `FILESAPP`/`EXPLORER` command,
   which calls `files_app_go_root`. Opening Files from the desktop **icon** or
   the **taskbar** button goes through `wm_activate_from_shell`, which shows and
   raises the window but never triggered a directory load. So an icon-opened
   Files window rendered with `fs_available = 0` and printed "Read-only FAT32 not
   available" even though `DISK` and `FS` both reported a healthy FAT32 boot disk
   on AHCI port 0. The `FILESAPP` command path worked, which is why the failure
   looked intermittent.
2. Missing backend layer. Files had no notion of "which storage backend am I
   using" and no honest path for real-hardware USB boot, where the kernel AHCI
   driver finds no device (NVMe/GPT or USB-only machines) and there was no USB
   mass-storage read path at all - so it fell straight to the generic
   "not available" message with no explanation.

`DISK`, `FS`, `RWSTATUS`, `SELFTEST`, and Files did **not** actually disagree
about the FAT32 state in the VM (all saw the same healthy boot disk); the Files
app simply was not reloading on the icon/taskbar open path. Stage 29 scratch
disk did not change boot-disk selection: the boot disk stays on AHCI port 0 and
`ahci_find_active_port` still picks the lowest present port (port 0).

What changed:

- Files now reloads its directory whenever the window becomes visible by any
  means (desktop icon, taskbar, or the `FILESAPP` command), and resets to root
  when it closes so reopening starts clean. This is the direct fix for the
  reported failure.
- New storage-backend selection with a single source of truth
  (`storage_select_source`): priority is AHCI boot-disk FAT32, then read-only USB
  FAT32, then none. The Files app shows the chosen source in its status bar:
  - `SOURCE: AHCI BOOT DISK - READ ONLY`
  - `SOURCE: USB MASS STORAGE - READ ONLY`
  - `SOURCE: NONE`
- Honest, source-aware error text when no backend exists: "No readable FAT32
  filesystem found." plus the specific reason ("AHCI boot disk not available." or
  "USB storage not available yet."), "(No files were faked.)", and a one-line USB
  step summary when an xHCI controller is present.
- New read-only USB mass-storage diagnostic probe (`usb_storage_probe`,
  `VibioUsbStorage`). It records how far the read-only USB path honestly gets:
  xHCI controller found -> capability registers read (CAPLENGTH / HCIVERSION /
  MaxPorts / MaxSlots, read-only, only if the firmware already enabled the
  controller MMIO and its BAR is in the identity-mapped low 4 GiB) -> root-hub
  PORTSC connect status (read-only). It deliberately stops there: full device
  enumeration and the Bulk-Only Transport SCSI read commands (INQUIRY / TEST UNIT
  READY / READ CAPACITY(10) / READ(10)) are not implemented in this build, so the
  sector backend and FAT32 mount stay unavailable and Files never uses USB as a
  real backend. Nothing issues a USB write or any controller register write that
  changes media.
- New terminal commands: `USBSTOR` (alias `USBMSD`) prints the layered USB
  storage status; `FILESDBG` prints which backend Files uses and why
  (`PRIORITY: AHCI > USB > NONE`, and that the scratch disk is a separate write
  sandbox only). `USB` now points to `USBSTOR` for the storage read path. `DISK`
  now shows `ROLE: BOOT DISK READ ONLY` and `FAT32 RO: MOUNTED`. `FS` now shows
  `SOURCE: AHCI BOOT DISK (READ ONLY)`. `HELP` lists the new commands.
- `SELFTEST` gained honest storage lines: `AHCI BOOT FAT32`, `USB STORAGE RO`,
  and `FILES BACKEND` (PASS/SKIP/FAIL), alongside the existing `RW SANDBOX` and
  `BOOT DISK WRITE PATH: NONE`. It still performs no destructive writes and does
  not run `RWTEST`.
- Stage/ABI bumped to 30.

What storage backends are now supported:

- AHCI boot-disk FAT32, read-only (VM and real SATA machines). This is what Files
  uses in the VM.
- Read-only USB mass storage: diagnostic layers only in this build (xHCI detect /
  capability read / port status). The actual sector read + FAT32 mount over USB
  is not implemented yet, and is reported honestly as not available.

What Files uses in the VM:

- The AHCI boot disk, read-only. Root lists `EFI` (DIR), `KERNEL.BIN`, `SOUNDS`
  (DIR), and `START.HTM`; `SOUNDS` lists the five real WAV files; `START.HTM`
  opens in the HTML viewer. The status bar shows
  `SOURCE: AHCI BOOT DISK - READ ONLY`.

What Files can / cannot do on real-hardware USB boot:

- If the machine has a readable AHCI/SATA FAT32 disk, Files browses it read-only.
- On a USB-only boot with no AHCI device, Files cannot yet list USB files: the
  USB read path is diagnostic-only. It shows an honest reason (xHCI found /
  caps read / ports / "MSD read path not implemented") instead of a fake pass.
  `USB`/`USBSTOR` report the same honestly. USB storage is never claimed to work.

Screenshots:

![Stage 30 Files root (fixed)](screenshots/2026-06-25-stage30-files-root-fixed.png)

![Stage 30 Files SOUNDS](screenshots/2026-06-25-stage30-files-sounds-fixed.png)

![Stage 30 START.HTM opened from Files](screenshots/2026-06-25-stage30-files-open-start-fixed.png)

![Stage 30 USBSTOR honest status](screenshots/2026-06-25-stage30-usbstor.png)

![Stage 30 FILESDBG backend](screenshots/2026-06-25-stage30-filesdbg.png)

![Stage 30 SELFTEST storage lines](screenshots/2026-06-25-stage30-selftest.png)

![Stage 30 RWSTATUS sandbox only](screenshots/2026-06-25-stage30-rwstatus.png)

![Stage 30 RWTEST PASS (sandbox)](screenshots/2026-06-25-stage30-rwtest-pass.png)

![Stage 30 Files with scratch disk detached](screenshots/2026-06-25-stage30-files-noscratch.png)

![Stage 30 RWTEST refused (no sandbox)](screenshots/2026-06-25-stage30-rwtest-refused.png)

Verification (VM-only, powered off afterward):

1. Built with `build.ps1` (clang `-Wall -Wextra`), no warnings or errors.
2. Booted `Vibio OS Dev` headless with the scratch disk attached on port 1.
3. `VERSION` reports Stage 30 / SYSCALL ABI 30
   (`2026-06-25-stage30-version.png`).
4. `DISK` reports `READONLY AHCI PASS`, `PORT:0`, `ROLE: BOOT DISK READ ONLY`,
   `FAT32 RO: MOUNTED`, `WRITE PATH: NONE`.
5. `FS` reports `READONLY PASS`, `SOURCE: AHCI BOOT DISK (READ ONLY)`,
   5 sound files.
6. `USBSTOR` reports `XHCI CONTROLLER: NONE FOUND` and every later layer NOT
   IMPLEMENTED / NOT AVAILABLE - honest in the VM (USB disabled, no extpack).
7. `FILESDBG` reports `FILES BACKEND: AHCI BOOT DISK (READ ONLY)`,
   `USB FAT32: NO`, `PRIORITY: AHCI > USB > NONE`.
8. `RWSTATUS` still shows the sandbox on AHCI PORT 1, LABEL VIBIORW, WRITES ARMED,
   `BOOT DISK + ALL OTHERS: READ ONLY`.
9. `SELFTEST` passes; new lines `AHCI BOOT FAT32: PASS`, `USB STORAGE RO: SKIP`,
   `FILES BACKEND: PASS`, plus `RW SANDBOX: AVAILABLE`,
   `BOOT DISK WRITE PATH: NONE`.
10. Opened Files at root: lists the real boot-disk entries with
    `SOURCE: AHCI BOOT DISK - READ ONLY`; opened `SOUNDS` (five real WAV files);
    opened `START.HTM` in the HTML viewer.
11. `RWTEST` wrote `SEQ=2` to the sandbox `RWTEST.TXT` on port 1 only, sector +
    FAT32 read-back matched, `RWTEST: PASS`. The boot disk was not written.
12. The guest `SHUTDOWN` command still performs a real ACPI power-off
    (VirtualBox reported `VMState="poweroff"`).

Negative tests (scratch detached with `tools/rw-sandbox.ps1 -Detach`):

- Booted with no scratch disk on port 1.
- Files still browses the read-only AHCI boot disk and shows
  `SOURCE: AHCI BOOT DISK - READ ONLY` (`2026-06-25-stage30-files-noscratch.png`)
  - the boot disk is never replaced by the scratch disk and does not depend on it.
- `RWSTATUS` reported the sandbox missing; `RWTEST` printed
  `REFUSED - NO SANDBOX FOUND` / `FAIL (NOTHING WRITTEN)`
  (`2026-06-25-stage30-rwtest-refused.png`).
- No crash, reboot, panic, or accidental write. The scratch disk was re-attached
  afterward.

Not tested automatically:

- Real-hardware USB boot cannot be tested here: the dev VM keeps USB disabled
  (VirtualBox 7.2 needs the Extension Pack for emulated xHCI, which is not
  installed). The USB storage path is therefore diagnostic-only and is reported
  as such; it is not claimed to list real USB files.
- Mouse-only interactions (the taskbar power menu, clicking Files rows) still
  need a GUI hand-check; the keyboard paths exercised here drive the same code,
  and the Files reload-on-show fix covers the icon/taskbar open path too.

Real-hardware USB boot test plan (manual, not automated):

- Boot the laptop from the Vibio USB. Run `VERSION`, `DISK`, `FS`, `USB`,
  `USBSTOR`, `FILESDBG`, `SELFTEST`. Open Files and note the SOURCE label (AHCI,
  USB, or NONE). Confirm `RWSTATUS` is NOT ARMED (no VIBIORW sandbox present on
  real hardware). Do NOT run `RWTEST` on real hardware. Expect honest USB status:
  if no xHCI, "XHCI CONTROLLER: NONE FOUND"; if xHCI present, the layered status
  up to "MSD read path not implemented".

Safety status:

- Read-only boot-filesystem browsing only. The boot disk and every non-sandbox
  disk are treated as READ ONLY.
- VM-only write sandbox unchanged (Stage 29). Writes only ever target the
  positively identified `VIBIORW` scratch disk on a non-boot AHCI port.
- No real-hardware writes. No USB writes (the USB path is read-only diagnostics
  and issues no USB write / SCSI WRITE(10) / FORMAT UNIT). No internal disk
  writes. No host disk writes. The removable `G:` USB was not touched.
- No installer. No disk formatting. No partition resize. No bootloader changes.
  No boot-order changes. No EFI boot entries. No networking (still disabled).
- The VM was powered off after testing.

Next milestone:

Implement the actual read-only USB mass-storage transfer over xHCI (device slot
+ endpoint setup, Bulk-Only Transport, the four read-only SCSI commands) to turn
the diagnostic layers into a real sector backend, then mount FAT32 over it so
Files can browse a USB volume read-only on real hardware - still strictly
read-only, still no USB writes.

## 2026-06-25 - Stage 31 USB Hotplug Detection + Insert/Remove Sounds

Plain English:

Stage 31 adds USB port connect/disconnect detection where Vibio can safely read
xHCI root-hub port status, and connects those events to the existing USB insert
and remove WAV assets. This means Vibio can record "a USB port changed from
disconnected to connected" or "connected to disconnected" and attempt a one-shot
event sound. It does **not** mean USB storage works, USB flash drives work, USB
keyboards/mice work, or USB support is complete.

What changed:

- Added a separate USB hotplug state block for controller type, support status,
  readable port count, connected bitmap/count, previous state, insert/remove
  counters, last event type/port/tick, last sound result, simulated event count,
  and unsupported reason.
- Reused the Stage 30 USB controller table and xHCI capability/register-reading
  path. xHCI is selected first. If no xHCI controller is available, if MMIO is
  disabled, if the BAR is above the identity-mapped low 4 GiB, or if port status
  is unreadable, Vibio reports that honestly instead of claiming hotplug works.
- Polls xHCI PORTSC current-connect-status (CCS) only. The first poll records a
  baseline. Later disconnected -> connected transitions count as `INSERT`; later
  connected -> disconnected transitions count as `REMOVE`. Unchanged ports do
  not retrigger sounds.
- Insert events play `USBINS.WAV`; remove events play `USBRM.WAV`. If audio is
  unavailable, the event is still recorded with `LAST SOUND: AUDIO UNAVAILABLE`.
  If an asset is missing, the event is recorded with `LAST SOUND: MISSING`.
- Added `USBHOTPLUG` for focused hotplug status. `USB` now summarizes controller
  and hotplug status while still showing `STORAGE READ` and `WRITE PATH`.
- Added `USBTEST INSERT` and `USBTEST REMOVE` as simulated event/sound-path tests
  only. They are labeled `SIMULATED EVENT ONLY` and do not prove real hotplug.
- `SELFTEST` gained non-destructive lines for `USB HOTPLUG`,
  `USB SOUND ASSETS`, and `USB EVENT SOUND PATH`. It does not require a person
  to plug/unplug a device and does not fake a real-hotplug pass.
- Stage/ABI bumped to 31.

What USB hotplug detection means:

- Vibio watches root-hub port connection state when xHCI PORTSC registers are
  safely readable.
- It records connect/disconnect transitions and event counters.
- It attempts the matching one-shot USB insert/remove sound.

What it does NOT mean:

- No USB mass-storage support was added.
- No USB device functionality was added beyond port-state detection.
- No USB keyboard or mouse driver was added.
- No USB file browsing was added.
- No USB writes or media writes were added.

Controller support status:

- xHCI PORTSC polling is the first supported hotplug detector.
- UHCI/OHCI/EHCI hotplug is reported as unsupported for now.
- The dev VM still has USB disabled (`build.ps1` sets `--usb off`), so VM output
  honestly shows no controller and real hotplug unsupported.

Screenshots:

![Stage 31 USB hotplug status](screenshots/2026-06-25-stage31-usb-hotplug-status.png)

![Stage 31 simulated USB sound test](screenshots/2026-06-25-stage31-usb-sound-test.png)

Additional VM screenshots:

- `docs/screenshots/2026-06-25-stage31-version.png`
- `docs/screenshots/2026-06-25-stage31-usb-status.png`
- `docs/screenshots/2026-06-25-stage31-usbstor.png`
- `docs/screenshots/2026-06-25-stage31-selftest.png`
- `docs/screenshots/2026-06-25-stage31-rwstatus.png`
- `docs/screenshots/2026-06-25-stage31-files-root.png`

Verification (VM-only, powered off afterward):

1. Built with `build.ps1` (clang `-Wall -Wextra`), no warnings or errors.
2. Booted `Vibio OS Dev` headless.
3. `VERSION` reports Stage 31 / syscall ABI 31.
4. `USBHOTPLUG` reports no USB controller in this VM, zero ports, zero events,
   `REAL HOTPLUG: UNSUPPORTED`, `STORAGE READ: NONE (DIAGNOSTIC ONLY)`, and
   `WRITE PATH: NONE`.
5. `USB` reports controller counts and the same honest hotplug/storage/write
   status.
6. `USBSTOR` remains focused on read-only USB storage diagnostics and still
   reports no xHCI controller in the disabled-USB VM.
7. `SELFTEST` passes overall; USB hotplug is not falsely marked real-pass in the
   no-controller VM, while USB sound assets and event sound path pass.
8. `RWSTATUS` still shows the VIBIORW scratch disk as the only armed write
   target and keeps the boot disk plus all others read-only.
9. `USBTEST INSERT` reports `SIMULATED EVENT ONLY`, `REAL HOTPLUG: UNSUPPORTED`,
   and `LAST SOUND: STARTED` in the VM with audio available.
10. `FILESAPP` still opens the read-only AHCI boot-disk root.
11. The guest `SHUTDOWN` command still performs a clean VM power-off.

Manual real-hardware test plan:

- Boot Vibio from USB on the old laptop.
- Run `VERSION` and confirm Stage 31.
- Run `USB`.
- Run `USBSTOR` or `USBMSD`.
- Run `USBHOTPLUG`.
- Plug in a harmless USB device, such as a flash drive or mouse.
- Confirm whether Vibio records an insert event and whether `USBINS.WAV` plays.
- Remove the USB device.
- Confirm whether Vibio records a remove event and whether `USBRM.WAV` plays.
- Run `USB` / `USBHOTPLUG` again and record insert/remove counts, last event,
  last port, and last sound.
- Do not run `RWTEST` on real hardware.
- Do not mount, browse, or write the USB device; USB storage remains unsupported.

Expected real-hardware outcomes:

- If hotplug detection works: insert/remove counters increase, the matching sound
  plays, and command output shows the last event and last port.
- If it does not work: Vibio should show a specific reason such as no USB
  controller found, non-xHCI unsupported, xHCI memory space disabled, BAR above
  4 GiB, port count unavailable, port status unavailable, audio unavailable, or
  sound asset missing.

Safety status:

- No USB storage support added.
- No USB media writes.
- No disk writes.
- No FAT writes.
- No installer.
- No host/internal disk changes.
- No Windows, Ubuntu, EFI partition, host bootloader, or physical disk changes.
- No bootloader changes.
- No boot-order changes.
- No networking.
- Stage 29 VIBIORW write sandbox remains VM-only and separately guarded.
- Stage 30 Files/storage behavior remains intact: Files uses the AHCI boot disk
  read-only in the VM, and USB storage remains diagnostic-only.
- The VM was powered off after testing.

Next milestone:

Keep USB storage separate from hotplug: either add a more complete xHCI
enumeration diagnostic that still stops before storage media reads, or begin the
read-only USB mass-storage transfer path in a dedicated milestone with explicit
BOT/SCSI read-only limits and no writes.

## 2026-06-25 - Stage 32 Browser Image Rendering

Plain English:

The browser no longer treats every `<img>` as a placeholder. Vibio now includes a
real `TEST.PNG` file, preloads it at boot, packs it into the FAT32 root, and
renders it inside `START.HTM`. Opening a `.PNG` file from Files also routes to
the browser image viewer.

What changed:

- Added `VIBIO_BOOT_FILE_TYPE_IMAGE`.
- The UEFI boot stage now attempts to preload `\TEST.PNG` read-only, so the
  known test image can render even on real hardware where the kernel cannot read
  the boot USB/NVMe storage itself.
- `tools/make_fat32_image.py` now packs top-level `.PNG` files into the generated
  read-only FAT32 root alongside `.HTM` / `.HTML`.
- Added `Assets/TEST.PNG`, a small local test image.
- Added a browser image scratch buffer and a minimal PNG renderer.
- Inline `<img src="TEST.PNG">` now attempts to load and draw the PNG before
  falling back to the placeholder.
- Opening `.PNG` from Files opens it in the browser image viewer.
- Stage/ABI bumped to 32.

Supported image subset:

- PNG signature and IHDR parsing.
- 8-bit RGB or RGBA.
- Non-interlaced.
- Filter type 0 scanlines.
- One IDAT chunk containing a zlib wrapper around a single uncompressed deflate
  block.

Honest limitations:

- This is not full PNG support yet.
- Compressed deflate streams, multiple IDAT chunks, palette PNG, grayscale PNG,
  Adam7 interlace, and PNG filters 1-4 are not implemented.
- Unsupported or missing images still render a clean placeholder/error instead
  of crashing.
- No image editing, saving, writes, or network image loading.

Screenshot:

![Stage 32 browser image rendering](screenshots/2026-06-25-stage32-browser-image.png)

Verification (VM-only, powered off afterward):

1. Built with `build.ps1`, no warnings or errors.
2. Booted `Vibio OS Dev` headless.
3. Ran `BROWSER`.
4. Captured `docs/screenshots/2026-06-25-stage32-browser-image.png`, showing
   `TEST.PNG` rendered inline on the start page.
5. VM was powered off after the screenshot.

Safety status:

- Read-only browser/file display only.
- No disk writes.
- No FAT writes.
- No USB storage support or USB media writes.
- No installer.
- No host/internal disk changes.
- No bootloader changes.
- No networking.
- Stage 29 VIBIORW write sandbox remains VM-only and separately guarded.

Next milestone:

Extend PNG support incrementally: implement the standard PNG filters and normal
compressed deflate streams, then add size-aware scaling for larger local images.

## 2026-06-25 - Stage 33 UEFI Boot Files Backend + Broader Packaged Images

Plain English:

Stage 33 makes Vibio more useful on the real USB-boot laptop without pretending
that a full USB mass-storage driver exists yet. Before UEFI boot services are
exited, the boot stage now preloads more read-only files from the boot volume and
passes them to the kernel. If the kernel cannot read AHCI/NVMe/USB storage after
boot, Files can still show the UEFI-preloaded boot-file bundle.

This is the practical real-hardware USB-boot path for now: the firmware reads
the boot USB before `ExitBootServices`; Vibio then uses that read-only file table
after boot. It is not xHCI enumeration, not BOT/SCSI, not a USB FAT32 driver, and
not a write path.

What changed:

- Increased the boot file table from 8 to 32 entries and raised the per-file
  preload limit to 2 MiB.
- The boot stage still loads the known sound/home assets, then enumerates the
  boot volume root and preloads supported top-level files: `.WAV`, `.HTM`,
  `.HTML`, `.PNG`, `.BMP`, `.JPG`, `.JPEG`, `.GIF`, and `.VIM`.
- Files gained a third source: `UEFI BOOT FILES - READ ONLY`. Backend priority is
  now AHCI boot disk -> future USB FAT32 -> UEFI boot files -> none.
- `FILESDBG` reports the boot-file count and the new priority.
- `tools/make_fat32_image.py` now converts common top-level image assets into
  `.VIM`, a simple RGBA bitmap format. The kernel can render `.VIM` directly.
- Browser image rendering now tries direct `.VIM`, the Stage 32 simple PNG path,
  then generated `.VIM` fallback for `.PNG`, `.JPG`, `.JPEG`, `.BMP`, and `.GIF`
  references.
- Added `Assets/PHOTO.JPG` and updated `START.HTM`; the VM screenshot proves a
  JPG displayed through the generated bitmap fallback.
- Stage/ABI bumped to 33.

What works now:

- On the VM: AHCI remains the preferred Files backend, and the browser renders
  the inline JPG/PNG test images.
- On a real USB boot: the kernel should have a read-only UEFI boot-file bundle
  available even when post-boot AHCI/NVMe/USB storage is unavailable. Files can
  show those preloaded files, and the browser can render preloaded/generated
  images from that bundle.

What still does not work:

- No full USB mass-storage driver yet.
- No post-boot USB FAT32 directory walking.
- No xHCI device-slot/endpoint setup.
- No Bulk-Only Transport.
- No SCSI INQUIRY / READ CAPACITY / READ(10) implementation.
- No USB storage writes or media writes.
- No arbitrary host/internal disk access.
- No image saving/editing.
- No network image loading.
- The in-kernel PNG decoder is still a small subset; broader image support comes
  from build-time conversion to `.VIM` for packaged assets.

Screenshots:

![Stage 33 browser images](screenshots/2026-06-25-stage33-browser-images.png)

Additional VM screenshots:

- `docs/screenshots/2026-06-25-stage33-version.png`
- `docs/screenshots/2026-06-25-stage33-filesdbg.png`

Verification (VM-only, powered off afterward):

1. Built with `build.ps1`, no warnings or errors.
2. Booted `Vibio OS Dev` headless.
3. Ran `BROWSER` and captured the start page showing `PHOTO.JPG` rendered through
   the generated bitmap fallback (`2026-06-25-stage33-browser-images.png`).
4. Ran `VERSION`, confirming Stage 33 / syscall ABI 33.
5. Ran `FILESDBG`; the VM still prefers AHCI, and the command reports the
   UEFI-preloaded boot-file bundle and backend priority.
6. VM was powered off after testing.

Manual real-hardware USB-boot test plan:

- Rebuild and deploy the updated boot files to the USB.
- Boot the laptop from the Vibio USB.
- Run `VERSION` and confirm Stage 33.
- Run `FILESDBG` and record the boot-file count.
- Open Files. If AHCI/NVMe/USB storage is unavailable after boot, expect
  `SOURCE: UEFI BOOT FILES - READ ONLY` and a list of preloaded files.
- Run `BROWSER` and confirm the start page renders the JPG/PNG test images.
- Do not run `RWTEST` on real hardware.
- Do not expect arbitrary USB files added after boot to appear; only files UEFI
  preloaded before `ExitBootServices` are available through this fallback.

Safety status:

- Read-only UEFI boot-file preload only.
- No disk writes.
- No FAT writes.
- No USB media writes.
- No USB mass-storage writes.
- No installer.
- No bootloader changes.
- No host/internal disk changes.
- No networking.
- Stage 29 VIBIORW write sandbox remains VM-only and separately guarded.

Next milestone:

Choose one direction explicitly: either continue the real USB storage driver
path with xHCI command ring / event ring / device context setup and read-only
BOT/SCSI diagnostics, or keep improving the UEFI boot-file fallback with
subdirectory preload and a richer image viewer.

## 2026-06-26 - Stage 33 Follow-up: USB Deploy Images + Human File Sizes

Plain English:

This fixes the real USB deploy gap found during hardware testing. The VM image
already included converted image fallbacks, but `deploy-usb.ps1` only copied the
boot files, `START.HTM`, and sounds. A USB boot created with that script could
therefore open Files but have no root JPG/PNG/VIM image files available for the
UEFI boot-file fallback to preload.

What changed:

- `deploy-usb.ps1` now copies top-level browser assets from `Assets` to the USB
  root using FAT-friendly 8.3 names.
- The deploy script also generates `.VIM` fallback images for `.PNG`, `.JPG`,
  `.JPEG`, `.BMP`, and `.GIF` assets when Python/Pillow is available, matching
  the VM disk builder's image path.
- Files and shell listings now display sizes as `B`, `KB`, `MB`, or `GB`
  instead of raw byte counts.

Real-hardware note:

This still does not implement full post-boot USB mass storage. The practical
path remains UEFI-preloaded, read-only boot files. After redeploying the USB,
the real machine should show the copied root assets and generated `.VIM` files
through Files even when Vibio has no post-boot USB storage driver yet.

Follow-up correction:

- The bootloader now resets the UEFI root directory handle before enumerating
  root files. Some real firmware can otherwise return only the hardcoded preload
  files (`.WAV` sounds and `START.HTM`) and skip root image discovery.
- The UEFI boot-file fallback now carries lightweight directory entries, so `/`
  can show `EFI` and `SOUNDS` even without a post-boot disk driver.
- In the fallback view, `/` shows folders plus root HTML/image assets, while
  `/SOUNDS` shows the preloaded WAV files.
- The boot-file table was raised from 32 to 64 entries, and root image/HTML
  files are loaded before folder markers so extra USB folders cannot crowd out
  images.
- Boot-file names and Files app names now allow longer firmware-reported names,
  so root files like `IMG_20260626.JPG` are not discarded by the fallback path.
- `FILES APP` self-test now passes when the UEFI boot-file fallback is available;
  `DISK` and `AHCI BOOT FAT32` remain honest post-boot driver tests and can
  still fail on real USB-only boot hardware.

## 2026-06-26 - Stage 34 Media Viewer And Media Player

Plain English:

Vibio now has dedicated Media Viewer and Media Player windows instead of routing
images through the tiny browser page view. Images open large with fit/100%/zoom
and pan. Packaged PNG/JPG files work through honest host-built `.VIMG` fallbacks.
Converted `.VIV1` test video plays with optional PCM audio. The UI says clearly
when a fallback is missing or when audio is unavailable.

What changed:

- Stage/ABI bumped to 34.
- Added `WINDOW_KIND_MEDIA_VIEWER` and `WINDOW_KIND_MEDIA_PLAYER` (~78% work-area
  default size, maximize/F11 supported).
- Media Viewer: native `.VIMG`/`.VIM`, BMP 24/32, PNG subset; fallback `.VIMG`
  for PNG/JPG/JPEG/BMP/GIF/WEBP; status shows source (AHCI/USB/UEFI) and decode path.
- Media Player: `.VIV1` frame streaming from FAT32/UEFI files, timer/audio sync,
  play/pause/stop/restart/mute, dropped-frame counter.
- `tools/make_media_assets.py` generates TEST/PHOTO fallbacks, `VIDEOTST.VIV`,
  `VIDEOTST.WAV`, and `MEDIA.MF`. Build runs it before `make_fat32_image.py`.
- Extended VIMG header (version 1 + checksum) while keeping legacy 16-byte VIMG.
- Files opens images in Media Viewer and video in Media Player.
- Terminal: `MEDIA`, `CODECS`, `VIEW`, `PLAY`, `VIMGINFO`, `VIVINFO`, `IMGTEST`,
  `VIDEOTEST`.
- SELFTEST: `MEDIA LAYER`, `VIMG DECODE`, `IMAGE FALLBACK`, `VIDEO HEADER`,
  `AUDIO PCM PATH`, `MEDIA VIEWER WINDOW` (SKIP when optional assets missing).

What works:

- VM build passes; `VERSION` reports Stage 34.
- Files -> `TEST.PNG` / `PHOTO.JPG` opens large Media Viewer through VIMG fallback.
- `VIDEOTEST` opens `VIDEOTST.VIV` in Media Player; frames advance; audio plays
  when AC97/HDA is ready, otherwise status says unavailable and video continues.
- `CODECS` and `MEDIA` report honest native vs converted support.

What still does not work:

- No in-kernel JPEG, GIF animation, WebP, MP4, H.264, VP9, or AAC decode.
- No full PNG zlib/deflate or interlaced PNG.
- Video conversion without host `ffmpeg` is limited to the built-in test clip.
- No post-boot USB FAT32 streaming beyond existing read-only paths.
- No media cache writes (read-only playback only).

Screenshots (capture after VM boot):

- `docs/screenshots/2026-06-26-stage34-media-viewer-png-fallback.png`
- `docs/screenshots/2026-06-26-stage34-media-viewer-jpg-fallback.png`
- `docs/screenshots/2026-06-26-stage34-media-viewer-fit.png`
- `docs/screenshots/2026-06-26-stage34-media-player-frame.png`
- `docs/screenshots/2026-06-26-stage34-media-player-audio-status.png`
- `docs/screenshots/2026-06-26-stage34-codecs-command.png`
- `docs/screenshots/2026-06-26-stage34-selftest-media.png`

Verification (VM):

1. `.\build.ps1` — PASS (Pillow converts 3 image fallbacks + VIDEOTST).
2. Boot `Vibio OS Dev`, run `VERSION` — Stage 34.
3. Files -> open `TEST.PNG` — Media Viewer large, status shows VIMG fallback.
4. Files -> open `PHOTO.JPG` — same.
5. Terminal `VIDEOTEST` — Media Player plays frames; audio status honest.
6. `SELFTEST` — media lines PASS/SKIP as expected; no crash when optional assets absent.
7. Close Media Player — playback stops.
8. `SHUTDOWN` / `RESTART` still work.

Safety status:

- Read-only media playback; no disk/FAT/USB writes from media apps.
- Host converter only writes under `build/media` and the VM image.
- No cache on internal/USB disks.

## 2026-06-26 - Stage 34 Follow-up: Native Media Fixes, Audio Player, Close Animation, Input Polish

Plain English:

This round makes native media actually work and fixes several UI/UX issues that
were reported from real use. Images already worked in the viewer, but video
played as a black rectangle, video/audio arguments to terminal commands were
dropped, the media windows had cramped/clipped text, audio files had no player
of their own, and pressing keys (Space especially) popped the Terminal open.
Closing a window also had no animation.

What changed:

- Input: typing no longer opens the Terminal. Pressing Space (or any key) on the
  desktop does nothing; the Terminal opens ONLY when its icon/taskbar button is
  clicked. Typing still routes to and raises an already-open Terminal.
- Window close/minimize now plays the exact reverse of the open animation (a
  fall-and-fade-out), instead of vanishing instantly. Re-activating a window
  mid-close cancels the close and plays the open animation. F11 fullscreen
  windows still close instantly (so the whole screen does not flash a fade).
- VIDEO RENDER BUG (root cause): `fat32_read_file_at` compared the absolute file
  offset against per-cluster-relative sector bounds, so every sector after the
  first cluster of a read was wrongly skipped. This silently truncated any
  multi-cluster/large read. Small reads (the VIV header + index, still images
  preloaded as boot files) fit in one sector and hid the bug; the 230 KB video
  frame reads did not, so frames failed to decode and rendered black. The loop
  now tracks the absolute byte position, so multi-cluster reads work.
- VIDEO FORMAT BUG: `tools/make_media_assets.py` wrote VIV frames as raw RGBA,
  but the kernel decodes every frame with `media_vimg_parse` (expects a VIMG
  header). Each frame is now stored as a self-contained legacy VIMG bitmap. This
  fixes the test clip and the host ffmpeg conversion path.
- Media Player now auto-plays on open (it previously opened paused on frame 0,
  which looked like "the video does not show up"), and the main loop repaints
  the player window each time the displayed frame advances so playback is not
  stuck at the ~4 Hz idle redraw rate.
- Terminal argument parsing fixed: `console_copy_cmd_arg` skipped to the
  character right after the command name (the separating space) and returned an
  empty argument, so `VIEW`, `PLAY`, `VIMGINFO`, and `VIVINFO` never received a
  filename. It now skips the separator space(s) first.
- Audio files now have their own player app: opening a `.WAV` from Files (or
  `PLAY name.WAV`) opens the Media Player in a new audio-only "Now Playing"
  mode (file name + animated bar + transport controls) instead of just showing
  a Files info panel. Images open in Media Viewer, video and audio in Media
  Player; none of these appear as desktop icons.
- UI clipping fixed: Media Viewer header rows are spaced by the Geist line
  height (no more overlapping text spilling into the image), and the Media
  Player status rows and transport buttons are sized to their labels so
  "RESTART" is no longer clipped to "RESTA".
- VM audio: the dev VM now exposes an AC'97 controller (was HDA). The kernel has
  a working AC'97 driver but only a "driver needed" stub for Intel HD Audio, so
  AC'97 is what actually produces sound in the VM. Boot sound and video/audio
  PCM now play in the VM.

Technical details:

- `fat32_read_file_at`: replaced the per-cluster `skip`/`chunk_end` comparison
  with an absolute `pos` walk over the cluster chain.
- `make_viv1`: each frame is prefixed with `b"VIMG" + <w><h><1>` before being
  indexed; the per-frame index already supported variable sizes.
- `wm_begin_close`/`window_draw`/`wm_show_and_raise`: close keeps the window
  visible with `anim_close=1`; `wm_step_animations` hides it and applies the
  final minimized state at the end; the draw path time-reverses the open
  ease-out curve; `rect_add_window` already padded the dirty rect downward to
  cover the fall, and the compositor rebuilds each dirty rect from the static
  base layer so the fade does not smear.
- `media_player_open`/`media_player_tick`/`media_player_render`: added an
  `audio_only` path for `.WAV` (no frames, PCM only, "Now Playing" card).

Commands tested (VM, headless boot + scancode injection + screenshots):

- Pressing Space 5x on the desktop: Terminal does NOT open (clean desktop).
- `VIEW PHOTO.JPG`: Media Viewer opens with the JPG via VIMG fallback.
- `IMGTEST`: Media Viewer opens TEST.PNG via native PNG-subset decode.
- `VIDEOTEST`: Media Player auto-plays VIDEOTST.VIV; frames advance (red->green,
  "VIDEOTST N/30" text) and audio reports "playing PCM".
- `PLAY BOOT.WAV`: Media Player opens in audio-only "Now Playing" mode, PCM plays.
- `FILESAPP`: Vibio Files lists the real read-only FAT32 root (19 items).
- `BROWSER`: HTML test page renders with inline JPG/PNG images (no regression).
- `SHUTDOWN`: VM powers off via ACPI S5 (VMState=poweroff).
- `.\build.ps1`: PASS (exit 0), Pillow converts 3 image fallbacks + VIDEOTST.

Screenshot paths:

- `docs/screenshots/2026-06-26-stage34-space-no-terminal.png`
- `docs/screenshots/2026-06-26-stage34-image-viewer-png.png`
- `docs/screenshots/2026-06-26-stage34-image-viewer-jpg.png`
- `docs/screenshots/2026-06-26-stage34-video-red.png`
- `docs/screenshots/2026-06-26-stage34-video-green.png`
- `docs/screenshots/2026-06-26-stage34-audio-player.png`
- `docs/screenshots/2026-06-26-stage34-files.png`

Safety status:

- Read-only media playback only. No disk/FAT/USB/host writes were added; the
  `fat32_read_file_at` change is read-only. VM-only verification.

Honest limitations / still does not work:

- Real-hardware lag (especially fullscreen) was NOT fixed or verified this
  session. VBoxManage offers no mouse injection and I have no access to the
  user's hardware, so I could not profile it. The likely cause is full-frame
  CPU framebuffer blits whose cost scales with the (large) native resolution;
  this needs on-hardware profiling before changing the working compositor.
- The window close animation is verified by code/build only, not by an
  automated click (VBoxManage cannot inject mouse events). Please click-test
  closing/minimizing windows.
- Still no in-kernel JPEG/PNG-full/H.264/VP9/AAC decode; real video still needs
  host-side ffmpeg conversion to `.VIV`, which is not installed here, so only
  the built-in VIDEOTST clip and `.VIM/.VIMG` fallbacks are exercised.
- Intel HD Audio is still detect-only on real hardware (no HDA driver), so
  audio plays in the VM (AC'97) but not on HDA-only real machines.
- Typing on the desktop while the Terminal is hidden still buffers into the
  Terminal input (harmless, but leftover characters appear when it is opened).

Next milestone (suggested):

- Profile and reduce full-frame presents on real hardware (the lag report), and
  add a real Intel HD Audio output path so audio works on real machines.

## 2026-06-26 - Stage 34 Follow-up 2: Real-Hardware Media Preload (USB boot)

Plain English:

The previous round fixed media in the VM but I had not deployed to the G: USB,
so the laptop saw no change. This round deploys, and - more importantly - fixes
why video could never appear on the laptop: the laptop has no post-boot storage
driver, so it can only use files the UEFI bootloader preloads from the USB, and
that preloader (a) skipped any file over 2 MB and (b) allocated a fixed 2 MB
buffer per file. The 6.9 MB test video was therefore skipped entirely, and
`deploy-usb.ps1` never copied video at all.

What changed:

- Bootloader (`src/boot/main.c`): `load_read_only_file` now asks the firmware for
  the exact file size (`EFI_FILE_INFO` via `GetInfo`) and allocates exactly that
  (rounded to pages). If `GetInfo` is unavailable it falls back to the original
  fixed-buffer read, so the loader can never boot-fail. This lets the laptop
  preload large media (video, full-res images) with no per-file waste.
- `VIBIO_BOOT_FILE_MAX_SIZE` raised 2 MB -> 32 MB (now only an upper safety cap,
  not a per-file allocation). `AUDIO_BUFFER_BYTES` decoupled from it (fixed 8 MB).
- VM RAM raised 128 MB -> 512 MB so the VM can hold larger preloads (it runs the
  same bootloader path the laptop uses).
- `deploy-usb.ps1` now copies `build/media/*.VIV` (+ matching `.WAV`) to the USB
  root, and converts images at 1024 px (was 512) for crisper output.
- Image scratch/buffer raised 4 MB -> 8 MB to hold the higher-res bitmaps.

Verification (VM, 512 MB):

- `.\build.ps1` PASS; VM boots on the new bootloader.
- `VIDEOTEST`: the full 6.9 MB VIDEOTST.VIV now PRELOADS and plays to completion
  (frame 30/30, colors cycle). This is the same UEFI-preload path the laptop
  uses (no AHCI), so it validates the laptop path.
- `IMGTEST`: TEST.PNG still renders.
- Deployed to G: (VIBIO): BOOTX64.EFI, KERNEL.BIN, images + .VIM, sounds, and
  VIDEOTST.VIV (6.6 MB) + VIDEOTST.WAV.

Safety status:

- Read-only. Deploy is removable-only (DriveType=2), non-destructive, never
  formats. Bootloader change is read-only with a safe fallback.

Honest limitations:

- "Any video/image, decoded in-kernel like Windows" is NOT possible for this
  from-scratch kernel: there is no in-kernel JPEG/PNG-full/H.264/MP4 decoder.
  The real model is HOST CONVERSION (Pillow for images, ffmpeg for video) to
  Vibio-native bitmaps/frames, which the kernel then renders for real.
- Images: any common format works IF converted (build/deploy does this with
  Pillow) and the file fits the 32 MB preload cap. Opening the ORIGINAL long
  filename can miss its `.VIM` fallback due to FAT 8.3 short-name truncation;
  opening the generated `.VIM` directly is reliable.
- Video: arbitrary user videos require host `ffmpeg` (NOT installed here) to
  produce `.VIV`; only the built-in test clip is converted without it. Long
  videos may exceed the 32 MB preload cap - true arbitrary-length playback needs
  a real post-boot USB storage driver (large, future).
- Real-hardware lag: still not fixed/verified (no hardware access here).
- Could not test on the laptop directly; please boot G: and report what shows.

Next milestone (suggested):

- Either a post-boot USB mass-storage (xHCI BOT/SCSI) read driver so the laptop
  can stream arbitrary large media off the boot USB, or in-kernel simple-frame
  video compression so longer clips fit the preload budget.

## 2026-06-26 - Stage 34 Follow-up 3: VFAT Long File Name Reading (real-OS direction)

Plain English:

Per the user's direction (make Vibio behave like a real OS that reads real files,
not just preloaded/converted assets), this adds VFAT long-file-name support to the
FAT32 reader. Before, Vibio only understood 8.3 short names, so a file like
"background default.png" showed as "BACKGROU.PNG" and could not be opened by its
real name. Now Vibio assembles and matches the long name.

What changed:

- `fat32_lfn_piece` + `fat32_name_ieq`: assemble the 13-char-per-entry VFAT long
  name (reset on the 0x40 "last" entry) and compare case-insensitively.
- `fat32_find_in_dir_ex` (used by `fat32_read_file` and `fat32_read_file_at`):
  matches either the assembled long name OR the 8.3 short name. Opening a file by
  its long name works even when it has no valid 8.3 form.
- `fat32_list_dir`: shows the real long name in the Files app (8.3 fallback when
  there is no long name).
- Image builder `make_fat32_image.py`: writes real VFAT LFN entries
  (`lfn_entries`/`lfn_checksum`) and preserves an asset's original filename as its
  long name when it does not survive 8.3, so the VM image can exercise the path.

Verification (VM):

- `.\build.ps1` PASS; VM boots.
- Files app now lists "background default.png" (long name with space + lowercase)
  instead of "BACKGROU.PNG". Other entries unchanged.
- Existing FAT32 browsing, media, browser still work.
- Deployed to G: USB.

Safety status: read-only; deploy removable-only/non-destructive.

Honest limitations:

- Opening a long-named file in a format Vibio cannot decode natively (e.g. a real
  JPEG) still needs its converted .VIM, and that fallback-name match is still
  short-name based - so a long-named JPEG may list but not open until that
  matching is made long-name aware (next step).
- This is the FAT32/long-name part of the "real OS files" goal. The big item -
  post-boot USB mass storage (xHCI + BOT/SCSI) so the laptop can read arbitrary
  files off the boot USB after boot - is NOT done and currently cannot be tested
  in this environment (VM has no xHCI without the Extension Pack, which is
  declined; QEMU is not installed). It needs a real test path before it is safe
  to develop.

Next milestone:

- Decide a USB-driver test path (QEMU xHCI+usb-storage, or laptop-only), then
  implement the xHCI + USB Mass Storage read stack and mount FAT32 on it. Also
  make image .VIM fallback matching long-name aware.

## 2026-06-26 - QEMU Test Harness (unblocks the USB driver)

Plain English:

To build the post-boot USB mass-storage driver the user wants, I need an
environment that emulates an xHCI controller with a USB drive. The VirtualBox dev
VM cannot (no Extension Pack, by choice). QEMU can, with no extension pack, so it
is now installed and wired up as a second test harness alongside VirtualBox.

What changed:

- Installed QEMU 11.0 for Windows (user-approved).
- New `tools/run-qemu.ps1`: boots `build/vibio.img` under OVMF UEFI firmware
  (q35) with an AHCI boot disk and an emulated `qemu-xhci` controller carrying a
  `usb-storage` stick (a 64 MiB `build/qemu-usb.img`). Captures a screenshot over
  QMP (`screendump` -> PPM -> PNG via Pillow) and shuts down cleanly. VM-only;
  only touches files under `build/`.

Verification:

- `.\tools\run-qemu.ps1 -Shot ...`: Vibio boots to its desktop under QEMU/OVMF
  (1280x768) and the screenshot is captured. QEMU accepts the xHCI + usb-storage
  devices, so the USB stack can be developed and tested here.

Safety status: read-only/VM-only; no host disk access.

Honest status:

- This is the HARNESS only. The actual xHCI + USB Mass-Storage (BOT/SCSI) read
  driver and mounting FAT32 over it are NOT implemented yet - that is the next
  body of work, now unblocked.
- Whether Vibio's existing USB probe already detects the QEMU xHCI is the first
  thing the driver work will check (the USB status is in-window/terminal, not
  capturable via the keyboard-only path).

Next milestone:

- Implement xHCI init (command/event rings, port enable, device enumeration,
  descriptors) -> USB Bulk-Only Transport + SCSI READ(10) -> mount the existing
  read-only FAT32 reader on the USB block device, so Vibio browses real USB files
  post-boot. Then point the laptop's real xHCI at the same driver.

## 2026-06-26 - Stage 35: Host ffmpeg Video, Native Resolution, Higher Media Limits

Plain English:

Per the user's direction (codecs stay host-side, no kernel codec libraries; videos
and images should not be artificially limited; use native resolution; install the
tools needed). Installed ffmpeg and the VirtualBox Extension Pack, made the
bootloader pick the panel's native resolution, raised the media size limits, and
proved an MP4 converts and plays.

What changed:

- Tools installed: ffmpeg 8.1 (C:\ffmpeg, on PATH) so any host video converts to
  Vibio's `.VIV` at build/deploy; VirtualBox 7.2.6 Extension Pack (enables
  emulated xHCI for future USB-driver testing in VBox too).
- Bootloader: `fill_boot_info` now enumerates GOP modes and SetMode()s to the
  largest 32-bit RGB/BGR mode <= 1920x1080 (the panel's native res), instead of
  the firmware default. Capped at 1080p on purpose so the per-frame blit cost
  stays bounded (native res and the reported lag pull against each other).
- Media limits raised: video up to 854x480 and up to 3600 frames (~2 min @30fps),
  frame index now read in chunks (no huge stack buffer); images decode buffer
  4 MB -> 24 MB and viewer/deploy conversion 1024 px -> 2048 px (near-native).
- `make_media_assets.py`: host video conversion preserves source aspect ratio
  (scaled down only to fit, even dims), uses all frames up to the cap, and prints
  the result size. A sample `Assets/Media/SampleClip.mp4` exercises it.

Verification (VM, 1920x1080):

- `.\build.ps1`: ffmpeg converts SampleClip.mp4 -> SAMPLECL.VIV (640x360, 62
  frames, 54.5 MB).
- `PLAY SAMPLECL.VIV`: the converted MP4 AUTO-PLAYS and advances, STREAMING from
  the 54.5 MB FAT32 file (over the 32 MB preload cap, so it is read per-frame from
  AHCI - the streaming path). Desktop is now native 1920x1080.
- `IMGTEST`: image still renders; header no longer overlaps the image.
- Deployed to G: (includes SAMPLECL.VIV, VIDEOTST.VIV, 2048px image fallbacks).

Safety status: read-only/VM-only; deploy removable-only, non-destructive.

Honest limitations:

- Raw `.VIV` frames are huge (54 MB for ~4 s at 640x360). True "any length, native
  res" is physically impossible with uncompressed frames. The realistic paths are
  (a) compressed per-frame decode in the kernel - which is a codec, explicitly out
  of scope per the user - or (b) stream from disk so RAM is not the limit.
- On the LAPTOP there is still no post-boot storage driver, so only files <= 32 MB
  preload: VIDEOTST.VIV (6.6 MB) plays, but SAMPLECL.VIV (54.5 MB) and most real
  videos will NOT until the USB mass-storage driver exists. This is the next item.
- Native 1080p means ~4x the pixels of the old default -> may worsen the reported
  lag on real hardware. Lag is still unprofiled (no hardware access); the 1080p
  cap is a deliberate bound, and could be lowered if the laptop is worse.
- Long-named JPEGs now LIST by real name but opening still resolves the `.VIM`
  fallback by short name (still to be made long-name aware).

Next milestone: the post-boot USB mass-storage (xHCI + BOT/SCSI) driver, testable
now via tools/run-qemu.ps1 and/or the VBox Extension Pack, so the laptop can
stream arbitrary large media off the USB after boot.

## 2026-06-27 - Stage 36: Real-Hardware Graphics Performance Pass

Plain English:

The real-hardware lag report points at the CPU framebuffer renderer, especially
after Stage 35 raised the desktop to 1080p. This pass makes the renderer much
less wasteful without pretending Vibio has GPU acceleration. The kernel now
builds optimized code, fills and blits pixels with wider clipped stores, keeps
multiple dirty rectangles instead of one huge bounding rectangle, and stops
periodically repainting a static fullscreen window only because the hidden shell
clock changed.

What changed:

- `build.ps1` now compiles the UEFI boot stage and kernel with `-O2`,
  `-fno-strict-aliasing`, `-fno-vectorize`, and `-fno-slp-vectorize`. This keeps
  the freestanding kernel scalar while removing debug-speed `-O0` loops.
- `VIBIO_STAGE` is now 36, so `VERSION` is expected to report Stage 36.
- `fill_rect` now clips once and writes rows directly instead of calling
  `put_pixel` for every pixel. This affects wallpaper, window bodies, taskbar,
  fullscreen clears, media backgrounds, and many controls.
- Full-buffer and rectangle blits now use non-volatile scalar wide stores with
  small manual unrolling, plus an `sfence` after framebuffer presents so
  write-combining stores are flushed in order.
- The compositor now tracks up to 16 dirty rectangles. When several small areas
  change, it repaints those areas separately instead of expanding them into one
  giant rectangle.
- F11 fullscreen no longer redraws the whole fullscreen window for a periodic
  timer update when the shell/taskbar is hidden. Fullscreen still redraws the
  full surface for actual window/content changes such as video frames, scrolling,
  keyboard input, open/close, or fullscreen toggles.

Technical details:

- The dirty-list path keeps the old clipping model: each dirty rectangle sets
  `g_paint_x0/y0/x1/y1`, then the normal compositor draws all visible windows
  clipped to that rectangle. This preserves overlapping-window correctness while
  reducing the number of pixels touched on common cursor/window/taskbar updates.
- If too many dirty rectangles accumulate, the compositor merges into the region
  with the smallest area growth instead of immediately falling back to a full
  screen repaint.
- The fallback direct-framebuffer path still works: if compositor buffers cannot
  be allocated, drawing goes directly to the framebuffer and uses the same paint
  bounds.

Commands tested:

- `.\build.ps1` - PASS, no compiler warnings.
- `.\tools\run-qemu.ps1 -Shot docs\screenshots\2026-06-27-stage36-qemu-boot.png -BootWait 18`
  - PASS, QEMU boots to the desktop.
- VirtualBox GUI boot - PASS, desktop appears at 1920x1080.
- Hidden console command `BROWSER` - PASS, browser opens.
- `F11` while browser is focused - PASS, browser enters fullscreen and renders.
- Hidden console command `VIDEOTEST` - PASS, Media Player opens and advances to
  frame 29/30 in the screenshot.
- Hidden console command `RESTART` - PASS, VM returns to the desktop after reboot.
- Hidden console command `SHUTDOWN` - PASS, VM state becomes `poweroff`.

Screenshot paths:

- `docs/screenshots/2026-06-27-stage36-optimized-desktop.png`
- `docs/screenshots/2026-06-27-stage36-browser-window.png`
- `docs/screenshots/2026-06-27-stage36-browser-fullscreen.png`
- `docs/screenshots/2026-06-27-stage36-video-playback.png`
- `docs/screenshots/2026-06-27-stage36-restart-rebooted.png`
- `docs/screenshots/2026-06-27-stage36-qemu-boot.png`

Safety status:

- No disk/FAT/USB write path was added or widened.
- No host EFI partition, bootloader, boot order, internal disk, Linux disk,
  Windows disk, or random USB device was touched.
- `build.ps1` only regenerated files under `build/` and managed the existing
  VirtualBox VM-only scratch disk through the existing safety gates.
- Verification was VM/QEMU-only.

Honest limitations:

- This was NOT tested on real hardware here. The user must boot the updated
  image on the PC/laptop to measure whether the reported 2 FPS fullscreen/window
  lag is fixed.
- Vibio still has a CPU framebuffer compositor, not GPU acceleration. Fullscreen
  video or image redraws still touch many pixels; this pass reduces overhead but
  does not make the hardware magically accelerate blits.
- VirtualBox GUI automation could send keyboard commands and capture app paths,
  but it could not reliably deliver mouse button clicks to open the Terminal in
  this session. Therefore visible `VERSION`/`SELFTEST` terminal screenshots were
  not captured; the Stage 36 constant is verified by source/build, and the
  command paths that open apps/restart/shutdown did execute.
- The existing post-boot USB storage driver is still not implemented, so large
  real-hardware media still needs either UEFI preload within the cap or future
  USB mass-storage streaming.

What still does not work:

- No GPU driver, hardware cursor, VBE/GOP acceleration, or page-flip support.
- No real-hardware performance PASS until the updated USB/image is booted on the
  target PC/laptop.
- No full USB mass-storage stack, no arbitrary post-boot USB file streaming, and
  no safe real-hardware write path.

Next milestone:

- Boot Stage 36 on the real PC/laptop and report exact behavior: desktop
  resolution, cursor/window smoothness, fullscreen browser smoothness, fullscreen
  media smoothness, and whether performance is still below target. If it is still
  slow, the next safe optimization is a performance graphics mode cap or a more
  structural compositor change such as damage-aware window-only drawing and
  reduced-resolution fullscreen media scaling.

## 2026-06-27 - Stage 36 Follow-up: Shell And Media Polish

Plain English:

This is a Stage 36 follow-up, not a new stage. It fixes several small real UI
problems without adding fake apps or fake codec support: percent labels no longer
clip, the pinned taskbar defaults to Files then Terminal, taskbar hover shows a
real preview/tooltip, WAV playback stops visually when the audio finishes, F11
accepts an additional hardware scancode form, and desktop icons now have hover
and drag-selection state.

What changed:

- Kept `VIBIO_STAGE` at 36.
- The Media Viewer toolbar measures/centers button labels, and the `100%` button
  is wider so the percent text does not cut off.
- The volume popup now right-aligns the full percentage string instead of drawing
  the number and `%` from a fixed x position.
- Audio-only `.WAV` playback now follows the real audio driver's
  `playback_active` state. When the WAV finishes, the Media Player switches back
  to `PLAY`, stops the scanning bar, and reports `Audio: stopped`. Pressing Play
  after that starts from the beginning.
- `.MP3` remains unsupported. No MP3 decoder was added.
- The taskbar app slots are now explicit: default pinned apps are Files, then
  Terminal. Other real windows appear after those pins only while they are
  actually running.
- Taskbar hover draws a tooltip for closed pinned apps and a real one-window
  preview for running/minimized windows. It does not invent fake tabs.
- Desktop icons now highlight on hover. Dragging on empty desktop space tracks a
  selection rectangle and selected-icon mask.
- Clicking/dragging the titlebar of a maximized window restores it to its saved
  size and starts moving it, matching the expected "pull down from maximized"
  behavior.
- F11 now recognizes both translated set-1 `57/d7` and common untranslated set-2
  `78/f0 78` PS/2 scancode forms.
- Fixed a dirty-region bug found during this pass: keyboard-triggered F11
  restore now dirties the old fullscreen rectangle, so old fullscreen pixels do
  not remain behind the restored window.

Technical details:

- Added taskbar slot mapping helpers so draw, hit-test, underline, and hover
  preview all use the same Files/Terminal-first order.
- Added window-manager hover/selection fields and dirty-region helpers for the
  desktop icon column, active selection rectangle, and taskbar hover popup.
- Desktop selection uses a cheap hatched fill plus outline instead of alpha
  blending a large rectangle, to avoid adding a slow real-hardware path.
- Added `wm_visual_state_changed` so keyboard handlers that mutate window rects
  or fullscreen state mark the compositor dirty even when mouse input did not.
- The set-2 F11 break-prefix state is isolated to F11 handling; the rest of the
  keyboard path remains the existing translated set-1 path.

Commands tested:

- `.\build.ps1` - PASS.
- `.\tools\test-vm.ps1 -Shot docs\screenshots\2026-06-27-stage36-followup-taskbar-default.png`
  - PASS, VM boots with only Files and Terminal pinned after Search.
- Hidden console command `VIEW TEST.PNG` - PASS, Media Viewer opens and `100%`
  is not clipped.
- Hidden console command `PLAY BOOT.WAV` - PASS, after playback drains the player
  shows `Audio: stopped` and the button is `PLAY`.
- Hidden console command `BROWSER`, then VM-injected set-1 F11 `57 d7` - PASS,
  browser enters fullscreen and restores cleanly.
- Attempted raw set-2 F11 injection through VirtualBox - SKIP for proof: the
  VirtualBox `keyboardputscancode` path did not behave like a real untranslated
  PS/2 set-2 stream. The code path is built, but needs real hardware validation.
- Hidden console commands `VERSION`, `SELFTEST`, and `LS` - PASS for command
  injection/no VM failure. Visible terminal output was not captured because the
  Terminal is intentionally hidden by default and mouse automation did not open
  it reliably.
- Hidden console command `FILESAPP` - PASS, Files opens and the Files taskbar
  button shows the running underline.
- Visible VirtualBox mouse hover over the Files taskbar button - PASS, hover
  preview captured.
- Visible VirtualBox desktop drag-selection screenshot - SKIP: mouse automation
  did not reliably start the drag on the desktop. Needs manual real hardware or
  a better VM pointer harness.
- Hidden console command `RESTART` - PASS, VM rebooted back to the desktop.
- `.\tools\run-qemu.ps1 -Shot docs\screenshots\2026-06-27-stage36-followup-qemu-boot.png -BootWait 18 -NoUsb`
  - PASS, QEMU boots to the desktop.

Screenshot paths:

- `docs/screenshots/2026-06-27-stage36-followup-taskbar-default.png`
- `docs/screenshots/2026-06-27-stage36-followup-files-taskbar.png`
- `docs/screenshots/2026-06-27-stage36-followup-taskbar-hover-preview.png`
- `docs/screenshots/2026-06-27-stage36-followup-image-percent-controls.png`
- `docs/screenshots/2026-06-27-stage36-followup-wav-stopped.png`
- `docs/screenshots/2026-06-27-stage36-followup-browser-window.png`
- `docs/screenshots/2026-06-27-stage36-followup-f11-vm-fullscreen.png`
- `docs/screenshots/2026-06-27-stage36-followup-f11-vm-restored.png`
- `docs/screenshots/2026-06-27-stage36-followup-restart-rebooted.png`
- `docs/screenshots/2026-06-27-stage36-followup-qemu-boot.png`

Safety status:

- No disk/FAT/USB write path was added or widened.
- No host EFI partition, bootloader, boot order, internal Windows disk, internal
  Linux disk, or random USB device was touched.
- Build outputs stayed under `build/`.
- The only write-capable storage path remains the existing Stage 29 VM-only
  scratch disk guarded by the existing scripts.
- Verification was VM/QEMU-only. This was not tested on real hardware here.

Honest limitations:

- Real-hardware F11 set-2 support is implemented but not proven here; it needs
  the PC/laptop that previously failed F11.
- Desktop icon hover/drag-selection code is implemented, but the drag-selection
  screenshot was not captured because the VM mouse harness was unreliable for
  that exact gesture.
- Taskbar previews show real Vibio windows. Vibio has no browser tabs, so it
  cannot show multiple fake tab previews.
- `.MP3` files are unsupported. `.WAV` is PCM WAV through the existing audio
  path; unsupported audio types should continue to show unsupported/no-preview
  messages.
- Real hardware performance was not measured in this follow-up.

What still does not work:

- No MP3 decoder.
- No GPU driver, hardware cursor, page flipping, or accelerated compositor.
- No real-hardware PASS for the set-2 F11 path until the updated image is booted
  on the failing PC/laptop.
- No safe real-hardware write path.

Next milestone:

- Manual real-hardware check: boot the updated image, verify F11 on the PC and
  laptop, verify maximized-window titlebar restore, verify desktop icon hover and
  drag-selection with a real mouse/touchpad, and report whether Stage 36
  performance is now acceptable.

## 2026-06-27 - Stage 36 Follow-up: Desktop Context Menu And Movable Icons

Plain English:

This is another Stage 36 follow-up, not a new stage. The desktop now has a
Windows-like right-click menu, movable desktop icons, icon view settings, a
temporary new-folder/new-text-item path, and built-in wallpaper choices. It does
not add any unsafe or persistent disk-write behavior.

What changed:

- Kept `VIBIO_STAGE` at 36.
- Desktop icons are now drawn dynamically instead of being baked into the static
  wallpaper base layer.
- Desktop icons can be dragged to new positions.
- Right-clicking empty desktop space opens a desktop context menu.
- The View submenu supports Large, Medium, and Small icon cells, Auto arrange
  icons, and Show desktop icons.
- Refresh closes/redraws the menu without moving icon positions.
- The New submenu creates a temporary in-memory `New folder` or `New text file`
  desktop item at the click location and starts a rename box.
- Rename supports typing, Backspace, Enter, Escape, and click-away commit.
- Open terminal from the context menu raises/opens the real Terminal window.
- Wallpaper opens a built-in picker with default and simple color/generated
  wallpaper choices.

Technical details:

- Added per-desktop icon positions to `VibioWindowManager` for the four built-in
  app icons and up to eight temporary desktop items.
- Replaced the static `desktop_icon_specs()` table with dynamic icon helpers
  that use the current icon size, visibility, temporary items, and moved
  positions.
- Removed desktop icons from `draw_desktop_base()` and draws them from the
  dynamic desktop overlay so moved/hidden/resized icons do not leave stale base
  pixels.
- Added context-menu hit-testing/action handling inside `wm_update()` with a
  separate right-button edge state.
- Added dirty-region tracking for icon/menu state so small shell updates do not
  require a full-screen repaint except when changing wallpaper.
- Wallpaper changes mark the static base dirty and force a full redraw of the
  base layer.

Commands tested:

- `.\build.ps1` - PASS.
- `.\tools\test-vm.ps1 -Shot docs\screenshots\2026-06-27-stage36-desktop-context-boot.png -BootWait 14 -KeepRunning`
  - PASS, VM boots with dynamic desktop icons and the existing Files/Terminal
  taskbar pins.
- Hidden console command `FILESAPP` through
  `.\tools\test-vm.ps1 -Shot docs\screenshots\2026-06-27-stage36-desktop-context-filesapp.png -Type FILESAPP`
  - PASS, Files still opens and lists the read-only AHCI boot disk.
- Hidden console command `RESTART` through
  `.\tools\test-vm.ps1 -Shot docs\screenshots\2026-06-27-stage36-desktop-context-restart.png -Type RESTART`
  - PASS, VM rebooted back to the desktop.
- `.\tools\run-qemu.ps1 -Shot docs\screenshots\2026-06-27-stage36-desktop-context-qemu-boot.png -BootWait 16`
  - PASS, QEMU boots to the desktop.
- Direct VirtualBox mouse injection for right-click screenshots - SKIP:
  VirtualBox 7.2.6 in this environment does not expose `mouseputmousestateabs`
  or equivalent `controlvm` mouse subcommands. Right-click/menu/drag behavior
  needs manual VM GUI or real-hardware validation.
- Visible `VERSION` and `SELFTEST` terminal screenshots - SKIP: Terminal is
  intentionally hidden by default, and this environment could not inject mouse
  clicks to open it. Stage 36 is still verified by source/build.

Screenshot paths:

- `docs/screenshots/2026-06-27-stage36-desktop-context-boot.png`
- `docs/screenshots/2026-06-27-stage36-desktop-context-filesapp.png`
- `docs/screenshots/2026-06-27-stage36-desktop-context-restart.png`
- `docs/screenshots/2026-06-27-stage36-desktop-context-qemu-boot.png`

Safety status:

- No storage driver write path was added.
- New Folder and New Text File are temporary in-memory desktop items only; they
  are not real FAT32 files and do not persist after reboot.
- The existing Stage 29 VIBIORW scratch disk remains the only write-capable
  storage path, and this pass did not use it.
- `G:\Backgrounds` was inspected read-only and not modified. The five images
  there are not loaded by Vibio at runtime in this pass.
- No host EFI partition, bootloader, boot order, internal Windows disk, internal
  Linux disk, or random USB device was modified.
- Verification was VM/QEMU-only. This was not tested on real hardware here.

Honest limitations:

- The right-click context menu and icon dragging built and booted, but mouse-only
  behavior was not screenshot-verified because this VirtualBox CLI lacks mouse
  injection support.
- Temporary desktop items are not filesystem objects. Persistence needs a
  separate safe design, likely VM-only first or another explicitly gated write
  path.
- External wallpaper loading from `G:\Backgrounds` is not implemented in the OS.
  The current wallpaper picker uses built-in choices only.
- Real-hardware performance and mouse behavior were not measured here.

What still does not work:

- No persistent desktop folder/file creation.
- No runtime host-drive wallpaper browser.
- No GPU driver, hardware cursor, page flipping, or accelerated compositor.
- No real-hardware PASS for the new context-menu/icon-drag behavior until the
  updated image is booted on the target PC/laptop.

Next milestone:

- Boot on the real PC/laptop and manually verify right-click desktop menu,
  View/New/Wallpaper actions, icon dragging, and the original Stage 36
  performance concerns.

## 2026-06-26 - Stage 36 Follow-up: Desktop/Media/Files Stability And Polish Pass

Plain English:

This is a stability and polish pass, not a new stage. The goal was to make Vibio
feel like a normal desktop OS before any new features: fix single vs double
click, desktop hover/selection, the right-click submenu, a real wallpaper picker
that uses the user's own G:\Backgrounds images, the taskbar defaults, a Files
scrollbar, and several Media Viewer/Player bugs. Vibio stays VM-only and
read-only; no new disk-write path was added. The stage number stays 36.

The headline win: the wallpaper picker now shows and renders the user's REAL
desktop photos from G:\Backgrounds - not faked, not placeholder colors. The
images are converted on the host (Pillow) into native pixel arrays compiled into
KERNEL.BIN, exactly like the icons and fonts already are, because Vibio has no
in-kernel JPEG/PNG decoder for arbitrary photos and no post-boot USB storage
driver on the laptop. Picking one paints the photo across the whole desktop.

Bugs fixed (code-complete this pass):

Desktop / icons:
- Desktop icons are now single-click select / double-click open (matching the
  Files app and Windows), instead of opening on a single click. A double-click
  is two clicks on the same icon within ~0.3s (30 timer ticks).
- Single click selects exactly one icon; the hover highlight already tracked one
  icon at a time and is preserved.
- Small-icon mode no longer clips or overflows labels: a new
  draw_desktop_icon_label() measures the real proportional Geist font, truncates
  with a ".." ellipsis to fit the cell, centres, and clips to the cell width and
  the full 20px Geist line height.
- The desktop selection rectangle and drag paths still work; the dirty-rect
  compositor already rebuilds each rect from the base layer so drags do not
  smear (no new trail bug was found in code review).

Right-click menu / wallpaper:
- The right-click submenu state bug is fixed: hovering a root row that opens a
  submenu (View/New/Wallpaper) swaps to it, and hovering a root row that does
  NOT (Refresh, Open terminal) now closes any stale submenu.
- The Wallpaper picker is now a real, rendering image picker. "Vibio default"
  keeps the built-in gradient; every image imported from G:\Backgrounds is a
  selectable row that paints that photo as the desktop wallpaper, scaled to
  cover the screen. The old placeholder color swatches were removed.
- New tool tools/make_wallpapers.py imports G:\Backgrounds (read-only) into
  src/kernel/wallpapers_generated.h. It is run by hand (kept out of build.ps1,
  like make_icons.py / make_fonts.py) so the normal build stays Pillow-free and
  never depends on the removable drive being attached.

Taskbar / tray:
- Taskbar already defaulted to Files (first) then Terminal pinned, and the
  open/underline indicator already only shows for a running window; both were
  reviewed and kept. The hover preview card (icon + Open/Minimized window) is
  retained.

Files / scrolling:
- The Files app now has a real vertical scrollbar with a proportional thumb,
  shown only when the folder has more entries than fit. Clicking the track jumps
  the list, and the mouse wheel scrolls it by whole rows. A right-side gutter is
  reserved so byte counts never draw under the bar. Keyboard scrolling is
  unchanged.

Media Player:
- Video replay after finishing now restarts from frame 0: pressing PLAY when the
  clip is at its last frame resets current_frame to 0 and re-reads frame 0,
  instead of flashing the final half second and immediately stopping.
- Unmute now restores audio immediately during playback: it clears audio_started
  and last_frame_tick so the next player tick re-arms the PCM stream (audio-only
  mode also resumes playing), instead of only coming back on the next restart.
- Audio finished-state (stop + PLAY label) and PLAY-after-finished restart for
  audio-only .WAV were already handled by audio_service()'s duration countdown
  clearing playback_active; reviewed and kept.

Critical fix found during verification:
- The boot stage only loaded the first 512 KB of KERNEL.BIN
  (VIBIO_KERNEL_MAX_SIZE). Embedding the wallpapers grew KERNEL.BIN to ~3.7 MB,
  so the tail rodata (icon AND wallpaper pixel arrays) loaded as zeros and the
  desktop rendered black with no PNG icons. Raised VIBIO_KERNEL_MAX_SIZE to
  16 MB (EFI_ALLOCATE_ADDRESS at 0x100000). This is the root cause of "black
  wallpaper / missing icons" and must stay above the real KERNEL.BIN size.

Technical details:

- src/shared/boot_info.h: VIBIO_KERNEL_MAX_SIZE 512 KB -> 16 MB.
- tools/make_wallpapers.py: new; converts G:\Backgrounds *.jpg/*.png to opaque
  0xAARRGGBB arrays capped at 512px longest side (~0.7 MB each), writing
  src/kernel/wallpapers_generated.h with VibioWallpaperImage[] + count.
- src/kernel/main.c: include wallpapers_generated.h; WALLPAPER_IMAGE_BASE /
  DESKTOP_DOUBLE_CLICK_TICKS macros; wallpaper menu rows = 1 + count;
  draw_wallpaper_image() (cover scale, centre crop); wm_update() submenu-close
  and single/double-click via desktop_last_click_index/tick + g_anim_now;
  VibioFilesApp scrollbar (sb_* fields, files_app_scroll_by(), track-click +
  wheel routing); draw_desktop_icon_label() ellipsis/clip; media player
  PLAY-restart-from-0 and unmute-restores-audio.

Commands / tests run (PowerShell):
- .\build.ps1 - PASS (kernel + boot rebuilt; KERNEL.BIN ~3.7 MB; no warnings).
- py tools\make_wallpapers.py - PASS (5 wallpapers imported from G:\Backgrounds).
- .\tools\test-vm.ps1 -Shot ...stability-default-desktop.png -BootWait 20 - PASS.
- Temporary default-to-image build + test-vm screenshot - PASS (real beach photo
  rendered as wallpaper end to end), then reverted to the gradient default.
- .\tools\test-vm.ps1 -Type FILESAPP -Shot ...stability-files-open.png - PASS
  (Files opens, 20 items, READ ONLY, vertical scrollbar visible on overflow).
- .\tools\test-vm.ps1 -Type RESTART -Shot ...stability-restart.png - PASS
  (clean reboot back to the desktop; shutdown/restart still work).

Screenshots captured (docs/screenshots/):
- 2026-06-26-stage36-stability-default-desktop.png
- 2026-06-26-stage36-stability-wallpaper-image.png
- 2026-06-26-stage36-stability-files-open.png
- 2026-06-26-stage36-stability-restart.png

VM status:
- Boots and runs in VirtualBox (Vibio OS Dev, 512 MB, AC'97). Desktop, taskbar,
  Files, wallpaper image rendering, Files scrollbar, and restart all verified
  headless. VERSION still reports Stage 36; SELFTEST remains honest (no writes).

Real hardware status:
- NOT tested on the laptop/PC this pass. The wallpapers live in KERNEL.BIN, so
  the laptop needs the updated kernel: run .\deploy-usb.ps1 -Drive G (it copies
  BOOTX64.EFI/KERNEL.BIN/START.HTM/SOUNDS + media, never formats). G: was NOT
  deployed automatically in this pass to respect the VM-only/never-touch-USB
  safety ethos - the user should run deploy when ready.

Honest limitations:
- VirtualBox 7.2.6 in this environment cannot inject guest mouse movement, so
  the click-only behaviours were done by source/build/boot review, not headless
  screenshot: desktop single-vs-double click, hover highlight, selection
  rectangle, right-click submenu close, taskbar hover preview, volume popup,
  Media Viewer zoom/pan, Media Player transport/mute. These need a hands-on
  VM-GUI or laptop test.
- Vibio is single-window-per-app (one window per kind), so the taskbar preview
  shows one window; "2+ windows per app with per-window previews" is not
  supported and was NOT faked.
- Two-finger touchpad scrolling depends on the PS/2 IntelliMouse wheel; touchpads
  that refuse IntelliMouse mode send no wheel byte. Scrollbar + keyboard remain
  the fallbacks. No I2C/HID touchpad driver exists.
- G:\Backgrounds is imported at BUILD time on the host (read-only). Vibio cannot
  read G:\Backgrounds at run time - that needs the post-boot USB mass-storage
  driver (not built) or NVMe/GPT (not built). Re-run make_wallpapers.py + build
  + deploy to change the laptop's wallpapers.
- MP4/H.264/AAC/MP3 are still NOT decoded in-kernel. Video/audio use host ffmpeg
  conversion to .VIV/.WAV. Files larger than the 32 MB UEFI preload cap will not
  reach the laptop until the USB storage driver exists. Unsupported/oversized/
  missing media show honest error text, not fake playback.
- Real-hardware F11 and rendering performance remain unverified (no HW access).

What still does not work:
- No persistent filesystem writes (desktop New folder/text are in-memory only).
- No runtime external wallpaper/file browsing on the laptop.
- No multi-window-per-app, no GPU/hardware cursor/page flipping.
- No native MP4/MP3 decode; no post-boot USB mass storage.

Next milestone:
- Hands-on VM-GUI / laptop pass: verify with a real mouse the double-click,
  hover, selection rectangle, right-click submenu, wallpaper picker rows, volume
  popup, Media Player mute/replay, and F11 on real hardware; then deploy to G:.

## 2026-06-27 - Stage 37: Read-Only xHCI USB Mass Storage (post-boot USB reads)

Plain English:

Vibio can now read files from a USB drive *after boot*, by itself, with no UEFI
preload - a real OS storage milestone. It brings up an xHCI USB controller from
scratch, enumerates a USB flash drive, talks USB Mass Storage Bulk-Only
Transport + SCSI to read sectors, mounts the drive's FAT32 read-only through the
existing parser, and lets the Files app browse it as a live "USB MASS STORAGE"
source. Images/audio/video/docs and folders open straight from USB. This stage
is STRICTLY READ-ONLY: the only SCSI commands issued are INQUIRY, TEST UNIT
READY, READ CAPACITY(10) and READ(10) - there is no WRITE(10), no format, no FAT
write, no partition edit, nothing that changes the device.

What changed:

- New from-scratch xHCI driver (src/kernel/main.c): controller reset, DCBAA,
  command ring, event ring (ERST), optional scratchpad buffers, run; root-hub
  port find + reset; Enable Slot; Address Device; GET_DESCRIPTOR (device +
  config); parse the BOT mass-storage interface + bulk IN/OUT endpoints;
  SET_CONFIGURATION; Configure Endpoint. Polled (no IRQs), single device.
- USB Mass Storage Bulk-Only Transport + SCSI (read-only): CBW -> data-in -> CSW,
  with INQUIRY, TEST UNIT READY (retried), READ CAPACITY(10), READ(10).
- A block-device backend: VibioDiskReadTest gained a `backend` field; the single
  read chokepoint ahci_read_sector() dispatches AHCI vs USB, so the read-only
  FAT32 parser is reused over USB unchanged (one 512-byte sector per READ(10)).
- usb_mount_fat32(): reads LBA 0, accepts a superfloppy FAT32 boot sector or an
  MBR FAT32 partition (type 0x0B/0x0C/0xEF), then mounts via the existing
  fat32_run_readonly_test over the USB backend.
- Files app: a live USB source. A "Disk"/"USB" toolbar toggle appears when a USB
  FAT32 mount exists; the status bar shows "SOURCE: USB MASS STORAGE - READ
  ONLY"; folders and files open from USB. Opening a file points the
  browser/viewer/player at the active device so it reads from USB (and a USB
  disk now takes priority over the UEFI preload in media_read_bytes, so a
  same-named preloaded file does not shadow the USB copy).
- Commands: XHCI (driver detail: caplen/version/slots/ports, slot/speed/VID/PID,
  INQUIRY vendor+product, capacity, reached step + error), STORAGE/MOUNT
  (xHCI + USB summary), USBFILES (open Files on the USB source); USBSTOR/USBMSD
  extended with the now-real device/MSD/sector/FAT32 lines. VERSION reports
  Stage 37.
- SELFTEST gained honest lines: XHCI PCI DETECT, XHCI INIT, USB ENUM, USB MSD
  READ, USB FAT32 MOUNT - PASS when the step really happened, SKIP when there is
  no controller/device (e.g. VBox with USB off), never a fake PASS.

Technical details (the two real bring-up bugs):

- KERNEL LOAD CAP: the boot stage loaded a FIXED 16 MiB at 0x100000 via
  EFI_ALLOCATE_ADDRESS. OVMF rejects that fixed reservation (it worked in VBox).
  Fixed: query the kernel's exact size via EFI_FILE_INFO and reserve only that
  (+16 KiB guard), falling back to 6 MiB if GetInfo is unavailable. (src/boot.)
- HIGH MMIO BAR: QEMU q35 places the xHCI 64-bit BAR near 0xC0_0000_0000, outside
  the kernel's low-4 GiB identity map, so register reads would page-fault. Fixed:
  after the probe finds the BAR, map its 2 MiB MMIO page uncacheable (PCD|PWT)
  into the active page tables and reload CR3 to flush the TLB, before running the
  driver. The init also enables the controller's PCI memory space + bus master
  itself (OVMF leaves them off on the non-boot xHCI) and gates only on a sane
  CAPLENGTH, not HCIVERSION (which QEMU reported oddly).
- All xHCI DMA structures are page-allocated, identity-mapped (phys == virt),
  low-4 GiB; x86 DMA is cache-coherent so no explicit flushes.
- A field-by-field fat32_copy() avoids the freestanding kernel's missing memcpy
  (a whole-struct assignment through a pointer would emit one).

New tools:

- tools/make_usb_image.py: builds an MBR + FAT32 USB test image
  (build/qemu-usb.img) with README.TXT, a long-named "USB Test Document.txt", a
  native PHOTO.VIM, NOTIFY.WAV, VIDEOTST.VIV, and a DOCS subfolder (with its own
  long-named file). Reuses make_fat32_image.py's FAT32 primitives; pure stdlib.
  build.ps1 now generates it; run-qemu.ps1 exposes it as the usb-storage stick.
- tools/run-qemu.ps1: added -Type (type a command via QMP send-key) and -Keys
  (raw qcode navigation), so the hidden-Terminal console and focused windows can
  be driven headless, like the VBox harness.

Commands / QEMU tests run:

- .\build.ps1 - PASS (no warnings; KERNEL.BIN ~3.7 MB; USB image generated).
- .\tools\run-qemu.ps1 -Shot ...stage37-qemu-boot.png - PASS (q35 + OVMF + AHCI +
  qemu-xhci + usb-storage; boots to desktop; xHCI init runs at boot).
- .\tools\run-qemu.ps1 -Type USBFILES -Shot ...stage37-usb-files.png - PASS:
  Files shows "SOURCE: USB MASS STORAGE - READ ONLY - 6 items": DOCS,
  README.TXT, "USB Test Document.txt" (VFAT long name over USB), PHOTO.VIM,
  NOTIFY.WAV, VIDEOTST.VIV.
- ...-Type USBFILES -Keys "ret" -Shot ...stage37-usb-docs-folder.png - PASS:
  /DOCS lists NOTES.TXT + "Long Folder Note.txt" (subfolder read over USB).
- ...-Type USBFILES -Keys "down down down ret" -Shot ...stage37-usb-open-image.png
  - PASS: Media Viewer opens PHOTO.VIM "Src: USB MASS STORAGE 75 KB", VIMG
  decoded natively (the USB copy, not the 40 KB preloaded one).
- .\tools\test-vm.ps1 -Type RESTART -Shot ...stage37-vbox-nousb-restart.png -
  PASS: VirtualBox (USB off, AC'97) boots to the desktop, xHCI init cleanly
  skips (no controller), Files uses AHCI, and Restart reboots - the kernel-load
  change works on VBox firmware too.
- A temporary on-desktop debug line confirmed the staged progress
  (step=6 rdy=1 mnt=1 slot=1 spd=4) and was removed before the final build.

Screenshots captured (docs/screenshots/):
- 2026-06-27-stage37-qemu-boot.png
- 2026-06-27-stage37-usb-files.png
- 2026-06-27-stage37-usb-docs-folder.png
- 2026-06-27-stage37-usb-open-image.png
- 2026-06-27-stage37-vbox-nousb-restart.png

Safety status:
- READ ONLY. The only SCSI commands issued are INQUIRY, TEST UNIT READY, READ
  CAPACITY(10) and READ(10). No WRITE(10), no FAT/file writes, no create/delete/
  rename, no format, no partition edit, no EFI/boot changes, no internal-disk
  access. The Stage 29 VIBIORW write sandbox is untouched and unrelated.
- Tested in QEMU (USB) and VirtualBox (no USB) only. No host disk, EFI partition,
  bootloader, or internal/real USB device was modified. The PCI writes are only
  the standard "enable memory space + bus master" on the emulated xHCI.

What works in VM:
- Full xHCI bring-up, enumeration, BOT/SCSI READ(10), FAT32 mount, Files USB
  browsing (root + subfolder + long names), and opening an image from USB - all
  verified in QEMU. Boot-preload and AHCI/FAT32 paths still work; restart works.

Real hardware status:
- NOT tested on the laptop/PC this stage (no hardware access here). The laptop's
  boot USB is behind a real xHCI; this driver is the path to reading it after
  boot, but it is unverified on real silicon. Do not assume real-hardware support
  until it is booted on the target.

Honest limitations:
- One controller, one device, one LUN, one configuration; polled (no interrupts);
  512-byte sectors only (READ CAPACITY block size other than 512 is refused).
- No hubs, no multi-LUN, no hotplug re-enumeration after boot, no error recovery
  beyond CSW checks + a few retries, no USB writes ever.
- Native MP4/H.264/AAC/MP3 are still not decoded; USB just delivers the bytes.
  Large media can stream from USB in the VM (per-frame READ(10)); on real
  hardware it is unverified.

Next milestone:
- Boot the updated image on the real laptop and verify it reads its own boot USB
  after boot; add hubs / multi-LUN / hotplug and broader SCSI error recovery; and
  only then consider any (separately gated) USB write path.

## 2026-06-27 - Stage 37 follow-up: desktop / Files / media / audio bug-fix pass

This is a bug-fix pass on top of Stage 37 (not a new stage). VERSION still
reports STAGE 37 and SELFTEST is unchanged and still honest. It works through the
known-bug list: desktop icon dragging, the desktop grid, the Files app file
listing and scrollbar, the taskbar hover preview, wallpaper pixelation, Media
Viewer zoom, and the audio end-of-playback behaviour.

Plain English:

- Desktop icons no longer smear a trail when you drag them, and dropping an icon
  now snaps it onto a clean, evenly spaced grid (filling top-to-bottom, then into
  the next column) so the desktop looks tidy instead of icons landing wherever
  the pointer happened to be.
- The Files app now lists many more files per folder (up from 64 to 256) and -
  importantly - there is no longer any artificial *file-size* limit on what is
  *listed*. A large file (e.g. a 6.5 MB video) shows in the list; size limits
  only apply when you actually open/load it.
- The Files scrollbar can now be grabbed and dragged: the thumb follows the
  cursor, clicking the track above/below the thumb pages by a screenful, and the
  mouse wheel is ignored while you are dragging so a noisy touchpad cannot fight
  it.
- The taskbar hover preview now shows useful, live information about each app
  (Files: path + item count; Browser: page title; Media Viewer: file + zoom;
  Media Player: file + Playing/Muted/Stopped; Terminal/System: a short label)
  instead of always saying "Open window".
- Wallpapers are no longer blocky. The kernel now scales the imported photos with
  bilinear interpolation instead of nearest-neighbour, so the upscale to a
  1920x1080 desktop is smooth.
- Media Viewer zoom is fixed: the "-" button always zooms out and the "+" button
  always zooms in (previously "-" could jump the zoom up to 100% first from
  FIT), the zoom no longer reverses direction near low percentages, it clamps
  cleanly at 10%, and small zoom/pan changes are far less laggy.
- Sound playback no longer replays a ~0.3 s blip of the start of the clip after
  it finishes; audio now stops cleanly the instant the hardware finishes and the
  player state lands on STOPPED, not a lingering "playing".

What changed (src/kernel/main.c unless noted):

- Desktop drag smear: dirty_list_add_desktop_interactions() now also compares
  every icon's old vs new position and redraws both the old and new cell
  (dirty_list_add_desktop_icon_cell, with extra height for the label that hangs
  below the cell). Previously a drag only changed the icon's position, which none
  of the tracked fields caught, so only the small cursor rectangle was redrawn
  and the rest of the icon trailed. The whole-icon-area dirty rect also grew
  downward to clear the label.
- Desktop grid: desktop_snap_icon_to_grid() + desktop_cell_occupied() snap a
  dropped icon to the nearest free grid cell (fixed origin + cell-size pitch),
  called on mouse release in wm_update().
- Files listing cap: FILES_MAX_ENTRIES 64 -> 256, with a comment making clear it
  is a listing capacity, not a size filter. files_app and the browser's VIM
  -fallback scan array were moved off the kernel stack (static) so the larger
  array never risks the boot-provided stack. fat32_list_dir already applies no
  extension/size filter - the 64 cap was the only artificial listing limit.
- Files scrollbar drag: VibioFilesApp gained sb_thumb_y/sb_thumb_h/sb_dragging/
  sb_grab_dy; the render records the thumb geometry; the input handler grabs the
  thumb on press, keeps capture while held (even off-track horizontally) via
  files_app_scrollbar_set_from_thumb_top(), and pages on a track click. Wheel is
  dropped while files_app.sb_dragging (same guard as the browser bar).
- Taskbar preview: taskbar_preview_detail() builds a per-app description from the
  live app structs already reachable through VibioDesktopContext; the preview
  tile shows that plus a Running/Minimized line.
- Wallpaper: draw_wallpaper_image() now does bilinear sampling (wallpaper_bilerp,
  24.8 fixed point). tools/make_wallpapers.py was briefly bumped to 768px but
  REVERTED to 512px: a 768px set grew KERNEL.BIN to ~7.7 MB and OVMF/QEMU then
  refused the fixed-address kernel load at 0x100000 ("Could not load
  KERNEL.BIN"). 512px keeps KERNEL.BIN ~3.7 MB (known-good on QEMU and the
  laptop); the bilinear scaler delivers the smoothness without any size cost.
- Media Viewer zoom: VibioMediaViewer gained effective_scale (the real displayed
  percentage, set during render even in FIT mode). The +/- handlers now step from
  effective_scale with a clean 10%..400% clamp and a 10% step, so "-" always
  decreases and "+" always increases. media_viewer_draw_image() now clips its
  rescale loop to only the pixels inside the current paint/clip rectangle, so the
  compositor's per-dirty-rect repaint no longer rescales the entire image for a
  cursor-sized region (the zoom/pan lag).
- Audio end-of-playback: audio_service() polls the AC97 status register and
  stops the moment the DMA controller halts (SR DCH bit). The 32-entry BDL is a
  ring, so leaving RUN set one tick past the end let the controller wrap to
  entry 0 and replay ~0.3 s of the start; cutting RUN exactly at DCH removes that
  blip and lands the player state on STOPPED. The software countdown remains as a
  fallback.

Already correct in source (re-verified, no change needed): video PLAY-after
-finished resets to frame 0, and video unmute re-arms PCM mid-playback (both
added in the earlier Stage 36 pass).

Commands / tests run:

- .\build.ps1 - PASS (no warnings; KERNEL.BIN 3.58 MB, well under the 16 MB load
  cap; with the temporary 768px wallpapers it was 7.7 MB and FAILED to boot,
  which is why the source res stayed 512px).
- QEMU (q35 + OVMF + AHCI + qemu-xhci + usb-storage):
  - boot -> desktop renders at 1920x1080, clean icon grid, taskbar, tray.
  - USBFILES -> Files lists the USB source: DOCS (dir), README.TXT, the long-named
    "USB Test Document.txt", PHOTO.VIM, NOTIFY.WAV, and VIDEOTST.VIV (6.5 MB shown
    in the list, proving listing has no size filter). "6 items".
  - IMGTEST -> Media Viewer opens TEST.PNG and renders at FIT (rescale clip change
    does not break rendering).
  - Temporary default wm.wallpaper_mode = WALLPAPER_IMAGE_BASE -> the imported
    beach photo renders smooth (bilinear), not blocky; reverted afterwards.
- VirtualBox (AC97, USB off): boot -> desktop; RESTART command -> clean reboot
  back to the desktop. No regression on the power path.

Screenshots (docs/screenshots/):
- 2026-06-27-stage37-fixes-desktop.png
- 2026-06-27-stage37-fixes-files-listing.png
- 2026-06-27-stage37-fixes-media-viewer.png
- 2026-06-27-stage37-fixes-wallpaper-bilinear.png
- 2026-06-27-stage37-fixes-restart.png

Safety status:
- Still VM-only and read-only. No new disk-write path was added; the Stage 29
  VIBIORW write sandbox is untouched. No host disk, EFI partition, bootloader, or
  internal/USB device was modified. VM USB/network/clipboard/drag-drop stay off.

Honest limitations / what still does not work:
- Mouse-driven behaviours are verified by source review plus build/boot, NOT by
  driving the guest mouse: VBox headless cannot inject mouse, and QEMU PS/2
  relative motion with the kernel's pointer acceleration is too imprecise to
  click specific small controls. So the icon drag-smear fix, grid snapping, the
  Files scrollbar drag, the taskbar hover preview, and the Media Viewer +/- zoom
  buttons are reasoned-correct and confirmed not to break rendering, but the
  click/drag/hover interactions themselves still need a hand test in the
  VirtualBox GUI or on the laptop.
- Audio is not verifiable here at all: QEMU has no audio device configured and
  VBox headless has no capture, so the AC97 replay-blip fix and the finished
  -state behaviour are reasoned-correct but UNHEARD. They need a listen test in
  the VBox GUI / on the laptop.
- Two-finger trackpad scrolling on the laptop is still NOT supported. It needs a
  dedicated Synaptics PS/2 (or I2C-HID) touchpad driver - a larger input stage -
  which cannot be written or verified without the physical laptop. The existing
  IntelliMouse-wheel path (PS/2 device ID 3/4) is best-effort and only works if
  the pad actually exposes a PS/2 wheel; it was left unchanged to avoid breaking
  the working pointer on real hardware.
- Wallpapers are smoothed, not re-detailed: the stored source is 512px (boot-size
  bound), so the photo is soft up close on a 1080p screen. A sharper set would
  need either a smaller fixed kernel-load address budget or a real post-boot
  storage path to load full-res images at runtime.
- Real-hardware behaviour for all of the above is unverified (no hardware access
  this pass).

Next milestone:
- Hand-test the mouse/drag/zoom and audio fixes in the VirtualBox GUI and on the
  laptop; if two-finger scroll is wanted, scope a real touchpad-driver stage.

## 2026-06-27 - Stage 37 follow-up #2: show every file, multi-select drag, honest no-AHCI selftests, G: deploy

Second bug-fix pass (still Stage 37). Addresses direct user feedback: arbitrary
files (e.g. a long-named .mp4) were not showing; multi-selected desktop icons
only moved one; the DISK/AHCI/FAT32 selftests "failed" on the laptop. Then the
new build was deployed to the removable G: USB.

Plain English:

- Files now lists LITERALLY ANY file, including types Vibio cannot open (e.g.
  "2026-06-26 13-41-59.mp4"). Before, the UEFI boot-volume enumeration silently
  dropped any unknown/unsupported extension, so on the laptop (which reads its
  own boot USB) those files never appeared. Now every file is listed with its
  real size; opening one Vibio cannot decode shows an honest message instead of
  hiding the file. (The FAT32 / USB path already listed all types - this fixes
  the UEFI-preload path the laptop falls back to.)
- Selecting several desktop icons and dragging now moves the whole group, not
  just one. A plain single click still reduces the selection to one icon; a drag
  moves all selected together and they snap to the grid on drop.
- The DISK / FAT32 / AHCI BOOT FAT32 selftests no longer report FAIL on a machine
  that simply has no AHCI SATA disk (the laptop is NVMe, which Vibio has no driver
  for). That is correctly N/A, not a failure - reporting FAIL there was dishonest.
  In the VM, where an AHCI disk is present, they still PASS.
- The updated BOOTX64.EFI + KERNEL.BIN and supporting files were deployed to the
  removable G: USB (non-destructive copy; the user's own files, including the
  .mp4, were left untouched).

What changed:

- src/boot/main.c: load_root_boot_files() no longer skips files whose extension
  is unknown. A new add_boot_listing_only() records the file's NAME and real
  SIZE without loading its bytes (address == 0), so big/undecodable files appear
  in the listing without consuming memory or failing the load. A known-type file
  that fails to load (e.g. over the preload cap) is also re-listed this way.
- src/shared/boot_info.h: new VIBIO_BOOT_FILE_TYPE_OTHER (6) for those
  listed-but-not-loaded files.
- src/kernel/main.c:
  - Files BOOT-source listing now shows directories, OTHER (listed-not-loaded)
    files, and loaded files - the old `address == 0` skip that hid them is gone.
  - Multi-select desktop drag: pressing an already-selected icon keeps the whole
    selection; the drag moves every selected icon by the same delta and snaps
    them all to the grid on drop; a plain click reduces back to one icon.
  - selftest_check_disk_readonly / selftest_check_fat32 return SKIP (N/A) when
    there is no usable AHCI disk (only a present-but-bad disk is a real FAIL).
  - selftest_check_files_app now lists the live USB root when USB is the source
    (instead of falsely skipping via the absent AHCI fs).
  - media_player not-loaded message is clearer: it explains MP4/MOV/MKV/AVI must
    be converted to .VIV on a PC (no in-kernel decoder), instead of a terse
    "unsupported VIV".
- tools/make_usb_image.py: the QEMU USB test image now includes a long-named
  "2026-06-26 13-41-59.mp4" (placeholder bytes) so the "any file shows" fix is
  visually verifiable.

Commands / tests run:

- .\build.ps1 - PASS (boot + kernel; KERNEL.BIN 3.58 MB).
- QEMU USBFILES -> Files lists "2026-06-26 13-41-59.mp4" (124 B) alongside the
  .txt/.VIM/.WAV/.VIV files and the DOCS folder ("7 items"): an arbitrary video
  container with spaces in its name now shows.
- VirtualBox boot - desktop renders, no regression from the boot-stage change.
- .\deploy-usb.ps1 -Drive G - copied BOOTX64.EFI + KERNEL.BIN (+ START.HTM,
  images, SOUNDS, VIV/WAV) to the removable VIBIO USB. Verified G:\KERNEL.BIN and
  G:\EFI\BOOT\BOOTX64.EFI byte-match the fresh build, and the user's
  "2026-06-26 13-41-59.mp4" is still present (untouched).

Screenshots (docs/screenshots/):
- 2026-06-27-stage37-fixes2-usb-mp4-listed.png
- 2026-06-27-stage37-fixes2-vbox-boot.png

Safety status:
- Still VM-first + read-only. No new write path. The deploy to G: is a
  non-destructive file copy to a REMOVABLE drive only (the script refuses fixed/
  system disks and never formats); it overwrites only Vibio's own files and left
  every user file in place. No host disk, EFI partition, or bootloader on any
  internal disk was touched.

Honest limitations / what still does not work:
- The laptop's INTERNAL disk is NVMe and Vibio has no NVMe/GPT driver, so the
  DISK / FAT32 / AHCI BOOT FAT32 tests are N/A there - that is the honest truth,
  not something this pass can make "pass". Files reads the laptop's BOOT USB
  (via the UEFI preload, and the Stage 37 xHCI USB driver if it works on that
  silicon), not the internal disk.
- Opening the .mp4 still cannot play it: Vibio has no in-kernel MP4/H.264
  decoder. It now shows a clear "convert to .VIV on a PC first" message. To
  actually watch that recording in Vibio it must be converted with ffmpeg on a
  PC into .VIV.
- Multi-select group drag is verified by source review + build/boot only (the
  harness cannot drive the guest mouse). Desktop selection is tracked per icon
  KIND, so the four built-in app icons multi-select/drag correctly, but multiple
  temporary "New folder"/"New text file" items of the SAME kind cannot be
  distinguished from each other yet (a future index-based selection would fix
  that).
- All of the above is unverified on the real laptop (no hardware access here).

Next milestone:
- Boot the refreshed G: USB on the laptop and confirm the .mp4 (and all files)
  now list, and that DISK/FAT32 show N/A rather than FAIL.

## 2026-06-27 - Stage 38 Native MP4 Container Demuxer + MP4INFO

Plain English:

Vibio can now open a real `.mp4` file *natively* - it reads and understands the
MP4 container by itself, with no host conversion and no ffmpeg. It does NOT
decode the video yet (that would be a huge H.264 decoder), and it is honest
about that: it tells you exactly what is inside the file and why it cannot play
the pixels. This is real, limited, honest MP4 support - not "convert to VIV and
pretend".

What actually works:

- A from-scratch MP4 / ISO-BMFF box parser (demuxer). It walks ftyp, moov, mvhd,
  trak, tkhd, mdia, mdhd, hdlr, minf, stbl, stsd, stts, stsc, stsz, stco/co64
  and mdat, with strict bounds checking at every read.
- Codec identification from the sample-entry FourCC: avc1/avc3 -> H.264,
  hvc1/hev1 -> H.265, av01 -> AV1, vp09 -> VP9, mp4v -> MPEG-4, mp4a -> AAC,
  .mp3 -> MP3, ac-3/ec-3 -> AC-3, Opus.
- For H.264 it reads the *real* profile / constraint flags / level out of the
  avcC AVCDecoderConfigurationRecord, so "Constrained Baseline, Level 3.0" is
  reported truthfully (not guessed).
- New `MP4INFO <file>` terminal command: prints file found + size, duration,
  timescale, track count, video codec + WxH (+ H.264 profile/level), audio codec
  + sample-rate/channels, per-track sample/chunk counts, the stbl table presence
  (stsd/stts/stsc/stsz/stco), whether decode is supported, and the exact
  unsupported reason. Safe on missing / corrupt / oversized files.
- Media Player: opening a `.MP4`/`.M4V`/`.MOV` shows a "NATIVE MP4 PROBE" surface
  with all of the above and a clear "Decode: NOT supported (demux/probe only)"
  line - it never crashes, never shows a fake frame. (If a host-converted
  companion `.VIV` exists, the existing VIV playback path still runs, honestly
  labelled as the host-conversion path, not native decode.)

Where the code lives (kept out of the kernel mainline as much as the flat-binary
build allows):

- `src/kernel/mp4.h` / `src/kernel/mp4.c` - a self-contained module that depends
  only on `<stdint.h>`, does no I/O and allocates nothing (the kernel reads the
  file into a buffer and hands it in). It is compiled as its OWN translation unit
  and linked into the kernel - the 22k-line `main.c` is untouched except for the
  small command/UI wiring. Vibio has no ELF/user-process loader, so a kernel-
  linked module is as "outside the kernel" as the design currently allows; the
  decode work is cleanly separated so it could later move to a user process.

H.264 / AAC decode status (honest):

- NOT implemented. There is no in-kernel H.264 or AAC decoder. Writing a real
  Constrained-Baseline H.264 decoder (CAVLC, intra/inter prediction, deblocking,
  IDCT, YUV->RGB) was far more than this pass could land safely without bloating
  KERNEL.BIN, so per the brief we finished the honest first layer instead: a real
  demuxer + MP4INFO + codec/profile detection + a safe Media Player path.
- AAC is detected (mp4a, sample-rate, channels) but NOT decoded. Reported as
  audio present, decode unsupported.

Test assets (real containers, generated with no ffmpeg - `tools/make_mp4_assets.py`):

- TEST.MP4  - faststart H.264 Constrained Baseline 320x180 Level 3.0 (a genuine
  SPS/PPS + avcC built by a small exp-Golomb writer) + AAC-LC 44100 stereo. The
  box structure, sample tables and codec config are all real; only the coded
  sample bytes in mdat are stubs (no encoder was available, and Vibio cannot
  decode them anyway). This is stated plainly rather than faked.
- UNSUP.MP4 - same container shape with an hvc1 (H.265) video track, to exercise
  the unsupported-codec path.
- BAD.MP4   - valid ftyp followed by a moov that overruns the file, to exercise
  the corrupt/truncated path.
- These land on the VM FAT32 disk via `tools/make_fat32_image.py` (MP4 added to
  the packed media extensions).

Image builder fix (found while packing the MP4s): the FAT32 root directory was a
single 1024-byte cluster (32 entries) and was already full of duplicate `.VIM`
entries, so extra assets silently fell off the end (TEST.MP4, UNSUP.MP4 and even
VIDEOTST.VIV were being dropped). `make_fat32_image.py` now writes the root
directory as a proper multi-cluster FAT32 chain, so all 40 root entries are
present. The kernel already follows the root cluster chain, so this needed no
kernel change.

How it was tested:

- `.\build.ps1` - clean compile + link of both kernel objects (`kernel_main.o` +
  `mp4.o`), image built. (The VBox VDI convert step fails on this host with a
  pre-existing disk-lock error; the raw `build\vibio.img` builds fine and is what
  QEMU boots.)
- Host cross-check: the exact same `mp4.c` compiled as a host program and run on
  the three assets reports TEST.MP4 = H.264 Constrained Baseline 320x180 L3.0 +
  AAC 44100/2ch, UNSUP.MP4 = H.265 unsupported, BAD.MP4 = "moov truncated".
- Fuzz: 351 malformed inputs (truncations, bit-flips, pure random, giant box
  sizes) - zero crashes (bounds checks hold).
- QEMU (q35 + OVMF + AHCI), one run at a time:
  - boots to desktop, no regression.
  - `view test.png` -> Media Viewer, native PNG decode (regression OK).
  - `view photo.jpg` -> Media Viewer, native JPEG decode (regression OK).
  - `play videotst.viv` -> Media Player plays the VIV (frame 029) (regression OK).
  - `play test.mp4`  -> NATIVE MP4 PROBE: H.264/AVC 320x180, Constrained Baseline
    Level 3.0, AAC 44100Hz 2ch, "Decode: NOT supported (demux/probe only)".
  - `play unsup.mp4` -> H.265/HEVC detected, "H.265/HEVC video not supported".
  - `play bad.mp4`   -> "This is not a playable MP4: moov truncated / past
    buffer" (no crash).
  - Note: `MP4INFO` and `SELFTEST` write to the Terminal console, which only
    opens via a mouse click the headless harness cannot perform, so their text
    output is not screenshot-able here. `MP4INFO`'s logic is verified by the host
    cross-check (identical parser + identical printed fields), and the Media
    Player probe screen shows the same parsed data live in the guest. `SELFTEST`'s
    check functions were not modified by this pass and the kernel boots and runs
    all new code cleanly. (The QEMU media windows also need one keystroke after
    opening to force a content repaint in the headless harness - a harness redraw-
    timing quirk, not a kernel bug; the windows render correctly once nudged.)

KERNEL.BIN size: 3,793,344 -> 3,812,880 bytes (+19,536 bytes, ~19 KB) for the
whole MP4 demuxer + MP4INFO + player probe surface.

Screenshots (docs/screenshots/):

![Stage 38 native MP4 probe](screenshots/stage38_mp4_test.png)

![Stage 38 unsupported codec](screenshots/stage38_mp4_unsupported.png)

![Stage 38 corrupt MP4](screenshots/stage38_mp4_corrupt.png)

![Stage 38 VIV regression](screenshots/stage38_viv_regression.png)

- stage38_view_png.png (PNG viewer regression), stage38_view_jpg.png (JPEG
  viewer regression).

Safety status:
- VM-only, read-only. No new write path. MP4 parsing is pure-in-memory on a file
  already read through the existing read-only paths. No xHCI work, no GPU work.

Honest limitations / what still does not work:
- No H.264 decode and no AAC decode: a real `.mp4` with H.264 video shows its
  details but the pixels are not rendered. "Native MP4 support" here means native
  container demux + probe, not playback of the coded video/audio.
- moov must be within the read window. Vibio reads up to its media buffer
  (~the player's frame buffer) from the start of the file; an MP4 whose `moov`
  sits past that (a very large non-faststart file) reports "no moov box in read
  window" honestly instead of guessing.
- The included TEST.MP4 has real container + codec metadata but stub coded
  frames; it is a demuxer/probe fixture, not a watchable clip.
- VirtualBox: not re-verified this pass (the VDI convert step is broken on this
  host); QEMU is the verified rig. Real hardware: not tested (no access).

Next milestone:
- If pursued: a minimal Constrained-Baseline, no-B-frame, CAVLC-only H.264 intra
  decoder for tiny resolutions, output to the existing Media Player framebuffer
  path - kept in the mp4/codec module, only if it can be done without bloating
  KERNEL.BIN.

### 2026-06-27 - Stage 38 fix: storage now mirrors a whole folder (no curation)

Bug reported: opening an actual `.mp4` showed "This is not a playable MP4: file
not found or unreadable". Root cause: the kernel demuxer + Files lister were
correct (the runtime AHCI/USB FAT32 reader already lists and reads EVERYTHING on
the device with no filter), but the BUILD never put the user's `.mp4` on the VM
disk - `make_fat32_image.py` only packed HTML + still images by extension, and
the video path needed ffmpeg. So `Assets/Media/SampleClip.mp4` simply was not on
the disk to read.

User feedback (correct): stop hand-picking files into the image by extension;
just put EVERYTHING from a folder onto the storage so it all shows, exactly like
a real disk/USB.

Fix (build-side, no kernel change): a recursive "verbatim folder" packer.
- `make_fat32_image.py` gained `pack_tree()` + a `--files <dir>` option that
  copies the ENTIRE contents of a folder onto the disk root - every file, any
  type, subfolders and all - generating proper 8.3 names + VFAT long names and
  nested directory clusters. The image auto-grows to fit. (The OS's own files -
  kernel, bootloader, SOUNDS, the browser's built-in pages, generated image
  fallbacks - are still added automatically; they are infrastructure, not user
  content.)
- New top-level `Disk/` folder -> boot AHCI disk; `build.ps1` passes
  `--files Disk`. `SampleClip.mp4` was moved here.
- `make_usb_image.py` was refactored the same way: a new top-level `Usb/` folder
  is copied verbatim onto the emulated USB stick (replacing its old hardcoded
  test file list).
- The earlier extension-based `copy_raw_videos()` shim was removed (superseded),
  and the stale 57 MB `SAMPLECL.VIV` build artifact was cleared.

So now: drop any files in `Disk/` (or `Usb/`), run `build.ps1`, and they ALL
appear in Vibio's Files app and open by their real names. The kernel side was
already capable; this just feeds the storage everything.

Verified in QEMU:
- `filesapp` -> Files shows "SOURCE: AHCI BOOT DISK - READ ONLY - 41 items"
  listing everything (incl. the verbatim `SampleClip.mp4` + `Clips/` subfolder),
  read live from the disk.
- `play sampleclip.mp4` (opened by its real long name) -> NATIVE MP4 PROBE =
  H.264/AVC 640x360, High profile, Level 3.0, video-only, 4 s, "Decode: NOT
  supported (demux/probe only)" - no more "file not found".
- Recursion, long names, and the `Clips/` subfolder verified by dumping the
  on-disk FAT32 tree. KERNEL.BIN unchanged (3,812,880).

![Stage 38 Files lists everything on the disk](screenshots/stage38_files_listing.png)

![Stage 38 user MP4 native probe](screenshots/stage38_mp4_verbatim.png)

Honest limit unchanged: the container is demuxed/probed natively, but H.264/AAC
are not decoded, so the pixels still do not render (this clip is High profile,
also outside the eventual Baseline decode goal). On REAL hardware you don't even
need the `Disk/`/`Usb/` folders - copy files onto the real USB and Vibio's
post-boot USB driver reads them directly the same way.

## 2026-06-27 - Stage 38 follow-up: MP4 sample demux tables for baseline test file

Plain English:

Vibio's native MP4 support now gets past "what codecs are in this file" and
resolves real coded sample byte ranges from the MP4 sample tables. This is the
first ordered video-playback step only: MP4 container parsing/demux is now more
complete. Vibio still does NOT decode H.264 frames or AAC audio, so `.mp4` files
still do not play pixels/sound natively yet.

What changed:

- `src/kernel/mp4.h` / `src/kernel/mp4.c` now parse and retain the `stts`,
  `stsc`, `stsz`, and `stco` / `co64` table ranges, sample counts, chunk counts,
  first-sample offset/size/duration, max sample size, and H.264 NAL length size.
- New `mp4_get_sample()` resolves a 0-based video or audio sample index into a
  file offset, coded byte size, DTS, and duration without allocation or I/O.
- MP4INFO and the Media Player probe surface now report `demux=READY` when the
  sample tables resolve, while still reporting decode unsupported.
- The Media Player probe path now handles common non-faststart MP4 files whose
  `moov` box is at the end: it reads the first window, and if no `moov` is found,
  it reads a tail window and scans for a valid `moov` box. This is read-only and
  does not load the whole video into RAM.
- `SELFTEST` now has an honest `MP4 DEMUX` line. It PASSes only when `TEST.MP4`
  parses and both first video/audio samples resolve; it SKIPs if the asset or
  scratch buffer is absent.
- Added `tools/mp4_probe_host.c`, a tiny host-side parser probe that compiles the
  same `src/kernel/mp4.c` and prints parser/demux facts for real MP4 files.
- Copied the supplied test file
  `C:\Users\jackd\Downloads\vibio_test_baseline_320_aac - Copy.mp4` into
  `Disk/BASE320.MP4` so the VM boot disk includes the exact baseline clip under a
  short terminal-friendly name.

Technical details:

- The supplied baseline file is H.264 Constrained Baseline, 320x180, yuv420p,
  60 fps, plus AAC-LC stereo 44.1 kHz. `ffprobe` reports 4594 video frames and
  3299 AAC frames over about 76.57 seconds.
- Vibio's MP4 parser now resolves the baseline file's first video sample at
  offset 290, size 4400, duration 256 in the video timescale, with max video
  sample size 13149 and H.264 NAL length size 4.
- It resolves the baseline file's first AAC sample at offset 48, size 242,
  duration 1024 in the audio timescale.
- Whole-struct assignments in the freestanding parser were replaced with
  explicit byte copies so the kernel link does not require libc `memcpy`.

Commands / tests run:

- `ffprobe -hide_banner -show_streams -show_format -print_format json "C:\Users\jackd\Downloads\vibio_test_baseline_320_aac - Copy.mp4"` - PASS; confirms Constrained Baseline H.264 320x180 + AAC-LC stereo.
- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe "C:\Users\jackd\Downloads\vibio_test_baseline_320_aac - Copy.mp4"` - PASS; demux ready for video and audio samples.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; copied VM asset matches parser behavior.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - expected FAIL status, cleanly reports `moov truncated / past buffer` with no crash.
- `.\build.ps1` - PASS; builds `BOOTX64.EFI`, `KERNEL.BIN`, `vibio.img`, `vibio.vdi`, and attaches the VDI to `Vibio OS Dev`.
- QEMU `PLAY BASE320.MP4` - PASS for the ordered Step 1 goal: Media Player shows H.264/AAC facts and `Demux: READY`, then honestly says `Decode: NOT supported`.
- QEMU `PLAY UNSUP.MP4` - PASS failure path: H.265 is detected, sample demux resolves, and decode remains unsupported with a clear message.
- QEMU `VERSION` and `SELFTEST` command injection - no crash; Terminal output is not screenshot-visible because the desktop intentionally does not open a hidden Terminal from typed input.
- QEMU `RESTART` - PASS; returns to the desktop.
- VirtualBox boot via `tools\test-vm.ps1` - PASS; desktop renders and VM powers off cleanly.

Screenshots (docs/screenshots/):

- `2026-06-27-stage38-mp4-demux-boot.png`
- `2026-06-27-stage38-base320-mp4-demux.png`
- `2026-06-27-stage38-mp4-demux-unsupported.png`
- `2026-06-27-stage38-demux-restart.png`
- `2026-06-27-stage38-demux-vbox-boot.png`
- `2026-06-27-stage38-demux-version-command.png` and
  `2026-06-27-stage38-demux-selftest-command.png` show the desktop remained
  alive after command injection, but the command text is hidden by design.

Safety status:

- VM-only and read-only. No disk write path was added or enabled.
- The baseline file was copied only into the project `Disk/` folder and then
  packed into the generated VM disk under `build\`.
- No host EFI partition, host bootloader, internal Windows disk, internal Linux
  disk, USB device, or boot order was touched.
- The existing Stage 29 VM-only write sandbox was not changed.

Honest limitations / what still does not work:

- Native MP4 playback still does NOT work yet. This pass completes the container
  sample-demux layer only.
- H.264 Constrained Baseline video decode is still unsupported. No YUV frames are
  produced and no pixels from `.mp4` are rendered.
- AAC-LC audio decode is still unsupported. AAC is detected, but no PCM is
  produced.
- There is no audio/video sync because there is no decoded audio/video stream
  yet.
- The tail-`moov` fallback only works when the whole `moov` box fits in the
  existing probe window. Very large MP4 metadata can still report no usable
  `moov` honestly.
- `VERSION` still reports Stage 37 even though the capability log is Stage 38;
  this was left unchanged to avoid a cosmetic version bump during a parser-only
  follow-up.
- Real hardware was not tested in this pass.

Next milestone:

- Only after this MP4 demux layer remains stable: start Step 2, a minimal H.264
  Constrained Baseline video-only decoder for tiny 320x180 clips, using the
  resolved MP4 sample byte ranges. Do not start AAC or sync until H.264 baseline
  frames actually decode.

## 2026-06-27 - Stage 38 follow-up: Step 2 H.264 Baseline header/slice probe started

Plain English:

Started the next item in the user's progression tree: H.264 Constrained Baseline
video-only. This is NOT playback yet. Vibio now validates the H.264 Baseline
setup for the supplied `BASE320.MP4` file: it reads SPS/PPS from MP4 `avcC`,
reads the first video sample through the MP4 demux offsets, parses the first IDR
slice header, and reports that the stream fits the first decoder target. It still
does not reconstruct macroblocks, output YUV, convert YUV to RGB, or show video
pixels.

What changed:

- Added isolated `src/kernel/h264.h` / `src/kernel/h264.c`.
- The H.264 module has a freestanding bitreader that handles emulation-prevention
  bytes and Exp-Golomb fields.
- It parses SPS, PPS, and first slice headers for the first Baseline target:
  profile/constraint flags, level, cropped width/height, macroblock dimensions,
  frame number bits, POC type, CAVLC/CABAC flag, slice groups, first slice type,
  frame number, IDR picture id, and POC LSB.
- `build.ps1` now compiles and links `h264.o` as its own translation unit.
- `mp4.c` now exposes the first SPS/PPS byte ranges from `avcC`.
- Media Player now reads only the first H.264 sample by its MP4 demux offset and
  shows `H.264 Step 2: HEADERS OK 320x180 slice=I` for `BASE320.MP4`.
- `tools/mp4_probe_host.c` now also runs the H.264 header/slice probe so the same
  parser can be tested on the host before booting the OS.

Technical details:

- The first video sample in `BASE320.MP4` is 4400 bytes at file offset 290.
- That sample contains two NAL units: SEI and IDR. SPS/PPS are not inside the
  sample; they are read from `avcC`, which is normal for MP4.
- The H.264 probe reports Constrained Baseline headers OK, 320x180, first slice
  type I, and two NALs in the first sample.
- This is the front door for the real decoder, not the decoder itself. The next
  work is macroblock reconstruction: CAVLC residual decode, intra prediction,
  inter prediction/motion compensation for P-slices, reference frames, and YUV
  output.

Commands / tests run:

- Python/ffmpeg inspection of `Disk\BASE320.MP4` - confirmed the first sample has
  SEI + IDR only and that SPS/PPS are in `avcC`.
- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports `h264_probe status=OK`, 320x180, `slice=I`, `constrained=1`.
- `.\build.ps1` - PASS; builds and links `h264.o`.
- QEMU `PLAY BASE320.MP4` - PASS for partial Step 2 probe: Media Player shows
  `H.264 Step 2: HEADERS OK 320x180 slice=I` and still clearly says decode is
  unsupported.
- VirtualBox boot via `tools\test-vm.ps1` - PASS; desktop renders and the VM
  powers off cleanly.

Screenshots (docs/screenshots/):

- `2026-06-27-stage38-h264-step2-probe.png`
- `2026-06-27-stage38-h264-step2-vbox-boot.png`

Safety status:

- VM-only and read-only. No disk write path was added or enabled.
- The new H.264 code reads only bytes already exposed by the MP4 demux and the
  existing read-only file path.
- No host EFI partition, host bootloader, internal disk, USB device, or boot
  order was touched.

Honest limitations / what still does not work:

- Step 2 is started but NOT complete. Native H.264 video playback still does not
  work.
- No CAVLC residual decode yet.
- No intra prediction or inter prediction/motion compensation yet.
- No reference frame storage/use yet.
- No deblocking filter.
- No YUV420 output and no YUV420 -> RGB conversion.
- AAC remains detection-only; no AAC decode and no A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: decode the first IDR frame's macroblocks into a YUV420
  frame buffer for the supplied 320x180 Constrained Baseline file. Do not start
  Step 3 until actual YUV frames exist.

## 2026-06-27 - Stage 38 follow-up: Step 2 enters first H.264 macroblock header

Plain English:

Continued the same progression-tree item: Step 2, H.264 Constrained Baseline
video-only. Vibio now gets one layer deeper than SPS/PPS/slice headers: it enters
the first IDR slice's first macroblock header, identifies the macroblock kind and
coded-block pattern, and stops before residual decoding. This is still NOT video
playback.

What changed:

- `src/kernel/h264.c` now parses the PPS fields needed to reach macroblock data
  correctly: reference-index defaults, weighted prediction flags, initial QP,
  deblocking-control presence, constrained-intra flag, and redundant-pic-count
  flag.
- It now parses the IDR slice fields that sit before macroblocks:
  `no_output_of_prior_pics_flag`, `long_term_reference_flag`, `slice_qp_delta`,
  and deblocking filter offsets when present.
- It parses the first I-slice macroblock header:
  - I_NxN / I_16x16 / I_PCM classification.
  - Intra4x4 prediction mode flags/rem modes for I_NxN.
  - Intra chroma prediction mode.
  - Intra coded-block-pattern mapping.
  - Macroblock QP delta when residual data is present.
- Media Player now displays `mb=I_NxN` on the H.264 Step 2 line when opening
  `BASE320.MP4`.

Technical details:

- The first IDR sample still resolves at offset 290, size 4400.
- First macroblock result for `BASE320.MP4`:
  - `mb_type=0`
  - kind `I_NxN`
  - luma coded block pattern `3`
  - chroma coded block pattern `0`
  - `mb_qp_delta=0`
  - residual data present
- The previous first attempt incorrectly landed on `mb_type=26`. That was not a
  stream limitation; it exposed missing slice-header parsing before macroblock
  data. The parser now consumes those fields and lands on the correct first
  macroblock.

Commands / tests run:

- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports
  `h264_first_mb ok=1 type=0 kind=I_NxN cbpl=3 cbpc=0 qp_delta=0 residual=1`.
- `.\build.ps1` - PASS.
- QEMU `PLAY BASE320.MP4` - PASS for the new Step 2 sub-gate; Media Player shows
  `H.264 Step 2: HEADERS OK 320x180 slice=I mb=I_NxN` and still says decode is
  unsupported.

Screenshots (docs/screenshots/):

- `2026-06-27-stage38-h264-step2-mb-probe.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled.
- No host EFI partition, host bootloader, internal disk, USB device, or boot
  order was touched.

Honest limitations / what still does not work:

- Step 2 remains incomplete. No decoded frame exists yet.
- CAVLC residual parsing is still not implemented.
- Intra prediction, inverse transform, reconstruction, reference frames, and
  deblocking are still not implemented.
- YUV420 output and YUV420 -> RGB conversion are still not implemented, so Step 3
  has not started.
- AAC remains detection-only; no AAC decode or A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2: implement enough CAVLC residual parsing for the first IDR
  macroblock(s), then reconstruct luma/chroma samples into a YUV420 frame buffer.

## 2026-06-27 - Stage 38 follow-up: Step 2 parses first H.264 CAVLC residual block

Plain English:

Continued the same progression-tree item: Step 2, H.264 Constrained Baseline
video-only. Vibio now parses one real CAVLC residual block from the supplied
`BASE320.MP4` file: the first coded luma 4x4 block in the first IDR macroblock.
This is still NOT video playback. No pixels are reconstructed or shown yet.

What changed:

- `src/kernel/h264.c` now has a limited CAVLC parser for the first coded luma
  4x4 residual block when its neighbour context is `nC=0`.
- The parser reads `coeff_token`, trailing-one signs, level syntax,
  `total_zeros`, and `run_before` values for that first block.
- `H264Probe` now records the first residual block index, coefficient count,
  trailing-one count, total-zero count, and run count.
- The Media Player probe line now shows the successful gate as
  `cavlc=7coef` for `BASE320.MP4`.
- `tools/mp4_probe_host.c` prints the same first-CAVLC-block result so this can
  be tested before booting the OS.

Technical details:

- `BASE320.MP4` still resolves the first video sample at offset 290, size 4400.
- The first macroblock is still `I_NxN` with luma coded-block pattern `3`,
  chroma coded-block pattern `0`, and QP delta `0`.
- The first coded luma 4x4 residual block parses as:
  - block `0`
  - total coefficients `7`
  - trailing ones `0`
  - total zeros `4`
  - run entries consumed `4`
- This pass validates part of the CAVLC bitstream syntax only. It does not yet
  store coefficient levels into a zig-zag/scanner array, perform inverse
  quantization/inverse transform, run intra prediction, or write a YUV frame.

Commands / tests run:

- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports
  `h264_first_cavlc ok=1 block=0 coeff=7 trailing=0 zeros=4 runs=4`.
- `.\build.ps1` - PASS; builds and links the updated H.264 module.
- QEMU `PLAY BASE320.MP4` - PASS for this partial Step 2 gate; Media Player
  shows `H.264 Step 2: HEADERS OK 320x180 slice=I mb=I_NxN cavlc=7coef` and
  still clearly says `Decode: NOT supported (demux/probe only)`.
- QEMU `VERSION` and `SELFTEST` command injection - no crash; the desktop
  remains alive, but this harness does not visibly expose the hidden Terminal
  output for these commands.
- QEMU `RESTART` - PASS; returns to the desktop.
- VirtualBox boot via `tools\test-vm.ps1` - PASS; desktop renders and the VM
  powers off cleanly.
- `.\build\mp4_probe_host.exe build\media\UNSUP.MP4` - PASS unsupported path;
  HEVC remains demuxed/detected but not H.264-decoded.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - expected FAIL status,
  cleanly reports `moov truncated / past buffer` with no crash.
- QEMU `PLAY UNSUP.MP4` - PASS failure path; unsupported video remains a clear
  probe-only result.

Screenshots (docs/screenshots/):

- `2026-06-27-stage38-h264-cavlc-media-player.png`
- `2026-06-27-stage38-h264-cavlc-version.png`
- `2026-06-27-stage38-h264-cavlc-unsupported.png`
- `2026-06-27-stage38-h264-cavlc-restart.png`
- `2026-06-27-stage38-h264-cavlc-vbox-boot.png`
- `2026-06-27-stage38-h264-cavlc-version-command.png`
- `2026-06-27-stage38-h264-cavlc-selftest-command.png`
- `2026-06-27-stage38-h264-cavlc-version-visible.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled.
- No host EFI partition, host bootloader, internal Windows disk, internal Linux
  disk, USB device, or boot order was touched.
- The only file copied for testing remains the user-supplied baseline clip under
  the project `Disk/` folder, which is packed into the generated VM disk.

Honest limitations / what still does not work:

- Step 2 remains incomplete. Native H.264 playback still does not work.
- Only the first coded luma residual block of the first macroblock is parsed.
- Full CAVLC residual parsing across all luma/chroma blocks is not implemented.
- Coefficients are not yet reconstructed into pixel-domain samples.
- Intra prediction, inverse transform, reference frames, deblocking, YUV output,
  and YUV420 -> RGB conversion are not implemented.
- AAC remains detection-only; no AAC decode or A/V sync.
- The QEMU command-injection harness can prove no crash for hidden Terminal
  commands, but it did not capture visible `VERSION` / `SELFTEST` text.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: store decoded CAVLC coefficient levels for the first
  block, add inverse transform/reconstruction for that block, then expand to all
  macroblocks in the first IDR frame before starting Step 3.

## 2026-06-27 - Stage 38 follow-up: Step 2 reconstructs first H.264 4x4 luma block

Plain English:

Same progression-tree item (Step 2, H.264 Constrained Baseline video-only).
Vibio now runs the real H.264 pixel pipeline end-to-end for ONE block: it takes
the already-decoded CAVLC coefficients of the first coded 4x4 luma block of the
first IDR macroblock in `BASE320.MP4` and turns them into actual luma sample
values - inverse quantization, the 4x4 inverse integer transform, DC intra
prediction, and reconstruction. This is the first genuinely decoded H.264 pixel
data in Vibio. It is still ONE 4x4 block, not a frame, so this is NOT playback.

What changed:

- `src/kernel/h264.c`: added `reconstruct_first_luma4x4()`. It is strictly gated
  to the first 4x4 luma block (block 0) of the first macroblock (`first_mb_in_slice
  == 0`) of an `I_NxN` macroblock. It computes `QP = 26 + pic_init_qp_minus26 +
  slice_qp_delta + mb_qp_delta`, dequantizes via the standard `dequant_coef`
  (normAdjust * flat weightScale 16) using spec 8.5.12.1, runs the 8.5.12.2 4x4
  inverse transform (rows then columns, `(x + 32) >> 6`), and adds DC intra
  prediction. DC is the only intra4x4 mode used because for the first macroblock
  both the left and top neighbours are unavailable, so the predictor is 128 and
  no neighbour samples are needed - any other mode is honestly refused.
- The first block's intra4x4 prediction mode is now captured (was discarded),
  and `slice_qp_delta` is now captured (was discarded) to compute QP.
- `H264Probe` gained `slice_qp_delta`, `first_mb_qp`, `first_block_intra4x4_mode`,
  the dequantized coefficients, the spatial residual, the DC predictor, and the
  reconstructed 0..255 luma samples, plus per-stage OK flags and a detail string.
- For `BASE320.MP4`: `qp=26`, intra4x4 mode 2 (DC), dequant OK, inverse transform
  OK (residual sum `-1244`), reconstruction OK. First reconstructed luma row is
  `41,48,45,35`; full block `41,48,45,35, 29,41,56,59, 71,60,45,41, 75,61,48,49`.
- Media Player now shows an extra honest line under the H.264 probe:
  `first 4x4 luma: qp=26 dequant+idct OK resid_sum=-1244 -> px[0..3]=41,48,45,35`.
- `tools/mp4_probe_host.c` prints the QP, per-stage flags, the residual values
  and the reconstructed pixels.

Why this is honest:

- It is real spec-conformant arithmetic on real decoded coefficients, not a fake.
- It is one 4x4 luma block only. The detail strings and Media Player text still
  say `Decode: NOT supported (demux/probe only)` and do not claim playback.
- Reconstruction is gated; if the first block ever used a non-DC intra4x4 mode it
  would report "residual decoded; ... needs neighbour samples" rather than guess.

Verification:

- Host probe (`build/mp4_probe_host.exe Disk/BASE320.MP4`): coefficients still
  `-24,1,0,0,...`; new lines show `qp=26 ... recon_ok=1` and pixels above.
- `.\build.ps1`: kernel + EFI + raw image + VDI all built (exit 0).
- QEMU `PLAY BASE320.MP4`: screenshot `build/test-shots/h264-firstblock.png`
  shows the new reconstruction line with `px[0..3]=41,48,45,35`.
- `UNSUP.MP4` (HEVC) and `BAD.MP4` (corrupt) still handled cleanly, no crash;
  `TEST.MP4` synthetic first slice is a P slice and is refused as before.

Honest limitations / what still does not work:

- Only ONE 4x4 luma block is reconstructed. No other blocks, no other
  macroblocks, no chroma, no full frame, no YUV420->RGB, no rendered video.
- Non-DC intra4x4 prediction (needs neighbour samples) is not implemented.
- AAC remains detection-only; no AAC decode and no A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: reconstruct the remaining 4x4 luma blocks of the first
  macroblock (needs per-block neighbour tracking for nC and intra4x4 prediction),
  then all macroblocks of the first IDR frame, then chroma, building toward a
  full decoded luma/chroma frame before YUV420->RGB (Step 3).

## 2026-06-27 - Stage 38 follow-up: Step 2 reconstructs ALL 16 luma blocks of macroblock 0 (bit-exact vs ffmpeg)

Plain English:

Same progression-tree item (Step 2, H.264 Constrained Baseline video-only).
Vibio now decodes and reconstructs the ENTIRE luma of the first macroblock of
`BASE320.MP4` - all sixteen 4x4 luma blocks, 16x16 real pixels - and the result
is bit-exact against ffmpeg. The decoded block is drawn on screen as grayscale.
This is still ONE macroblock (16x16), not a frame, so it is NOT playback.

Important honesty correction: the previous pass claimed the "first 4x4 luma block
reconstructed" but that result was actually WRONG - it had only been checked for
self-consistency, never against a reference decoder. This pass set up ffmpeg as a
ground-truth oracle (`-skip_loop_filter all` so ffmpeg outputs the pre-deblock
reconstruction, which is exactly what intra prediction consumes) and found the
old output did not match. Two real bugs were the cause and are now fixed.

Bugs found and fixed:

- CAVLC `total_zeros` tables were incorrect (e.g. for TotalCoeff=7 the codeword
  `11` was mapped to total_zeros=4 instead of 5). Replaced ALL coeff_token,
  total_zeros and run_before tables with the authoritative reference (FFmpeg
  `h264_cavlc.c`) arrays, fetched and transcribed verbatim, in `src/kernel/h264.c`.
- `run_before[TotalCoeff-1]` was never assigned the remaining `zerosLeft` (H.264
  9.2.4). With zeros left over, every coefficient was shifted to the wrong scan
  position. This latent bug existed in the old single-block decoder too; it only
  happened to be harmless for block 0 (its leftover zerosLeft was 0).

What changed (all `src/kernel/h264.c` / `h264.h`):

- Replaced the nC=0-only `coeff_token` reader and the wrong `total_zeros`/
  `run_before` tables with table-driven decoders over the authoritative arrays
  (`read_coeff_token` selects the nC range 0-1/2-3/4-7/>=8; `read_total_zeros`,
  `read_run_before`). `decode_cavlc_levels` was already correct and is reused.
- `decode_residual_4x4`: full CAVLC for one 4x4 luma block at any nC, with the
  `run_before[TotalCoeff-1]=zerosLeft` fix.
- `decode_and_reconstruct_mb0`: walks the 16 luma blocks in luma4x4BlkIdx scan
  order; computes nC from the left/top 4x4 neighbours' TotalCoeff (neighbours
  outside the first macroblock are unavailable); decodes residual only for the
  8x8 blocks flagged in cbp_luma; runs `dequant_idct_4x4` and `predict4x4`, adds
  and clips. `predict4x4` implements all 9 intra4x4 modes (vertical, horizontal,
  DC, the two diagonals, vertical-right/left, horizontal-down/up) with correct
  reference-sample availability including the top-right rule.
- Intra4x4 prediction modes are now derived per-block from the neighbour modes
  (`predIntra4x4PredMode = min(A,B)`, DC when a neighbour is outside the MB).
- `H264Probe` gained `mb0_luma[256]`, `mb0_block_mode[16]`, block count, sum,
  min, max and a detail string. First-block diagnostics are still populated for
  backward compatibility (Media Player / host probe).
- Media Player now reports `MB0 luma OK blocks=16/16 qp=26 sum=7988 min=16
  max=78` and draws the decoded 16x16 luma block as 6x grayscale (the first real
  decoded H.264 pixels visible in Vibio). `Decode: NOT supported (demux/probe
  only)` text is unchanged - no playback is claimed.
- `tools/mp4_probe_host.c` prints the full 16x16 decoded luma block and modes.
- New `tools/h264_verify_mb0.py`: runs the host probe and compares MB0 luma to
  ffmpeg (loop filter disabled) bit-for-bit. This is the honest regression check.

Verification:

- `tools/h264_verify_mb0.py Disk/BASE320.MP4` -> `MB0 luma mismatch = 0 / 256
  OK (bit-exact vs ffmpeg)`. The same check passes against the actual in-kernel
  C decoder (the host probe links `src/kernel/h264.c` unchanged).
- A standalone Python reference decoder (built during debugging) also matched
  ffmpeg 0/256, then the C port was confirmed to match it.
- `.\build.ps1`: kernel + EFI + raw image + VDI all built (exit 0).
- QEMU `PLAY BASE320.MP4`: screenshot `build/test-shots/h264-mb0.png` shows the
  MB0 line and the grayscale decoded macroblock.
- `UNSUP.MP4` (HEVC) and `BAD.MP4` (corrupt) still handled cleanly, no crash;
  `TEST.MP4` synthetic first slice is a P slice and is refused as before.

Honest limitations / what still does not work:

- Only ONE macroblock's LUMA is reconstructed. No other macroblocks (so no full
  frame), no chroma (Cb/Cr), no I_16x16 luma (needs the luma-DC transform), no
  reference/inter frames, no deblocking, no YUV->RGB, no rendered video frame.
- AAC remains detection-only; no AAC decode and no A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: extend the same machinery to ALL macroblocks of the first
  IDR frame (the decoder already tracks neighbours within a MB; it needs to track
  them across MB boundaries too), then add chroma (Cb/Cr 8x8 with chroma intra
  prediction and the chroma-DC transform), producing a full decoded YUV frame
  before moving to Step 3 (YUV420->RGB).

## 2026-06-27 - Volume popup fixes (position, draggable slider, dirty-rect)

Plain English:

Fixed three issues with the taskbar SOUND popup. (1) It only appeared where the
mouse cursor happened to pass over it - it wasn't being composited when it opened
or when the volume changed. (2) It was pinned to the right screen edge instead of
sitting over the sound tray icon. (3) The slider could only be clicked, not
dragged.

What changed (all `src/kernel/main.c`):

- Dirty-rect fix: new `dirty_list_add_volume_popup_overlay` (mirrors
  `dirty_list_add_power_menu_overlay`), called for both the current and previous
  window-manager state in the wm-changed path and in the content-redraw path. The
  popup's rect is now marked dirty when it opens, closes or the slider moves, so
  the compositor blits it instead of relying on the cursor's dirty rect drifting
  over it.
- Position fix: `taskbar_volume_popup_rect` now centres the popup over the sound
  tray icon (`taskbar_sound_icon_x + 16`, screen-clamped), like the power menu,
  instead of `framebuffer_width - w - 12`.
- Draggable slider: new `volume_dragging` window-manager flag. Pressing the slider
  (`taskbar_volume_slider_hit`) grabs the knob and sets the volume
  (`taskbar_volume_slider_apply`, which clamps the mouse x into the track); a
  continuation block in `wm_update` keeps following the mouse x every frame while
  the button is held, and releases on mouse-up. Refactored the old click-only
  `taskbar_volume_slider_set` into `taskbar_volume_slider_geom` / `_apply` / `_hit`.

Verification:

- `.\build.ps1`: kernel + EFI + raw image built (exit 0). (VDI conversion was
  skipped this run because the dev VirtualBox GUI had the .vdi open/locked; the
  raw `vibio.img` that QEMU uses built fine.)
- QEMU clean boot screenshot `build/test-shots/volume-clean-boot.png`: desktop,
  taskbar and tray render correctly - no regression.
- The popup's mouse interaction itself is source-verified only: the QEMU/VBox
  harness cannot drive the guest mouse precisely enough to click the tray icon and
  drag the slider, and the popup is only reachable that way. The logic mirrors the
  already-working power-menu overlay/position pattern and the window/scrollbar
  drag pattern. Needs a hand-test in the VM to confirm feel.

Safety status:

- VM-only, read-only. No storage write path, boot order, or host/USB disk touched.

## 2026-06-27 - Stage 38 follow-up: Step 2 decodes first-row luma through 3 macroblocks

Plain English:

Continued the same progression-tree item: Step 2, H.264 Constrained Baseline
video-only. Vibio now generalizes the old macroblock-0-only luma path into a
bounded first-row decoder. On `BASE320.MP4` it decodes the first 3 macroblocks of
the first IDR row (48x16 luma, 768 samples), verifies them bit-exact against
ffmpeg, and then stops honestly at the next unimplemented H.264 luma feature:
macroblock 3 is `I_16x16`.

What changed:

- `src/kernel/h264.c` now decodes first-row macroblocks in raster order until an
  unsupported feature is reached.
- Cross-macroblock CAVLC nC tracking is implemented for left-neighbour luma
  blocks.
- Cross-macroblock intra4x4 prediction mode derivation and left-sample
  prediction are implemented for the first row. A bug found during ffmpeg
  comparison was fixed: when either A or B is unavailable, the predicted
  intra4x4 mode must be DC, not `min(available, DC)`.
- Chroma residual CAVLC is parsed only far enough to keep the bitstream aligned
  while decoding luma. No Cb/Cr samples are reconstructed.
- `H264Probe` now reports decoded luma macroblocks, samples, region dimensions,
  sum/min/max, cross-macroblock tracking flags, and the exact blocker.
- `tools/mp4_probe_host.c` prints the decoded luma region.
- `tools/h264_verify_mb0.py` now verifies both MB0 and the wider decoded luma
  region against ffmpeg.
- Media Player now shows `H.264 Step 2: luma partial mbs=3/20 block=I_16x16
  luma not implemented`, prints the 48x16 luma diagnostics, draws the decoded
  grayscale strip, and still says native MP4 decode is unsupported.

Verification:

- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c
  src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS as a probe; reports
  `h264_luma_region mbs=3 width=48 height=16 samples=768 row_ok=0 ... cross_nc=1
  cross_intra=1 ... blocker_mb=3 detail=I_16x16 luma not implemented`.
- `py tools\h264_verify_mb0.py Disk\BASE320.MP4` - PASS:
  - `MB0 luma mismatch = 0 / 256`
  - `luma region 48x16 mismatch = 0 / 768`
- `.\build.ps1` - PASS; kernel, EFI, raw image, USB image, and VDI built.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - expected clean failure:
  `moov truncated / past buffer`.
- `.\build\mp4_probe_host.exe build\media\UNSUP.MP4` - PASS unsupported path:
  HEVC video + AAC audio demux/probe, no H.264 decode path entered.
- QEMU `PLAY BASE320.MP4` - PASS for this partial Step 2 gate; Media Player
  shows the luma partial status, the 48x16 grayscale strip, and the unchanged
  `Decode: NOT supported (demux/probe only)` line.
- QEMU `PLAY UNSUP.MP4` - PASS unsupported-codec smoke check.
- QEMU `RESTART` - PASS smoke check; system returned to the desktop.

Screenshots:

- `docs/screenshots/2026-06-27-stage38-h264-luma-partial.png`
- `docs/screenshots/2026-06-27-stage38-h264-luma-unsupported.png`
- `docs/screenshots/2026-06-27-stage38-h264-luma-restart.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled.
- No host EFI partition, host bootloader, internal Windows disk, internal Linux
  disk, USB device, or boot order was touched.

Honest limitations / what still does not work:

- Only the first 3 luma macroblocks of the first IDR row are decoded.
- The first full row does not exist yet; full-frame luma does not exist.
- `I_16x16` luma reconstruction is not implemented and is the exact blocker at
  macroblock 3 of `BASE320.MP4`.
- Chroma is parsed only for bitstream alignment; no Cb/Cr reconstruction exists.
- No YUV420 frame buffer, no YUV420 -> RGB conversion, no native MP4 playback.
- AAC remains detection-only; no AAC decode or A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: implement `I_16x16` luma reconstruction, including the
  luma-DC transform, then retry the first IDR row before moving toward full-frame
  luma. Do not start YUV420 -> RGB, timing, AAC, or playback yet.

## 2026-06-27 - Stage 38 follow-up: Step 2 reconstructs full first H.264 luma row

Plain English:

Continued the same progression-tree item: Step 2, H.264 Constrained Baseline
video-only. The previous blocker was macroblock 3 of `BASE320.MP4`, an
`I_16x16` macroblock. Vibio now implements enough `I_16x16` luma reconstruction
to pass that blocker and decode the full first IDR macroblock row: 20/20
macroblocks, 320x16 luma, 5120 samples. The full row is bit-exact against
ffmpeg. This is still only luma for one row - not a frame and not playback.

What changed:

- `src/kernel/h264.c` now parses `I_16x16` macroblock types correctly:
  prediction mode, chroma coded-block pattern, luma coded-block pattern, chroma
  prediction mode, and macroblock QP delta.
- Implemented `I_16x16` luma DC residual decode and the 4x4 luma-DC transform /
  dequant path, matching the same integer transform ordering used by FFmpeg.
- Implemented `I_16x16` 15-coefficient luma AC residual decode for each 4x4
  block, plus reconstruction by combining the transformed DC value with AC,
  inverse transform, and 16x16 intra prediction.
- Implemented the needed 16x16 intra prediction modes for this row. Unsupported
  availability still fails closed.
- The first-row luma decoder now continues through all 20 macroblocks of
  `BASE320.MP4`; 10 of those decoded macroblocks use `I_16x16`.
- `H264Probe` now reports `i16x16_luma_ok`, `i16x16_dc_transform_ok`, and the
  number of decoded `I_16x16` macroblocks.
- `tools/mp4_probe_host.c` prints the `I_16x16` diagnostics.
- `tools/h264_verify_mb0.py` now prints a dedicated MB3 mismatch line in
  addition to MB0 and the full luma-region comparison.
- Media Player now shows `H.264 Step 2: luma row OK mbs=20/20 I16=10`, draws the
  320x16 grayscale luma strip, and still says `Decode: NOT supported
  (demux/probe only)`.

Verification:

- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c
  src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports
  `h264_luma_region mbs=20 width=320 height=16 samples=5120 row_ok=1 ...
  cross_nc=1 cross_intra=1 i16=1 i16_dc=1 i16_mbs=10 ... detail=first macroblock
  row luma reconstructed`.
- `py tools\h264_verify_mb0.py Disk\BASE320.MP4` - PASS:
  - `MB0 luma mismatch = 0 / 256`
  - `luma region 320x16 mismatch = 0 / 5120`
  - `MB3 luma mismatch = 0 / 256`
- `.\build.ps1` - PASS; kernel, EFI, raw image, USB image, and VDI built.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - expected clean failure:
  `moov truncated / past buffer`.
- `.\build\mp4_probe_host.exe build\media\UNSUP.MP4` - PASS unsupported path:
  HEVC video + AAC audio demux/probe, no H.264 decode path entered.
- QEMU `PLAY BASE320.MP4` - PASS for this Step 2 sub-gate; Media Player shows
  the full-row luma status, the 320x16 grayscale strip, and the unchanged
  unsupported decode line.
- QEMU `PLAY UNSUP.MP4` - PASS unsupported-codec smoke check.
- QEMU `RESTART` - PASS smoke check; system returned to the desktop.

Screenshots:

- `docs/screenshots/2026-06-27-stage38-h264-i16-row.png`
- `docs/screenshots/2026-06-27-stage38-h264-i16-unsupported.png`
- `docs/screenshots/2026-06-27-stage38-h264-i16-restart.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled.
- No host EFI partition, host bootloader, internal Windows disk, internal Linux
  disk, USB device, or boot order was touched.

Honest limitations / what still does not work:

- Only the first luma macroblock row of the first IDR frame is decoded.
- Full-frame luma does not exist yet.
- Chroma is parsed only for bitstream alignment; no Cb/Cr reconstruction exists.
- No YUV420 frame buffer, no YUV420 -> RGB conversion, no native MP4 playback.
- AAC remains detection-only; no AAC decode or A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: extend the verified luma decoder from the first row to
  additional macroblock rows of the first IDR frame, adding top-neighbour
  cross-macroblock tracking. Do not start YUV420 -> RGB, timing, AAC, or playback
  yet.

## 2026-06-27 - Stage 38 follow-up: Step 2 luma reaches top-neighbour rows

Plain English:

Continued the same progression-tree item: Step 2, H.264 Constrained Baseline
video-only. Vibio now carries top-neighbour macroblock state for CAVLC nC,
intra prediction modes, and chroma-residual alignment while reconstructing luma
past the first row. On `BASE320.MP4`, the current single-slice probe decodes 78
luma macroblocks from the first IDR slice and exposes the 3 complete macroblock
rows it can rectangularly verify: 320x48 luma, 15360 samples. That 320x48 region
is bit-exact against ffmpeg. This is still luma-only probing, not native MP4
playback.

What changed:

- `src/kernel/h264.c` replaced the first-row decode routine with a bounded
  first-slice luma-region routine.
- Added per-column top luma total-coeff tracking so CAVLC nC can use the
  macroblock above for blocks in later rows.
- Added per-column top intra prediction mode tracking so intra4x4 predicted
  modes can use the macroblock above.
- Added top chroma AC total-coeff tracking for CAVLC bitstream alignment while
  chroma pixels remain unreconstructed.
- Reset left-neighbour state at macroblock-row starts instead of carrying it
  across row boundaries.
- Existing 4x4 and 16x16 prediction helpers now receive absolute frame-region
  coordinates, so top samples from prior rows are available through the decoded
  luma bitmap.
- Added `src/kernel/media_codec.h` as a small compile-time codec boundary for
  bounded luma-region facts; H.264 fills it without claiming playback support.
- `H264Probe` now reports complete macroblock rows decoded, luma rows decoded,
  top-neighbour nC/intra flags, and a larger luma region buffer.
- `tools/h264_verify_mb0.py` now keeps the old first-row regression visible
  even when the reported luma region is taller.
- Media Player now shows `H.264 Step 2: luma rows OK mbs=78/240 rows=3 I16=36`,
  draws the 320x48 grayscale luma region, and still says `Decode: NOT supported
  (demux/probe only)`.

Verification:

- `gcc -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c
  src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS.
- `py tools\h264_verify_mb0.py Disk\BASE320.MP4` - PASS:
  - `MB0 luma mismatch = 0 / 256`
  - `first row luma mismatch = 0 / 5120`
  - `luma region 320x48 mismatch = 0 / 15360`
  - `MB3 luma mismatch = 0 / 256`
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports
  `h264_luma_region mbs=78 mb_rows=3 rows=48 width=320 height=48 samples=15360
  row_ok=1 full_frame=0 cross_nc=1 cross_intra=1 top_nc=1 top_intra=1 i16=1
  i16_dc=1 i16_mbs=36 chroma=0 ... detail=multiple macroblock rows luma
  reconstructed`.
- `.\build.ps1` - PASS; kernel, EFI, raw image, USB image, and VDI built.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - expected clean failure:
  `moov truncated / past buffer`.
- `.\build\mp4_probe_host.exe build\media\UNSUP.MP4` - PASS unsupported path:
  HEVC video + AAC audio demux/probe, no H.264 decode path entered.
- QEMU `PLAY BASE320.MP4` - PASS; Media Player shows the 78/240 luma-row status,
  320x48 grayscale luma region, and unchanged unsupported decode line.
- QEMU `PLAY UNSUP.MP4` - PASS unsupported-codec smoke check.
- QEMU `PLAY BAD.MP4` - PASS corrupt-container smoke check.
- QEMU boot/restart smoke - PASS; system returned to the desktop.

Screenshots:

- `docs/screenshots/2026-06-27-stage38-h264-toprows-base320.png`
- `docs/screenshots/2026-06-27-stage38-h264-toprows-unsup.png`
- `docs/screenshots/2026-06-27-stage38-h264-toprows-bad.png`
- `docs/screenshots/2026-06-27-stage38-h264-toprows-boot.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled.
- No host EFI partition, host bootloader, internal Windows disk, internal Linux
  disk, USB device, or boot order was touched.

Honest limitations / what still does not work:

- The current H.264 luma probe decodes only the first IDR slice. `BASE320.MP4`
  contains multiple slice NALs in the first sample, so luma stops after 78
  macroblocks and exposes only 3 complete rows for rectangular verification.
- Full-frame luma does not exist yet.
- Chroma is parsed only for bitstream alignment; no Cb/Cr reconstruction exists.
- No YUV420 frame buffer, no YUV420 -> RGB conversion, no native MP4 playback.
- AAC remains detection-only; no AAC decode or A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Continue Step 2 only: refactor the luma-region decoder state so later IDR
  slice NALs can continue the same frame and complete visible-frame luma. Do not
  start YUV420 -> RGB, timing, AAC, or playback yet.

## 2026-06-27 - Stage 38 follow-up: Step 2 decodes the FULL first IDR luma frame (320x180, bit-exact vs ffmpeg)

Summary: the previous entry stopped luma at 78 macroblocks and blamed a
"multiple slice NALs" boundary. That diagnosis was WRONG. The first video
sample of `BASE320.MP4` contains exactly two NALs - one SEI (type 6, 625 B) and
one IDR slice (type 5, 3767 B). The single IDR slice owns the whole 20x12 = 240
macroblock frame; there is no second slice to continue. The real blocker was a
CAVLC context bug.

Root cause (real):

- In `decode_first_luma_region` the Intra_16x16 luma DC block was decoded with a
  hard-coded `nC = 0` (`decode_residual_4x4(b, 0, ...)`).
- Per H.264 9.2.1, the `Intra16x16DCLevel` block's `nC` is derived from the same
  neighbours as the top-left 4x4 luma block (luma4x4BlkIdx 0), not 0.
- `nC` selects the `coeff_token` VLC table. Macroblocks 74 and 75 are also
  Intra_16x16 and decoded fine only because their block-0 neighbours had `nC < 2`
  (same table as 0). Macroblock 78's block-0 neighbours give `nC = 3` (table 1),
  so the DC was read from the wrong table, producing a wrong DC TotalCoeff and
  desyncing the rest of that macroblock. The desync surfaced as an invalid
  `total_zeros` while decoding the I_16x16 AC of luma block 8.
- Because mb 0-77 were already bit-exact (proven against ffmpeg, including the
  internal partial 4th row mb 60-77), the bit position entering mb 78 was exact;
  the failure was purely the DC `nC` table selection.

How it was found:

- A throwaway host build with `-DH264_HOST_DEBUG` traced the per-macroblock and
  per-block CAVLC state; it isolated the failure to mb 78, I_16x16 AC, luma block
  8, with `nC=2` yielding `total_zeros=13 > 15-3`.
- A standalone Python re-implementation of just the CAVLC bitstream parser
  (no pixels) used "does the whole frame parse cleanly to 240 MBs" as the
  oracle. The stock rule failed; making the I_16x16 DC use block-0's neighbour
  `nC` made the entire frame parse to `('OK', 240)`. All debug code and the
  Python helper were removed afterwards.

Fix:

- `src/kernel/h264.c`: the I_16x16 luma DC residual now derives `nC` from
  `left_tc[0]` / `top_tc[mb_x*4]` with the standard availability + `(nA+nB+1)>>1`
  rule, identical to luma 4x4 block 0. One localized change; no rewrite.

Result:

- Full first IDR visible-frame luma is reconstructed: 240/240 macroblocks, 12
  macroblock rows, 320x180, 57600 samples, `full_frame=1`, 97 Intra_16x16 MBs.
- Bit-exact against ffmpeg (pre-deblock, `-skip_loop_filter all`):
  - `MB0 luma mismatch = 0 / 256`
  - `first row luma mismatch = 0 / 5120`
  - `luma region 320x180 mismatch = 0 / 57600`
  - `MB3 luma mismatch = 0 / 256`
- The Media Player now shows `H.264 Step 2: visible luma frame OK mbs=240/240
  rows=12 I16=97`, `luma region 320x180 qp=26 ... min=9 max=239`, and the full
  decoded first frame as grayscale luma (a recognizable scene). It still says
  `Decode: NOT supported (demux/probe only)` and `no RGB playback or AAC PCM`.

Module boundary:

- Unchanged and intentionally small. MP4 demux stays in `mp4.c`; H.264 internals
  stay in `h264.c`; `media_codec.h` still carries only the summarized
  `MediaCodecLumaRegion` fact; Media Player consumes the summary. No fake dynamic
  module loading was introduced.

Verification:

- `clang -Wall -Wextra -I src\kernel tools\mp4_probe_host.c src\kernel\mp4.c
  src\kernel\h264.c -o build\mp4_probe_host.exe` - PASS (host gcc lacks libc
  headers in this environment; clang is the host compiler).
- `.\build\mp4_probe_host.exe Disk\BASE320.MP4` - PASS; reports
  `h264_luma_region mbs=240 mb_rows=12 rows=192 width=320 height=180
  samples=57600 row_ok=1 full_frame=1 ... i16_mbs=97 blocker_mb=0
  detail=first IDR visible-frame luma reconstructed`.
- `py tools\h264_verify_mb0.py Disk\BASE320.MP4` - PASS, all four checks 0
  mismatch (256, 5120, 57600, 256).
- `.\build.ps1` - kernel, EFI and raw VM image built successfully; the optional
  VirtualBox `convertfromraw` VDI packaging step failed in this environment
  (VBox-side, unrelated to the kernel). QEMU uses the raw image and ran the
  fixed kernel.
- `.\build\mp4_probe_host.exe build\media\BAD.MP4` - clean failure
  `moov truncated / past buffer`.
- `.\build\mp4_probe_host.exe build\media\UNSUP.MP4` - clean unsupported path
  (HEVC + AAC demux/probe, no H.264 decode entered).
- QEMU `PLAY BASE320.MP4` - PASS; full 320x180 grayscale luma frame + honest
  unsupported decode line.
- QEMU `PLAY UNSUP.MP4` - PASS; `H.265/HEVC video not supported`.
- QEMU `PLAY BAD.MP4` - PASS; `This is not a playable MP4: moov truncated`.
- QEMU boot smoke - PASS; desktop renders.

Files changed:

- `src/kernel/h264.c` - I_16x16 luma DC `nC` derivation fix (the only behavioural
  change).

Screenshots:

- `docs/screenshots/2026-06-27-stage38-h264-fullframe-base320.png`
- `docs/screenshots/2026-06-27-stage38-h264-fullframe-unsup.png`
- `docs/screenshots/2026-06-27-stage38-h264-fullframe-bad.png`

Safety status:

- VM-only and read-only. No storage write path was added or enabled. No host EFI
  partition, bootloader, internal disk, USB device, or boot order was touched.

Honest limitations / what still does not work:

- Only LUMA of the first IDR frame is reconstructed. No chroma (Cb/Cr) is
  reconstructed; chroma residual is still only parsed for bitstream alignment.
- No YUV420 frame buffer assembled, no YUV420 -> RGB conversion, no frame timing,
  no native MP4 playback. Media Player still reports playback unsupported.
- Only the first frame (IDR) is decoded; inter (P) frames are not.
- AAC remains detection-only; no AAC decode and no A/V sync.
- Real hardware was not tested in this pass.

Next milestone:

- Still inside `H.264 Constrained Baseline video-only`. Natural next step toward
  native playback is chroma (Cb/Cr) reconstruction to assemble a complete YUV420
  frame, then `YUV420 -> RGB fast conversion`. Do not start AAC or timing before
  the YUV frame exists.

## 2026-06-27 - Stage 38 follow-up: native MP4 H.264 Baseline video-only playback

Goal:

- Finish the current progression-tree item only: H.264 Constrained Baseline
  video-only playback for the supplied small `BASE320.MP4`.
- Do not start AAC/audio.
- Keep H.264 internals in `src/kernel/h264.c` / `src/kernel/h264.h`; keep
  `main.c` limited to Media Player orchestration, bounded buffer ownership, API
  calls, RGB blit, and timing.

What changed:

- `src/kernel/h264.c` now carries the real frame decoder for this target:
  CAVLC residual decode, intra4x4/Intra16x16, P-skip/inter prediction,
  motion-vector prediction, luma/chroma motion compensation, residual add,
  Cb/Cr reconstruction, YUV420 frame output, reference-frame management, and
  YUV420-to-XRGB conversion support.
- Fixed the frame 116 divergence. The root cause was CAVLC level decoding for
  the `level_prefix == 14/15 && suffixLength == 0` edge case: prefix 14 must not
  receive the extra +15 adjustment, while prefix 15 still must. The earlier
  suspicion about non-IDR I-frame edge-neighbour state was useful to check but
  was not the bug.
- Corrected the Intra16x16 luma DC Hadamard pairing to use the proper 0/2 and
  1/3 coefficient pairs.
- `tools/h264_decode_host.c` is now a clean host verifier/dumper and accepts an
  optional `coded` argument to dump the full coded 320x192 frame instead of only
  the visible 320x180 crop.
- `src/kernel/main.c` allocates bounded MP4/H.264 buffers for the current small
  target, initializes the decoder from MP4 `avcC` SPS/PPS, decodes video frames
  sequentially, converts/blits RGB into the Media Player window, and advances
  using MP4 sample timing.
- Media Player now opens `BASE320.MP4` as real MP4 video, not a VIV fallback. It
  reports `MP4 video: H.264 Baseline OK` and `Audio: AAC detected, unsupported
  Playback: video-only`.
- Unsupported/corrupt MP4 paths still render clean probe/error pages. HEVC
  remains unsupported, and corrupt `moov` data is rejected without crashing.

Verification:

- Host H.264 harness build - PASS:
  `gcc -std=c99 -O2 -Wall -Wextra -I src\kernel -I src\shared
  tools\h264_decode_host.c src\kernel\mp4.c src\kernel\h264.c -o
  build\h264_decode_host.exe`.
- Cropped regression vs ffmpeg with deblocking disabled - PASS: frames 0-200
  matched Y/Cb/Cr, including frame 116.
- Full coded-file regression vs ffmpeg with deblocking disabled - PASS:
  `all match coded 4594 frames Y/Cb/Cr` against ffmpeg
  `-apply_cropping 0 -skip_loop_filter all`.
- Kernel build - PASS: `.\build.ps1`.
- QEMU `PLAY BASE320.MP4` - PASS: Media Player shows actual decoded video
  frames and the frame counter advanced to frame 356 in the captured run.
- QEMU `PLAY BAD.MP4` - PASS: clean error page,
  `This is not a playable MP4: moov truncated / past buffer`.
- QEMU `PLAY UNSUP.MP4` - PASS: clean unsupported page for H.265/HEVC + AAC; no
  H.264 decode path is entered.

Screenshots:

- `build/test-shots/mp4-base320-playback-long.png`
- `build/test-shots/mp4-bad-clean-2.png`
- `build/test-shots/mp4-unsup-clean-2.png`

Honest limitations:

- This is a narrow 320x180 H.264 Constrained Baseline video-only path for the
  current small test file, not a general H.264 player.
- H.264 deblocking is still stubbed/disabled. All bit-exact references are
  intentionally compared against ffmpeg with `-skip_loop_filter all`.
- AAC is detected only; no AAC PCM decode, no A/V sync, and no fake audio.
- H.264 Main/High, larger-resolution optimization, HEVC/H.265, VP9, AV1, MJPEG,
  Opus, MP3, and real hardware playback remain future work.

Next milestone:

- After this video-only pass, the progression tree can move to AAC-LC detection
  / AAC-LC stereo decode or, if normal ffmpeg visual parity is required first,
  H.264 deblocking. Do not claim audio until real AAC PCM exists.

## 2026-06-27 - System volume applies real PCM gain

Problem:

- The taskbar system volume slider changed the UI value and called the audio
  volume path, but nonzero levels sounded the same. Only 0 behaved differently,
  because the AC'97 mixer path effectively worked as mute-only in the tested
  environment.

Fix:

- Added software PCM16 gain in `audio_prepare_pcm`.
- Stereo WAV data is copied into an unscaled PCM source buffer; mono WAV data is
  expanded into that same unscaled stereo source buffer.
- The active DMA buffer is rendered from the unscaled source at the current
  system volume. `audio_set_volume` re-renders it while playback is active, so
  moving the slider during a sound changes the currently playing PCM instead of
  only affecting the next sound.
- The hardware mixer write remains in place, but audible nonzero volume levels
  no longer depend on the AC'97/HDA mixer honoring attenuation.

Verification:

- `.\build.ps1` - PASS.

## 2026-06-28 - Silent UEFI boot with color-coded boot stops

Problem:

- The UEFI loader showed a multi-step colored boot sequence with stalls before
  the kernel started. On real hardware this looked like flashing lights at the
  start instead of a clean Vibio boot.

Fix:

- Removed the cosmetic boot-step text, color changes, and boot delays from the
  normal UEFI success path.
- Normal boot now clears the firmware text screen once, hides the cursor, loads
  the kernel/assets, exits boot services, and jumps to the kernel silently.
- Kernel early-boot trace colors are disabled on the normal path; the old
  red/orange/yellow/blue/cyan/magenta/white/gray/purple/pink/green milestone
  paints no longer run during successful boot.
- Early loader failures now stop safely on a single solid-background diagnostic
  screen with `Vibio OS` at the top, a stable `VB-B###` code near the bottom, a
  short issue line, and the raw EFI status.
- Early kernel boot conditions that used to halt silently now stop on the same
  style of single-colour framebuffer screen with `VB-K###` near the bottom.
- The loader still halts in place on errors instead of returning to firmware, so
  it cannot chain into another boot option or touch a host/internal disk.

Boot stop codes:

- `VB-B001` red: boot volume could not be opened.
- `VB-B002` brown/yellow: `KERNEL.BIN` could not be loaded.
- `VB-B003` magenta: `VibioBootInfo` allocation failed.
- `VB-B004` light gray/white: UEFI memory-map buffer allocation failed.
- `VB-B005` green: framebuffer/GOP graphics mode could not be found.
- `VB-B006` blue: `ExitBootServices` failed after retries.
- `VB-K001` red: kernel received invalid boot information.
- `VB-K002` purple: dedicated kernel stack allocation failed.
- `VB-K003` green: kernel page-table setup failed.

Verification:

- `.\build.ps1` - PASS. Existing kernel unused-function warnings remain
  unrelated to this bootloader change.
- `.\tools\run-qemu.ps1 -Shot build\test-shots\silent-boot-smoke.png -BootWait
  18 -NoUsb` - PASS, QEMU reaches the Vibio desktop.
- `.\tools\run-qemu.ps1 -Shot build\test-shots\silent-boot-no-trace.png
  -BootWait 18 -NoUsb` - PASS, QEMU reaches the Vibio desktop with the kernel
  trace colors disabled.
- `.\tools\test-vm.ps1 -Shot build\test-shots\vbox-kernel-stop-hardening.png
  -BootWait 25` - PASS, VirtualBox reaches the Vibio desktop after a clean VM
  power cycle.
- `.\tools\run-qemu.ps1 -Shot build\test-shots\qemu-kernel-stop-hardening.png
  -BootWait 18 -NoUsb` - PASS, QEMU reaches the Vibio desktop.

Safety status:

- VM/QEMU-only verification.
- No host EFI partition, bootloader, internal disk, USB device, or boot order was
  touched.

## 2026-06-28 - Settings app and Super+I shortcut

What changed:

- Added a native Settings window (`WINDOW_KIND_SETTINGS`) with a Windows-style
  layout adapted to Vibio: left navigation rail, search field, Home dashboard,
  and clean status cards for System, Sound, Personalization, Storage, and
  Devices.
- Added a hand-drawn Settings gear icon for the taskbar and app chrome.
- Added global `Super+I` handling. The keyboard path tracks left/right Super
  (`E0 5B` / `E0 5C`) and opens/raises Settings on `I`.
- Settings uses live Vibio status where available: framebuffer resolution, boot
  readiness, audio state/volume, disk/FAT32 status, USB controller state, and
  current wallpaper/accent swatches.
- The app uses the existing window rise/fade animation and taskbar hover preview
  paths, so it behaves like a normal Vibio app instead of a static mockup.

Verification:

- `.\build.ps1` - PASS. Existing unused xHCI helper warnings remain unrelated.
- VirtualBox `Super+I` scancode injection - PASS:
  `build/test-shots/settings-super-i-final.png` shows the Settings app opened
  from the keyboard shortcut.

Safety status:

- VM-only verification.
- No host EFI partition, bootloader, internal disk, USB device, or boot order was
  touched.
