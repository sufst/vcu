# 0 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
# 23 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    .global _tx_thread_current_ptr
    .global _tx_thread_execute_ptr
    .global _tx_timer_time_slice
    .global _tx_execution_thread_enter
    .global _tx_execution_thread_exit




    .text
    .align 4
    .syntax unified
# 79 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    .global _tx_thread_schedule
    .thumb_func
_tx_thread_schedule:







    MOV r0, #0
    LDR r2, =_tx_thread_preempt_disable
    STR r0, [r2, #0]
# 102 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    CPSIE i



    MOV r0, #0x10000000
    MOV r1, #0xE000E000
    STR r0, [r1, #0xD04]
    DSB
    ISB



__tx_wait_here:
    B __tx_wait_here




    .global PendSV_Handler
    .global __tx_PendSVHandler
    .syntax unified
    .thumb_func
PendSV_Handler:
    .thumb_func
__tx_PendSVHandler:



__tx_ts_handler:
# 141 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    LDR r0, =_tx_thread_current_ptr
    LDR r2, =_tx_thread_execute_ptr
    MOV r3, #0
    LDR r1, [r0]



    CBZ r1, __tx_ts_new



    STR r3, [r0]
    MRS r12, PSP
    STMDB r12!, {r4-r11}






    LDR r4, =_tx_timer_time_slice
    STMDB r12!, {LR}



    LDR r5, [r4]
    STR r12, [r1, #8]
    CBZ r5, __tx_ts_new



    STR r5, [r1, #24]



    STR r3, [r4]



__tx_ts_new:



    CPSID i
    LDR r1, [r2]
    CBZ r1, __tx_ts_wait



    STR r1, [r0]
    CPSIE i



__tx_ts_restore:
    LDR r7, [r1, #4]
    LDR r4, =_tx_timer_time_slice
    LDR r5, [r1, #24]
    ADD r7, r7, #1
    STR r7, [r1, #4]



    STR r5, [r4]
# 215 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    LDR r12, [r1, #8]
    LDMIA r12!, {LR}






    LDMIA r12!, {r4-r11}
    MSR PSP, r12



    BX lr





__tx_ts_wait:
    CPSID i
    LDR r1, [r2]
    STR r1, [r0]
    CBNZ r1, __tx_ts_ready
# 258 "src/Middlewares/ST/threadx/ports/cortex_m7/gnu/src/tx_thread_schedule.S"
    CPSIE i
    B __tx_ts_wait




__tx_ts_ready:
    MOV r7, #0x08000000
    MOV r8, #0xE000E000
    STR r7, [r8, #0xD04]



    CPSIE i
    B __tx_ts_restore
