# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.S"
# 27 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.S"
    .text
    .align 4
    .syntax unified
# 70 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.S"
    .global _tx_thread_context_restore
    .thumb_func
_tx_thread_context_restore:
# 81 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_context_restore.S"
    BX lr
