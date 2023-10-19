# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_restore.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_restore.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_restore.S"
    .text 32
    .align 4
    .syntax unified
# 66 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_interrupt_restore.S"
    .global _tx_thread_interrupt_restore
    .thumb_func
_tx_thread_interrupt_restore:




    MSR PRIMASK, r0

    BX lr
