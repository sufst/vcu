# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.S"
    .text
    .align 4
    .syntax unified
# 68 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.S"
    .global _tx_thread_stack_build
    .thumb_func
_tx_thread_stack_build:
# 96 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_stack_build.S"
    LDR r2, [r0, #16]
    BIC r2, r2, #0x7
    SUB r2, r2, #68
    LDR r3, =0xFFFFFFFD
    STR r3, [r2, #0]



    MOV r3, #0
    STR r3, [r2, #4]
    STR r3, [r2, #8]
    STR r3, [r2, #12]
    STR r3, [r2, #16]
    STR r3, [r2, #20]
    STR r3, [r2, #24]
    STR r3, [r2, #28]
    STR r3, [r2, #32]



    STR r3, [r2, #36]
    STR r3, [r2, #40]
    STR r3, [r2, #44]
    STR r3, [r2, #48]
    STR r3, [r2, #52]
    MOV r3, #0xFFFFFFFF
    STR r3, [r2, #56]
    STR r1, [r2, #60]
    MOV r3, #0x01000000
    STR r3, [r2, #64]




    STR r2, [r0, #8]

    BX lr
