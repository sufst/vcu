# 0 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.S"
    .text 32
    .align 4
    .syntax unified
# 68 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_system_return.S"
    .thumb_func
    .global _tx_thread_system_return
_tx_thread_system_return:




    MOV r0, #0x10000000
    MOV r1, #0xE000E000
    STR r0, [r1, #0xD04]
    MRS r0, IPSR
    CMP r0, #0
    BNE _isr_context






    MRS r1, PRIMASK
    CPSIE i
    MSR PRIMASK, r1

_isr_context:
    BX lr
