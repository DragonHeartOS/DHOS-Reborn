OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

BOOTSTRAP_IMAGE_BASE = 0x0000000000400000;

SECTIONS
{
	. = BOOTSTRAP_IMAGE_BASE;

	.text : AT(ADDR(.text) - BOOTSTRAP_IMAGE_BASE) {
		*(.text .text.*)
	}

	.rodata : AT(ADDR(.rodata) - BOOTSTRAP_IMAGE_BASE) {
		*(.rodata .rodata.*)
	}

	.data : AT(ADDR(.data) - BOOTSTRAP_IMAGE_BASE) {
		*(.data .data.*)
	}

	.bss : AT(ADDR(.bss) - BOOTSTRAP_IMAGE_BASE) {
		*(COMMON)
		*(.bss .bss.*)
	}
}
