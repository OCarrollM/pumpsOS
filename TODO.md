- [**DONE**] Install Tools: qemu, nasm, gcc (fuck anything else)
- [**DONE**] Get minimal kernal to print smth ("I love pumps")
- [**DONE**] Make a bootable ISO with GRUB
- [ ] MAKE interrups (chatGPT said to learn but I had Andrew Scott for this)
- [ ] Implement a timer and context switch

### From this point I discovered OSDevers forums and website and from here vow to only continue using this to develop my skills.

- [ ] Add support for newline in terminal driver
> I assume I need to do something similar to how strlen was implemented.

> Right now I cannt handle newlines and apparently VGA does not render newlines rather they are logical entities. The tutorial states that in `terminal_putchar` I should check if the character is equal to \n and icnrement `terminal_row` and `terminal_column` from that.
- [ ] Implement terminal scrolling
> Move all the rows up by 1 if the terminal fills up rather than it resetting itself.
- [ ] Rendering some colorful ASCII art
> I have 16 colours available. 8 for the background. But background can be fixed with a real VGA driver
- [ ] [Calling Global Constructors](https://wiki.osdev.org/Calling_Global_Constructors)
- [ ] [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton)
- [ ] [Going Further on x86](https://wiki.osdev.org/Going_Further_on_x86)
- [ ] Bare Bones II
