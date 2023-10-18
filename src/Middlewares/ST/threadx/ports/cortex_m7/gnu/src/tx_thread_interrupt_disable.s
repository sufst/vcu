# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_disable.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_disable.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_disable.S"
    .text 32
    .align 4
    .syntax unified
# 66 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_disable.S"
    .global _tx_thread_interrupt_disable
    .thumb_func
_tx_thread_interrupt_disable:






    MRS r0, PRIMASK
    CPSID i

    BX lr
