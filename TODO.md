# pumpsOS Development

## Completed

- [x] Basic kernel with VGA text output
- [x] Newline support in terminal driver
- [x] Terminal scrolling
- [x] Colour support and ASCII art rendering
- [x] Global constructors
- [x] Meaty Skeleton project structure
- [x] Separated libc with printf, string functions
- [x] [Improving the Build System](https://wiki.osdev.org/Hard_Build_System)
- [x] [Stack Smash Protector](https://wiki.osdev.org/Stack_Smashing_Protector)

## In Progress

- [ ] [Going Further on x86](https://wiki.osdev.org/Going_Further_on_x86)
    - [x] Preperation
    - [x] Design Considerations
        - [x] Code Structure
            - Meaty Skeleton taught how to structure files properly, although this could change in the future, its current stage is a minimal example.
        - [x] Future Proofing
        - [x] Multithreading
        - [x] Hardware Abstraction
        - [x] Algorithms
    - [x] Paging
        - [x] Higher Half
        - [x] Permissions
            - Its basic for now, we have a write protection bit on paging but in the future for full vm, we need to set perms on every single section. 
    - [x] More x86
        - [x] Segments
            - Need to do some deeper reading on GDTs
        - [x] Interrupts
        - [x] Timers
        - [x] Keyboard input
    - [x] Memory Management
        - [x] Getting mem map
        - [x] Physical mem manager
        - [x] Virtual mem allocator
        - [x] Heap allocator
    - [x] Scheduler
        - [x] Multiprocessing
        - [x] Priority Levels
        - [x] Thread lists

- [ ] [Creating an operating system](https://wiki.osdev.org/Creating_an_Operating_System)
    - [x] Phase 0: Introduction
        - [x] Intro
        - [x] Building the latest GCC
    - [x] Phase 1: Beginning <- I am here
        - [x] [Setting up a Cross-Toolchain](https://wiki.osdev.org/GCC_Cross_Compiler)
        - [x] [Creating a Hello World Kernel](https://wiki.osdev.org/Bare_Bones)
        - [x] [Setting up a project](https://wiki.osdev.org/Meaty_Skeleton)
        - [x] [Calling Global Constructors](https://wiki.osdev.org/Calling_Global_Constructors)
        - [x] [Terminal Support](https://wiki.osdev.org/Formatted_Printing)
        - [x] [Stack Smash Protector](https://wiki.osdev.org/Stack_Smashing_Protector)
        - [x] [MultiBoot](https://wiki.osdev.org/Multiboot)
        - [x] [Global Descriptor Table](https://wiki.osdev.org/Global_Descriptor_Table)
        - [x] [Memory Management](https://wiki.osdev.org/Memory_Management)
        - [x] [Interrupts](https://wiki.osdev.org/Interrupts)
        - [x] [Multithreaded Kernel](https://wiki.osdev.org/Multithreaded_Kernel)
        - [x] [Keyboard](https://wiki.osdev.org/Keyboard)
        - [x] [Internal Kernel Debugger](https://wiki.osdev.org/Internal_Kernel_Debugger)
        - [x] [Filesystem support](https://wiki.osdev.org/Filesystem)
    - [ ] Phase 2: User-Space
        - [x] User-Space
        - [x] Program Loading
        - [x] System Calls
        - [x] OS Specific Toolchain
        - [x] Creating a C Library
        - [x] Fork and Execute
        - [x] Shell
    - [ ] Phase 3: Expanding
        - [x] Time
        - [x] Threads
        - [x] Thread Local Storage
        - [ ] Symmetric MultiProcessing
        - [x] Secondary Storage
        - [x] Real Filesystems
        - [ ] Graphics
        - [ ] User Interface
        - [ ] Networking
        - [ ] Sound
        - [ ] Universal Serial Bus
    - [ ] Phase 4: Bootstrapping
        - [ ] Porting Software
        - [ ] Porting GCC
        - [ ] Compiling your OS under your OS
        - [ ] Fully Self hosting
    - [ ] Phase 5: Complete

## The End Result

A windows/linux based OS 
