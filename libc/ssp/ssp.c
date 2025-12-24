// The concept behind this is to stop buffer overflows.
// These overflows usually target the stack and are intended to
// help run malicous code. So to combat this we will be using
// what is known as a canary value which gets checked before
// function returns, if someone foes perform a overflow, our
// entire OS will die.

#include <stdint.h>
#include <stdlib.h>

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396 // 32-bit canary
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766 // 64-bit canary
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void) {
#if __STDC_HOSTED__
    abort();
#else // __is_pumpsos_kernel
    // TODO: Replace with a real panic()
    // panic("Stack Smashing Detected")
    asm volatile("cli");
    while(1) {
        asm volatile("hlt");
    }
#endif
}