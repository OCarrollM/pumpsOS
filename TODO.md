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
    - [ ] Paging
        - [x] Higher Half
        - [ ] Permissions
            - Its basic for now, we have a write protection bit on paging but in the future for full vm, we need to set perms on every single section. 
    - [ ] More x86
        - [x] Segments
            - Need to do some deeper reading on GDTs
        - [x] Interrupts
        - [ ] Timers
        - [ ] Keyboard input
    - [ ] Memory Management
        - [ ] Getting mem map
        - [ ] Physical mem manager
        - [ ] Virtual mem allocator
        - [ ] Heap allocator
    - [ ] Scheduler
        - [ ] Multiprocessing
        - [ ] Priority Levels
        - [ ] Thread lists

## Next Steps

- [ ] Basic shell
- [ ] Bare Bones II
    - There is no page for this, more of a "You are now free to experiment".
    This is where future ideas comes in and the real project will take shape. Hopefully this will be begun in 2026!

## Future Ideas

- [ ] File system (FAT12/16?)
- [ ] User mode
- [ ] System calls
- [ ] Multitasking / scheduler

## The End Result

A windows/linux based OS 