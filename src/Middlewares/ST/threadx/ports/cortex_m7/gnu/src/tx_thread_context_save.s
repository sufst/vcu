# 0 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.S"
    .text
    .align 4
    .syntax unified
# 66 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.S"
    .global _tx_thread_context_save
    .thumb_func
_tx_thread_context_save:
# 79 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_save.S"
    BX lr
