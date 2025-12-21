- [X] Add support for newline in terminal driver
> I assume I need to do something similar to how strlen was implemented.

> Right now I cannt handle newlines and apparently VGA does not render newlines rather they are logical entities. The tutorial states that in `terminal_putchar` I should check if the character is equal to \n and icnrement `terminal_row` and `terminal_column` from that.
- [X] Implement terminal scrolling
> Move all the rows up by 1 if the terminal fills up rather than it resetting itself.
- [X] Rendering some colorful ASCII art
> I have 16 colours available. 8 for the background. But background can be fixed with a real VGA driver
- [X] [Calling Global Constructors](https://wiki.osdev.org/Calling_Global_Constructors)
- [ ] [Meaty Skeleton](https://wiki.osdev.org/Meaty_Skeleton)
- [ ] [Going Further on x86](https://wiki.osdev.org/Going_Further_on_x86)
- [ ] Bare Bones II
