# 0 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.S"
    .global _tx_timer_time_slice
    .global _tx_timer_system_clock
    .global _tx_timer_current_ptr
    .global _tx_timer_list_start
    .global _tx_timer_list_end
    .global _tx_timer_expired_time_slice
    .global _tx_timer_expired
    .global _tx_thread_time_slice
    .global _tx_timer_expiration_process

    .text
    .align 4
    .syntax unified
# 83 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_timer_interrupt.S"
    .global _tx_timer_interrupt
    .thumb_func
_tx_timer_interrupt:







    LDR r1, =_tx_timer_system_clock
    LDR r0, [r1, #0]
    ADD r0, r0, #1
    STR r0, [r1, #0]





    LDR r3, =_tx_timer_time_slice
    LDR r2, [r3, #0]
    CBZ r2, __tx_timer_no_time_slice





    SUB r2, r2, #1
    STR r2, [r3, #0]




    CBNZ r2, __tx_timer_no_time_slice





    LDR r3, =_tx_timer_expired_time_slice
    MOV r0, #1
    STR r0, [r3, #0]



__tx_timer_no_time_slice:





    LDR r1, =_tx_timer_current_ptr
    LDR r0, [r1, #0]
    LDR r2, [r0, #0]
    CBZ r2, __tx_timer_no_timer





    LDR r3, =_tx_timer_expired
    MOV r2, #1
    STR r2, [r3, #0]
    B __tx_timer_done




__tx_timer_no_timer:




    ADD r0, r0, #4




    LDR r3, =_tx_timer_list_end
    LDR r2, [r3, #0]
    CMP r0, r2
    BNE __tx_timer_skip_wrap




    LDR r3, =_tx_timer_list_start
    LDR r0, [r3, #0]

__tx_timer_skip_wrap:

    STR r0, [r1, #0]


__tx_timer_done:





    LDR r3, =_tx_timer_expired_time_slice
    LDR r2, [r3, #0]
    CBNZ r2, __tx_something_expired

    LDR r1, =_tx_timer_expired
    LDR r0, [r1, #0]
    CBZ r0, __tx_timer_nothing_expired


__tx_something_expired:

    STMDB sp!, {r0, lr}






    LDR r1, =_tx_timer_expired
    LDR r0, [r1, #0]
    CBZ r0, __tx_timer_dont_activate





    BL _tx_timer_expiration_process


__tx_timer_dont_activate:





    LDR r3, =_tx_timer_expired_time_slice
    LDR r2, [r3, #0]
    CBZ r2, __tx_timer_not_ts_expiration





    BL _tx_thread_time_slice
    LDR r0, =_tx_thread_preempt_disable
    LDR r1, [r0]
    CBNZ r1, __tx_timer_skip_time_slice
    LDR r0, =_tx_thread_current_ptr
    LDR r1, [r0]
    LDR r2, =_tx_thread_execute_ptr
    LDR r3, [r2]
    LDR r0, =0xE000ED04
    LDR r2, =0x10000000
    CMP r1, r3
    BEQ __tx_timer_skip_time_slice
    STR r2, [r0]
__tx_timer_skip_time_slice:



__tx_timer_not_ts_expiration:

    LDMIA sp!, {r0, lr}




__tx_timer_nothing_expired:

    DSB
    BX lr
