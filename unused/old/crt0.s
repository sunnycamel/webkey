                .text
                .global _start
        _start:
                mov     r0, sp
                mov     r1, #0
                add     r2, pc, #4
                add     r3, pc, #4
                b       __libc_init
                b       main
                .word   __preinit_array_start
                .word   __init_array_start
                .word   __fini_array_start
                .word   __ctors_start
                .word   0
                .word   0

                .section .preinit_array
        __preinit_array_start:
                .word   0xffffffff
                .word   0x00000000

                .section .init_array
        __init_array_start:
                .word   0xffffffff
                .word   0x00000000

                .section .fini_array
        __fini_array_start:
                .word   0xffffffff
                .word   0x00000000

                .section .ctors
        __ctors_start:
                .word   0xffffffff
                .word   0x00000000
