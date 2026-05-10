.section .text

/*
 * void task_switch(uint32_t* old_esp, uint32_t new_esp)
 *
 * Saves the current task's callee-saved registers onto its stack,
 * stores the resulting ESP into *old_esp, then loads new_esp and
 * pops the saved registers from the new task's stack.
 *
 * Arguments (cdecl, on the stack):
 *   4(%esp) = old_esp  (pointer to where we should save current ESP)
 *   8(%esp) = new_esp  (the ESP value to load for the next task)
 *
 * The C ABI says EAX, ECX, EDX are caller-saved. the compiler
 * already preserved any it cared about before calling us. We only
 * need to save EBX, ESI, EDI, EBP (callee-saved).
 */
.global task_switch
.type task_switch, @function
task_switch:
    /* Save registers onto curr tasks stack */
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp

    /* Save current esp into old esp */
    movl 20(%esp), %eax
    movl %esp, (%eax)

    /* Load new esp */
    movl 24(%esp), %esp

    /* Restore registers */
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx

    ret
