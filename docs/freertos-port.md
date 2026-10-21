# ThreadX -> FreeRTOS port

The VCU firmware has been moved from Azure RTOS (ThreadX + FileX + USBX) to FreeRTOS. All
changes are local and uncommitted (`git status` / `git diff` show everything). The firmware builds
(release and debug) with no new warnings, and the application has been exercised in a host
simulation. **It has not been run on the real VCU yet** — see the hardware test list at the end.

For how the RTOS is now organised (task table, priorities, rules), see
[code-overview.md](code-overview.md#rtos-freertos).

## What replaced what

| Before (Azure RTOS)       | After                                                              |
| ------------------------- | ------------------------------------------------------------------ |
| ThreadX 6.1.10            | FreeRTOS kernel V11.2.0 (ST `stm32_mw_freertos`, Cortex-M7 r0p1 port, `heap_4`) |
| —                         | CMSIS-RTOS2 wrapper (`cmsis_os2.c`), only because the RTCAN FreeRTOS OSAL uses it |
| FileX (SD logging)        | FatFs R0.16 (ST `stm32_mw_fatfs`) + `FATFS/Target/sd_diskio.c`     |
| USBX device, MSC class    | ST USB Device library, MSC class (`USB_DEVICE/`)                   |
| `fx_stm32_sd_driver_glue` | `SUFST/Src/Interfaces/sd_card.c` (shared by FatFs and USB MSC)     |
| `app_threadx.c`, `app_azure_rtos.c`, `tx_initialize_low_level.s` | `Core/Src/freertos.c`, `Core/Inc/FreeRTOSConfig.h` |
| RTCAN `rtcan_osal_threadx.c` | RTCAN `rtcan_osal_freertos.c` (submodule not modified)          |

### API mapping used in the services

| ThreadX                                    | FreeRTOS                                              |
| ------------------------------------------ | ----------------------------------------------------- |
| `tx_byte_allocate` + `tx_thread_create`    | `rtos_task_create()` (new helper, wraps `xTaskCreate`, stack from FreeRTOS heap) |
| `tx_thread_sleep(t)`                       | `vTaskDelay(t)`                                       |
| `tx_time_get()`                            | `xTaskGetTickCount()`                                 |
| `TX_TIMER_TICKS_PER_SECOND`                | `configTICK_RATE_HZ` (still 1000)                     |
| `tx_mutex_*` (`TX_INHERIT`)                | `xSemaphoreCreateMutexStatic` / `Take` / `Give` (priority inheritance) |
| `tx_queue_*`                               | `xQueueCreateStatic` / `xQueueSend` / `xQueueReceive` |
| `tx_timer_*`                               | `xTimerCreateStatic` / `xTimerChangePeriod`           |
| timer + semaphore to wake a periodic thread (wheelspeed) | `xTaskDelayUntil`                       |
| `tx_thread_terminate(self)`, returning from entry | `vTaskDelete(NULL)`                            |
| `tx_interrupt_control` around `vsnprintf`  | `taskENTER_CRITICAL` (`..._FROM_ISR` in interrupts)   |
| `TX_BYTE_POOL *stack_pool_ptr` parameters  | removed from every `*_init()`                         |

Priorities were converted keeping the same order and the same ties: FreeRTOS priority = 40 - ThreadX
priority (FreeRTOS: higher number = more urgent). Time slicing is disabled to match `TX_NO_TIME_SLICE`.

## Bugs found and fixed while porting

Some of these only became bugs because of FreeRTOS semantics; others were already there.

1. **PM100 mutex leak + recursive take (would have broken the car on FreeRTOS).**
   `pm100_request_torque()` kept the state mutex if it bailed out (e.g. inverter error latched),
   and it called `pm100_motor_speed()` while holding the same mutex. ThreadX mutexes are recursive
   and all callers are the control thread, so this was harmless before. With FreeRTOS mutexes, after
   an inverter fault every control tick blocked on mutex timeouts (~0.5 s per tick instead of 10 ms),
   and the fault could no longer be acknowledged with the TS button. Verified in the simulation:
   without the fix 2 checks fail, with it all pass.
2. **Tick thread gave back a mutex it might not hold** (FreeRTOS asserts on this). It now only
   unlocks if the lock succeeded.
3. **Tasks must not return.** The dash thread returned after the visual check, and pm100 / fans /
   remote_ctrl / SD threads terminated themselves; these now call `vTaskDelete(NULL)`.
4. **Log messages were truncated.** The ThreadX log queue used 64-byte messages but `log_msg_t`
   is 76 bytes, so the end of each message was dropped (lines could contain garbage). The queue now
   holds whole messages, and uses 10 KB instead of 39 KB.
5. **`log_printf()` is now interrupt-safe** (it is called from the CAN error callback).
6. **`fans_init()` with `enable = false`** left `config_ptr` NULL, and the control loop would then
   crash in `fans_update_thermal()`.
7. **TS-on LED blink.** In a fault, `dash_blink_ts_on_led()` is called every 10 ms and restarted the
   timer each time, so the LED never blinked during the fault (and then blinked forever after it was
   cleared). It now leaves a running timer alone, so the LED blinks during the fault. Note: nothing
   ever turns the blinking off again (same as before) — decide what the LED should do after a fault
   is acknowledged.
8. **SD card init failure no longer halts the VCU.** It used `MX_SDMMC1_SD_Init()`, which calls
   `Error_Handler()`; a bad SD card would stop the whole car. `sd_card_init()` now returns an error
   and logging is skipped.
9. `vcu_handle_can_rx_it()` / `vcu_handle_can_err()` read an uninitialised status.
10. Stack sizes: services 1024 -> 2048 bytes for headroom, RTCAN threads explicitly 1024 bytes
    (the CMSIS default would have been 512). FreeRTOS stack overflow checking is enabled
    (`Error_Handler()` on overflow).

### Found in the code review after the port (fixed)

11. **PM100 run faults in the low fault word were ignored.** The check tested `run_fault_hi` and
    `post_fault_lo` (typo) instead of `run_fault_lo` and `run_fault_hi`, so faults reported in the
    run fault low word never stopped the car. Now both run fault words and both POST fault words
    are checked. **Bench check needed**: make sure no fault bit is set during normal operation
    (see how-to-run.md, step 8.1).
12. **Inverter not disabled at standstill when the speed was slightly negative.** The signed motor
    speed was stored in a `uint16_t`, so -1..-9 rpm of jitter (the motor is wired backwards)
    looked like ~65,000 rpm. Now compared as signed (|speed| < 10).
13. **PM100 state written without the lock.** The PM100 task now holds the state mutex while it
    unpacks broadcasts, so the control task can't read half-updated values or lose a latched fault.
14. **A failed service start could hang the VCU.** If a service after the control task failed to
    start, the control task would use a mutex that was never created (FreeRTOS asserts and stops).
    Now the control task is suspended, the TS kept off, and the heartbeat blinks fast.
15. **`Error_Handler()` hung without blinking** (it used `HAL_Delay` with interrupts off), and left
    the outputs as they were. It now switches TS_ON and the RTDS off and blinks with a busy-wait.
16. **Log flooding**: "ADC/Torque" and "Sending torque request" were logged at 100 Hz each, more
    than the 115200-baud UARTs can carry, starving the heartbeat/SD tasks and dropping real
    warnings. They are now DEBUG level; the TS fault and failed-torque-request errors are logged
    once when they start instead of every 10 ms.
17. **CAN error interrupt logging** is rate-limited (once per second per bus, or when the error
    changes) instead of on every failed frame.
18. **SD log lines were dropped**: the SD task emptied its 64-line queue only every 10 s, so
    anything beyond 64 warnings per 10 s was lost (the test showed only 8 of 200 lines reaching the
    card). Lines are now written as they arrive and the file is synced every 10 s.
19. **Remote control leaked a CAN receive buffer** whenever its mutex timed out, eventually
    starving the whole CAN-S bus. The message is now always released.
20. **CAN broadcasts paused for 500 ms after one slow cycle.** The broadcast task now uses
    `xTaskDelayUntil` for a fixed 10 Hz rate.

### Found in the second review round (fixed)

21. **A FreeRTOS assert or a CPU fault left the AIRs closed.** `configASSERT` and the NMI /
    MemManage / BusFault / UsageFault handlers now go through `Error_Handler()` (TS off, RTDS off,
    LED blinking) like a HardFault already did.
22. **Boot hang with no indication.** FreeRTOS keeps interrupts masked from the first task creation
    until the scheduler starts, which freezes the HAL tick. A HAL timeout during initialisation
    (e.g. `HAL_CAN_Start()` with an unpowered CAN transceiver) then never expired. (ThreadX had the
    same problem: `tx_application_define` also runs with interrupts disabled.) `vcu_init()` now runs
    in a start-up task at the highest priority once the scheduler is running; the services it
    creates are all lower priority, so none runs before initialisation is complete.
23. **The control loop stopped for 2 s at R2D** (`rtds_activate()` slept inside the control task),
    so the shutdown circuit and everything else wasn't checked while the RTDS sounded. The RTDS is
    now timed by the state machine (`rtds_set()`), and the loop keeps running.
24. **Cooling switched off in a fault.** The TS fault handler forced the fan and pump off every
    tick, even after an over-temperature fault. They now follow the fans service's thermal
    hysteresis (on above 50 °C, off below 48 °C).
25. **Shutdown opening during the AIR wait or precharge** wasn't noticed until the 3 s precharge
    timeout; it now gives a TS fault straight away.
26. **More log flooding**: the PM100 disable command (every tick in a fault), the inverter lockout
    warning, CAN transmit failures in CANBC/wheelspeed, a persistent APPS discrepancy and the remote
    control timeout (an ERROR every second in every normal session) are now logged once or at most
    once per second.
27. The tick task no longer writes the shared sensor readings if it couldn't take their lock.
28. **Stack sizes checked** with a new worst-case stack analysis (`tests/tools/stack_analysis.py`):
    the heartbeat task (512 B -> 2 KB, it now also prints the RAM report) and the FreeRTOS idle task
    (512 B -> 1 KB) were too tight in the debug build; the main stack reservation was 1 KB while
    nested interrupts can need ~2.3 KB (now 4 KB). See how-to-run.md, step 1b.

### Found in the third review round (fixed)

29. **TS_ON (AIRs) and inverter power were switched ON at power-up.** `MX_GPIO_Init()` set both
    pins low, which on this board's inverted outputs means "on", until the control service switched
    them off - and in USB mass storage mode that never happened. Their initial level is now "off"
    (`VCU.ioc` PE13/PE15 `PinState=GPIO_PIN_SET`, and `gpio.c`), and the USB-mode path switches
    them off explicitly as well.
30. **Precharge could use a stale inverter state.** After TS was switched off and on again, the
    last state received before the inverter lost power (e.g. "ready") counted as "precharge
    complete" straight away. The cached state is now forgotten when a new precharge starts.
31. The shutdown checks during activation (and in R2D_WAIT) are debounced over 3 control ticks
    (30 ms), so a single glitch while the inverter powers up doesn't abort the activation.
32. The pump follows the cooling need (fan thermal state) while not driving, instead of keeping
    whatever value it had when a fault was acknowledged.
33. The APPS/BPS plausibility timer restarts on every entry to TS_ON (it could skip the 20 ms
    window after recovering from an APPS fault).
34. `pm100_is_precharged()` waits at most 100 ms for its lock (was 1 s, in the 100 Hz loop).
35. CAN transmit failure logs are rate-limited per bus, and the tick service's locking errors at
    most once per second.
36. The HardFault handler switches TS_ON and the RTDS off with its first instructions.

### Added afterwards

37. **Independent watchdog (IWDG)**: restarts the VCU if the control loop stops for more than
    ~0.5 s (0.34-0.94 s with the LSI tolerance). Started by the control task, kicked every cycle,
    not started on init failure or in USB mode, kept kicked in `Error_Handler()`, frozen while the
    debugger halts the CPU. A watchdog reset is logged on the next boot and shown as
    `CTRL_ERROR_WATCHDOG_RESET` (0x20) in the VCU error frame. `VCU.ioc` has IWDG enabled with
    "do not generate function call" (the control task starts it); the IWDG HAL driver v1.3.3 (the
    same HAL version as the rest of the project) was added.

Not changed: the RAM report pauses the scheduler for roughly 0.4 ms once every 10 minutes while
FreeRTOS measures the stacks, which is negligible for the 10 ms control loop.

## Cleanup (unused code removed)

- Functions: `trc_wait_for_ready`, `dash_wait_for_ts_on`, `dash_wait_for_r2d`, `input_wait`,
  `dash_set_ts_on_led_state`, `apps_check_plausibility`, `bps_fully_pressed`, `bps_valid`, `get_adc`,
  `rs232_disable`, `rs232_is_valid`, `pm100_lvs_on`.
- The whole `Test/` testbench code (only compiled under macros that were never defined) and
  `config.testbenches`.
- Config fields nothing read: `ts_ready_timeout_ticks`, `ts_ready_poll_ticks`,
  `ready_wait_led_toggle_ticks`, `vc_all_leds_on`, `vc_stagger_ticks`, `btn_sample_ticks`,
  `fully_pressed_fraction`, `pm100.speed_mode`, `remote_ctrl.power_limit` (note: the remote control
  power limit was never implemented), `rtos.ts_ctrl_thread_priority`, `rtos.tracex_enable`,
  `rtos.driver_ctrl_tick_rate`, and the commented-out legacy `#define`s at the end of `config.h`.
- Unused struct members (`tick.pump_pwr/fan_pwr`, `canbc.rolling_counter`, `vcu.err`, the unused
  semaphore/timer in `fans_context_t`), the UART mutex in the log service (only the log thread used
  it), and internal functions made `static`.
- Deleted: `src/Middlewares/ST/{threadx,filex,usbx}`, `src/AZURE_RTOS`, `src/FileX`, `src/USBX`,
  `Core/Inc/RTE_Components.h`, `tx_user.h` (both copies).

## Verification done

- `make` and `make DEBUG=1` build cleanly. The only warning in SUFST code (`log.c` format
  truncation) was already there in the ThreadX build.

  |                 | ThreadX (before) | FreeRTOS (after) |
  | --------------- | ---------------- | ---------------- |
  | Flash (text)    | 116,588 B        | 100,732 B        |
  | RAM (bss)       | 274,328 B        | 174,240 B        |

- Checked: SVC/PendSV/SysTick handlers come from FreeRTOS, the HAL timebase stays on TIM6, every
  interrupt that calls the RTOS (CAN 5, EXTI 6, SDMMC/DMA 7, USB 15) is at or below
  `configMAX_SYSCALL_INTERRUPT_PRIORITY` (5), and RTCAN only uses non-blocking calls from interrupts.
- **Test suite** in `tests/` (run `tests/run_tests.sh`, see [how-to-run.md](how-to-run.md#step-1-run-the-pc-test-suite)):
  92 unit checks on the control logic with the real calibration, and 40 scenarios that run the real
  SUFST code, RTCAN, FatFs and FreeRTOS kernel on the FreeRTOS POSIX port with a simulated car
  (driver, dash, shutdown circuit, PM100, remote control, CAN buses, SD card image). All pass.
  Each fix below was checked by putting the old code back and confirming a test fails.

## Test on the car / bench before trusting it

Nothing here has run on the real hardware. Suggested order:

1. Boot: heartbeat LED blinks at 2 Hz, visual check LEDs for 2 s, log lines on RS232/debug UART.
2. CAN: VCU state/sensors/errors/PDM at 10 Hz, wheel speeds at 50 Hz, no RTCAN errors.
3. Inputs: APPS/BPS/steering readings in the sensors frame, brake light.
4. TS activation with the inverter: TS button -> AIRs -> precharge complete -> R2D wait.
5. R2D (brake + button, RTDS 2 s), torque requests, R2D off, each mode on the mode switch.
6. Faults: APPS/BPS, APPS unplugged, shutdown opened, inverter fault + TS acknowledge.
7. SD logging: a new `log_N.txt` is created and WARN/ERROR lines appear in it.
8. USB mass storage: hold USER at power-on, card shows up on a PC, files read/write correctly.
9. Watch for `Error_Handler()` (heartbeat LED stops): it now also catches stack overflow and heap
   exhaustion.

## Still to do

- **`VCU.ioc`** has been edited by hand (CubeMX is not installed on the machine used for the port),
  so open it once in CubeMX and check it loads without complaints. Changes made:
  - Azure RTOS (X-CUBE-AZRTOS-F7: ThreadX, FileX, USBX) removed.
  - FREERTOS added with the CMSIS_V2 interface, no tasks/queues/mutexes defined in CubeMX
    (if CubeMX adds a `defaultTask`, delete it), heap 64 KB, stack overflow check 2, malloc
    failed hook, timer task priority 55.
  - PendSV and SysTick at priority 15, marked as used by FreeRTOS. HAL timebase stays TIM6.
  - All physical pins, peripherals, clocks and other NVIC settings unchanged.
  - FatFs and USB_DEVICE are deliberately **not** enabled as CubeMX middleware: the generator
    would overwrite FatFs R0.16 and our `sd_diskio.c` / `usbd_conf.c` / `usbd_storage_if.c`.
    SDMMC1 and USB_OTG_FS stay plain peripherals with "do not generate function call", as before.
  - To match CubeMX's F7 file layout, `app_freertos.c/.h` became `Core/Src/freertos.c` (with all
    VCU code in USER CODE sections), `main.c` includes `cmsis_os.h`, and `FreeRTOSConfig.h` was
    rearranged into the generated layout; settings CubeMX does not manage (time slicing off,
    32-bit ticks, timer task priority, extra `INCLUDE_`s, `configASSERT`) are in its USER CODE
    sections. The built image is identical in size to before.
  - After regenerating, check `git diff`: CubeMX may replace `src/Middlewares/Third_Party/FreeRTOS`
    with the kernel from the FW_F7 package. The code needs FreeRTOS V11 (e.g. `xTaskDelayUntil`,
    `configKERNEL_PROVIDED_STATIC_MEMORY`), so revert that folder if it is downgraded.
- `VCU_NUCLEO.ioc` (Nucleo dev board) and `VCU_Old.ioc` were converted the same way (Azure RTOS
  removed, FREERTOS CMSIS_V2 added). In those two, the CAN and EXTI15_10 interrupts were at
  priority 0, which may not call FreeRTOS, so they were moved to 5 (CAN) and 6 (EXTI) like in
  `VCU.ioc`. Their HAL timebase stays TIM3. No code is generated from them by the build.
- When committing, include the new untracked directories: `src/Middlewares/Third_Party/`,
  `src/Middlewares/ST/STM32_USB_Device_Library/`, `src/Drivers/CMSIS/RTOS2/` (`.gitignore` was
  updated so it is no longer ignored), `src/FATFS/`, `src/USB_DEVICE/`, and the new SUFST files.
- USB: VID/PID, strings and SCSI inquiry ("SUFST" / "VCU SD Card") are unchanged. The serial number
  is now derived from the MCU unique ID instead of the fixed `000000000001`.

### Existing behaviour noticed but deliberately not changed

- `TS_ACTIVATION_FAILURE` always moves straight to `TS_RUN_FAULT` on the next tick.
