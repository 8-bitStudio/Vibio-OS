ENTRY(kernel_main)

SECTIONS
{
    . = 0x100000;

    .text : ALIGN(16)
    {
        KEEP(*(.text.kernel_entry))
        *(.text*)
    }

    .rodata : ALIGN(16)
    {
        *(.rodata*)
    }

    .data : ALIGN(16)
    {
        *(.data*)
    }

    .bss : ALIGN(16)
    {
        *(COMMON)
        *(.bss*)
    }

    /* Force .bss into the flat binary.
     *
     * KERNEL.BIN is produced with `llvm-objcopy -O binary`, which DROPS a trailing
     * NOBITS (.bss) section. The loader then reserves only filesize + a 16 KiB
     * guard and never zeroes past the file, so ~190 KiB of .bss lived in memory
     * that was neither reserved nor zero-initialized. VirtualBox/OVMF happen to
     * hand back free, zeroed RAM there, but on real hardware that region holds
     * firmware/ACPI data (or garbage), corrupting zero-initialized globals and
     * hanging the kernel right after "Starting Vibio kernel".
     *
     * This one-byte PROGBITS marker sits AFTER .bss, so .bss is no longer trailing
     * and objcopy emits it as zeros in the binary. KERNEL.BIN then carries (and the
     * loader reserves + reads, hence zero-initializes) the full .bss. */
    .kimage_tail : ALIGN(16)
    {
        BYTE(0)
    }
}
