# pumpsOS
I am attempting at creating my own OS. This came to me from a dream that my cat was a programmer and made her own OS. This is my pure attempt at recreating her work

## Goal 
I am learning OS fundamentals. PumpsOS was too mainstream to make in one day but rome wasn't built in one day

## Steps
~~1. Repo
2. Tooling
3. Minimal Kernal
4. Simple Boot
5. Interupts, timer, scheduler (Thank you Andrew Scott)
6. File system and shell (Getting there)~~

I am starting out by following and learning from OSDevers. The current source code follows the Barebones tutorial + my own research through exercises found at (https://wiki.osdev.org/Bare_Bones)

final - It was literally windows but pumps based and straight out of the 90s

Update Winter 2025 -
    Back at it

# Current Tree

```bash
.
├── LICENSE
├── Makefile
├── README.md
├── TODO.md
├── build/
│   ├── isodir/
│   │   └── boot/
│   │       ├── grub/
│   │       │   └── grub.cfg
│   │       └── myos.bin
│   └── myos.iso
├── docs/
├── grub.cfg
├── kernel/
│   ├── Makefile
│   ├── arch/
│   │   └── i386/
│   │       ├── boot.s
│   │       ├── crti.s
│   │       └── crtn.s
│   ├── include/
│   │   └── kernel/
│   │       └── tty.h
│   ├── kernel/
│   │   ├── kernel.c
│   │   └── tty.c
│   └── linker.ld
├── libc/
│   ├── Makefile
│   ├── include/
│   │   ├── stdio.h
│   │   └── string.h
│   ├── stdio/
│   │   └── putchar.c
│   └── string/
│       └── strlen.c
└── sysroot/
    └── usr/
        ├── include/
        └── lib/
```