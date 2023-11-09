# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.S"
    .text 32
    .align 4
    .syntax unified
# 66 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_control.S"
    .global _tx_thread_interrupt_control
    .thumb_func
_tx_thread_interrupt_control:





    MRS r1, PRIMASK
    MSR PRIMASK, r0
    MOV r0, r1

    BX lr
