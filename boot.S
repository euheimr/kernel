// ARM Programmer's Guide: https://developer.arm.com/documentation/den0024/a/

.section ".text.boot" // make sure the linker puts this at the start of the kernel

.global _start 

// execution starts here
_start:
    // Check that we are executing on the main core 0, else hang
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

// not on main core ... hang on infinite wait loop...
1:  wfe
    b       1b

// on main core!
2:  // set stack to start below our code
    ldr     x1, =_start
    mov     sp, x1

    // clean BSS section
    ldr     x1, =__bss_start    // start address
    ldr     w2,=__bss_size      // size of section
3:  cbz     w2, 4f              // quit loop if zero
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b              // loop if non-zero

// Jump to our main() routine in C (make sure it doesn't return)
4:  bl      main
    // if it does return, halt main core 0
    b       1b