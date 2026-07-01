# pumpsOS
I am attempting at creating my own OS. This came to me from a dream that my cat was a programmer and made her own OS. This is my pure attempt at recreating her work

## About 
This project is all about OS development and will be updated over a period of several months when time is on my side (Started Nov 2025).

I am following the [OSDev Wiki](https://wiki.osdev.org/) as a means of guidance and learning and as a challenge, restricting the use of AI to reinforce understanding and really put my programming knowledge to the test.

[June 2026] I have taken out a operating systems book from my Universities library to help learn some concepts 

## Current Status

- Phase 1 & 2 from the OSDev Wiki complete
- Terminal has its own shell `PumpsOS>`
- Fully ported self hosted OS, Binutils & GCC

## Up coming

- Phase 3
  - Proper file system
  - An actual user interface (excited) + graphics

## Building
As of commit `43dd352` (and presumebly later commits as I would rather commit working builds) it is possible to run this project yourself.

You **must** have a cross compiler, ideally i686-elf. See [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler) for assistance and instructions. 

Once done. You can run
```bash
./clean.sh          # Cleans build
./headers.sh        # Installs headers to sysroot
./iso.sh            # Builds an ISO image
./qemu.sh           # Builds and runs in QEMU
```

## Current Structure

```bash
pumpsOS/
├── kernel/           # Kernel source code
│   ├── arch/i386/    # Architecture-specific code (boot, tty, linker)
│   ├── include/      # Kernel headers
│   └── kernel/       # Core kernel code
├── libc/             # Minimal C library
│   ├── include/      # Standard headers (stdio, stdlib, string)
│   ├── stdio/        # printf, putchar, puts
│   ├── stdlib/       # abort
│   └── string/       # memcpy, memset, strlen, etc.
└── sysroot/          # Installed headers and libraries
```

## Roadmap

See [TODO.md](TODO.md) for current progress and future steps

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Bare Bones Tutorial](https://wiki.osdev.org/Bare_Bones)
- [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton)
- [Creating an Operating System](https://wiki.osdev.org/Creating_an_Operating_System)
