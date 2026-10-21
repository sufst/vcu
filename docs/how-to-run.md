# How to test and run the FreeRTOS VCU firmware

This is the step-by-step guide for taking the FreeRTOS version of the VCU firmware from this
repository onto a real VCU. Work through it in order: each step builds on the previous one, and
the early steps catch problems before any hardware (or high voltage) is involved.

| Step | What | Needs |
| ---- | ---- | ----- |
| 1 | [Run the PC test suite](#step-1-run-the-pc-test-suite) | a Mac or Linux PC |
| 1b | [Check the RAM budget](#step-1b-check-the-ram-budget) | (part of step 1) |
| 2 | [Build the firmware](#step-2-build-the-firmware) | ARM toolchain |
| 3 | [Prepare the bench](#step-3-prepare-the-bench) | VCU on the bench, LV only |
| 4 | [Flash the VCU](#step-4-flash-the-vcu) | ST-Link |
| 5 | [Watch the log output](#step-5-watch-the-log-output) | RS232 or USB-serial adapter |
| 6 | [Low-voltage bench tests](#step-6-low-voltage-bench-tests) | CAN interface (e.g. CANoe / PCAN) |
| 7 | [SD card and USB tests](#step-7-sd-card-and-usb-mass-storage-tests) | micro SD card, USB cable |
| 8 | [Tests with the inverter and HV](#step-8-tests-with-the-inverter-and-high-voltage) | the car, HV-qualified people |
| 9 | [Debugging](#step-9-debugging-with-freertos-awareness) | ST-Link, VS Code |

If anything doesn't behave as described, see [Troubleshooting](#troubleshooting).

---

## Step 1: Run the PC test suite

The test suite runs the **real VCU code** (control state machine, PM100 handling, APPS/BPS,
torque map, logging, SD logging, RTCAN, FreeRTOS kernel) on your PC, with simulated hardware:
a simulated driver, dash, shutdown circuit, PM100 inverter, remote control and CAN buses.

```sh
cd vcu
tests/run_tests.sh
```

It takes about 2 minutes and prints one line per test. A good run ends with:

```
scenarios: 48 passed, 0 failed (of 48)
ALL TESTS PASSED
```

What it runs:

1. **Firmware build check** - builds the release and debug firmware (skipped if the ARM toolchain
   isn't installed), fails on any new compiler warning, and runs the stack/RAM analysis
   ([step 1b](#step-1b-check-the-ram-budget)).
2. **92 unit checks** on the pure logic, using the real calibration from `config.c`: torque map,
   torque limiters (slip + 75 kW power limit), APPS/BPS scaling and plausibility, mode switch.
3. **48 simulation scenarios**, each in a fresh process, in real time. Run
   `tests/build/vcu_sim --list` to see them all. They cover:
   - power-up, heartbeat, CAN broadcast rates, wheel speeds
   - TS activation, precharge (and its timeout), R2D (needs brake + long press), RTDS timing
   - every drive mode (endurance, max, torque control, crawl, reverse, remote, inverter
     programming, undefined) and changing mode while driving
   - APPS/BPS plausibility, APPS sensor disagreement/unplugged, brake sensor failure
   - shutdown circuit opening before and while driving
   - PM100 run faults (both fault words), POST fault, enable lockout, inverter going silent
   - power limiter, slip control, fans and their hysteresis, dash fan switch, brake light
   - 3000 random CAN frames while driving, a storm of CAN error interrupts
   - a 30 s random-driving soak test (no control loop stalls, no memory leaks)
   - the shutdown circuit opening during the AIR wait, precharge and the RTDS (and short glitches
     being ignored), precharge after an inverter power cycle
   - cooling kept on in a fault while hot, no log flooding in any state
   - a service failing to start, USB mass storage mode
   - the watchdog: kicked every cycle in every state, resets the VCU if the control loop hangs,
     and the reset is reported on the next boot
   - SD card logging (file numbering, nothing dropped, read back from a FAT image), SD write
     errors, no SD card

Useful options:

```sh
tests/run_tests.sh -k mode          # only scenarios with "mode" in the name
tests/run_tests.sh -j 8             # run 8 scenarios in parallel (default 4)
tests/run_tests.sh --no-firmware    # skip the ARM build
tests/build/vcu_sim driving_torque  # run one scenario directly, with full output
```

Every scenario's full output (state changes, all log lines, PASS/FAIL per check) is kept in
`tests/build/logs/<scenario>.log`.

> The simulator uses FreeRTOS's POSIX port, which very occasionally deadlocks on macOS. That is a
> simulator issue, not a VCU one; the runner detects it, retries the scenario once and prints
> "(simulator hung, retried)".

**Run this after every change you make to the VCU code**, before flashing. If you change
behaviour on purpose (e.g. a new threshold in `config.c`), update the matching test in
`tests/sim/src/sim_main.c` or `tests/unit/test_units.c`.

What the PC tests can't check: real timing on the STM32, the ADC and DMA, the CAN controllers and
transceivers, the SD card hardware and the USB stack. That's what steps 3-8 are for.

---

## Step 1b: Check the RAM budget

The STM32F746 has **320 KB of RAM**. Two different things have to fit:

1. **Global variables and buffers** (including the 64 KB FreeRTOS heap). The linker checks this:
   if they don't fit, the build fails with `region RAM overflowed`. They use about **171 KB (53 %)**.
2. **Each task's stack** must be big enough for its deepest chain of function calls. Nothing checks
   this at build time, and a stack overflow is a crash. Task stacks are taken from the FreeRTOS heap
   when the tasks are created.

`tests/tools/stack_analysis.py` works out (2) from the built firmware: it disassembles the ELF,
reads every function's stack frame (including the C library's `printf`), follows the call graph and
computes the worst case for every task and for the interrupts. `tests/run_tests.sh` runs it on both
builds and fails if any task has less than 25 % spare stack. To see the full report:

```sh
python3 tests/tools/stack_analysis.py build/release/VCU.elf
python3 tests/tools/stack_analysis.py build/debug/VCU.elf --verbose   # also prints the deepest call path
```

Current numbers (debug build, the worst of the two):

| | Used (worst case) | Available | Margin |
| --- | --- | --- | --- |
| Biggest service task stacks (Control, CANBC, LOG) | ~1.1 KB | 2 KB each | ~0.9 KB each |
| SD task | ~1 KB | 8 KB | 7 KB |
| RTCAN tasks | ~0.55 KB | 1 KB each | ~0.45 KB each |
| Interrupts (main stack, all priority levels nested) | ~2.3 KB | 4 KB reserved | ~1.7 KB |
| FreeRTOS heap (all task stacks + control blocks) | ~36 KB | 64 KB | ~28 KB |
| Whole RAM | ~175 KB (incl. heap + main stack) | 320 KB | ~145 KB never used |

So the RAM is far from full. On the real VCU, two runtime checks back this up:

- **FreeRTOS stack overflow checking is on**: if a task ever overflows its stack, the VCU goes to
  `Error_Handler()` (TS off, LED blinking, CAN stops) instead of corrupting memory.
- **A RAM report is logged** 10 s and 60 s after power-up and then every 10 minutes:

  ```
  10001 [INFO]: RAM: heap free 28xxx B (lowest 26xxx B), 18 tasks
  10001 [INFO]: RAM: Control stack unused 1xxx B
  ...
  ```

  "stack unused" is the least free stack the task has ever had since power-up (FreeRTOS fills
  stacks with a pattern and checks how much was never touched). After a bench session that went
  through all the states (step 6) and some driving (step 8), every task should still show a few
  hundred bytes unused. If one is close to 0, increase its `stack_size` in `config.c` (and, if the
  heap's "lowest" value gets small, `configTOTAL_HEAP_SIZE` in `FreeRTOSConfig.h` - there's plenty
  of RAM for both).

---

## Step 2: Build the firmware

If you haven't set up the toolchain yet, follow [dev-env-setup.md](dev-env-setup.md)
(ARM GNU toolchain, `make`, STLink tools). Make sure the submodules are checked out:

```sh
git submodule update --init --recursive
make            # release build -> build/release/VCU.elf / VCU.bin
make DEBUG=1    # debug build   -> build/debug/VCU.elf   (use this for debugging)
```

A good build prints the memory usage at the end (about 100 KB flash, 174 KB RAM for release).
The only expected warnings are an old `log.c` format-truncation warning and a linker note about
"RWX permissions"; anything else is new.

---

## Step 3: Prepare the bench

**For the first flash, use LV only.** Tractive system / HV disconnected, AIRs unable to close,
car on stands or not connected at all. The aim of steps 4-7 is to prove the firmware boots and
behaves before it can ever command torque.

You need:

- the VCU powered from a bench supply (or the car's LV system with HV isolated)
- an ST-Link (V2 or V3) on the VCU's SWD header
- a serial adapter for the log output (RS232 or the debug header, see step 5)
- a CAN interface on CAN-T (and ideally CAN-S) to watch traffic:
  - **CAN-T** (tractive/critical bus, CAN2): **500 kbit/s**
  - **CAN-S** (sensor bus, CAN1): **1 Mbit/s**
- the dash (TS and R2D buttons, LEDs) and pedal box if available, otherwise you can still do most
  checks with the CAN tool

---

## Step 4: Flash the VCU

With the ST-Link connected and the VCU powered:

```sh
make flash                 # flashes build/release/VCU.bin with st-flash
```

(`st-flash` comes from the STLink tools: `brew install stlink` on macOS.) Alternatives:
STM32CubeProgrammer (open `build/release/VCU.elf`, then "Download"), or start a debug session
from VS Code (step 9), which flashes the debug build.

After flashing, the VCU resets and starts straight away.

---

## Step 5: Watch the log output

The VCU prints its log on two serial ports at **115200 baud, 8N1**:

- **RS232** connector (UART8)
- **debug header** (USART1 TX)

Open either with a serial terminal, for example `screen /dev/tty.usbserial-XXXX 115200` (macOS) or
PuTTY (Windows). Each line looks like `<ms since boot> [LEVEL]: message`. A normal boot shows:

```
0 [INFO]: RTCAN service 0 started
0 [INFO]: RTCAN service 1 started
0 [INFO]: Enabling SD card            (or "SD card not detected!" without a card)
0 [INFO]: SD card init success
1 [INFO]: Logging service started, min level: INFO
... [INFO]: SD: logging to log_N.txt
```

Signs of trouble:

- `Some services failed to initialise` - a service didn't start; the control task is then
  stopped on purpose and the car can't be driven (see [Troubleshooting](#troubleshooting))
- `PM100 broadcast timeout` every 10 s - nothing heard from the inverter on CAN-T (expected with
  the inverter off)
- `Remote control: broadcast timeout` once after power-up - the remote control unit isn't sending
  (normal unless you use remote control mode)

---

## Step 6: Low-voltage bench tests

Tick each item off. "Expected" is what the firmware is designed to do.

### 6.1 Boot

- [ ] The heartbeat LED blinks at about **2 Hz** (toggles every 250 ms).
- [ ] All dash LEDs come on for **2 s** at power-up (visual check), then go off.
- [ ] Log lines appear on the serial port (step 5).
- [ ] Nothing is switched on: TS_ON (AIR) output off, RTDS silent.
- [ ] After 10 s the `RAM:` report lines appear in the log (see [step 1b](#step-1b-check-the-ram-budget)).

### 6.2 CAN broadcasts

Watch the buses with the CAN tool:

| Frame | ID | Bus | Expected rate |
| ----- | -- | --- | ------------- |
| VCU state | 0x101 | CAN-T | 10 Hz |
| VCU errors | 0x102 | CAN-T | 10 Hz |
| VCU PDM (inverter/pump/fan/brake light power) | 0x104 | CAN-T | 10 Hz |
| VCU sensors (APPS, BPS, torque request, ...) | 0x100 | CAN-S | 10 Hz |
| Wheel RPM | 0x250 | CAN-S | 50 Hz |

- [ ] All frames present at those rates, no CAN error frames.
- [ ] The VCU state frame shows control state 0 (TS_BUTTON_WAIT).

### 6.3 Inputs (decode with the DBC from the `can-defs` repo)

- [ ] Pedal released: APPS = 0 in the sensors frame. Fully pressed: ~1000. Smooth in between.
- [ ] Unplug one APPS sensor: the SCS error field in the errors frame becomes non-zero.
- [ ] Brake pressed: BPS rises; after holding it for 0.3 s the brake light bit in the PDM frame
      turns on (and the brake light itself, if the PDM is connected).
- [ ] Turn each wheel speed sensor's target: the wheel RPM frame changes.
- [ ] Mode switch: the requested mode in the state frame follows the switch (1 = endurance,
      2 = max, 3 = torque control, 4 = crawl, 5 = reverse, 6 = remote, 10 = inverter programming).

### 6.4 Watchdog

The independent watchdog (IWDG) restarts the VCU if the control loop stops running for longer
than about 0.5 s (0.34-0.94 s depending on the chip's internal oscillator). See
`src/SUFST/Src/Interfaces/watchdog.c` for the details.

- [ ] Normal boot: no `VCU was restarted by the watchdog` line in the log, and bit 0x20 is not set
      in the control error field of the VCU error frame (0x102).
- [ ] Deliberate test (with the debugger, step 9): pause the VCU, run
      `-exec call vTaskSuspend(vcu.ctrl.thread)` in the VS Code debug console, then continue.
      Within ~0.5 s the VCU must restart: the heartbeat LED restarts, the log shows
      `VCU was restarted by the watchdog`, and the error frame shows bit 0x20.
- [ ] Pausing at a breakpoint does **not** restart the VCU (the watchdog is frozen while the
      debugger halts the CPU).
- [ ] With a scope or meter on the TS_ON and inverter power (STATUS) outputs, check what they do
      during the restart: they must not switch on while the MCU is in reset (this depends on the
      board's pull resistors, see the note below).

### 6.5 State machine without HV (no precharge possible)

- [ ] With the shutdown circuit **open**, hold TS for 1 s and release: nothing happens.
- [ ] Close the shutdown circuit, hold TS for 1 s and release: TS_ON output switches on, 250 ms
      later the inverter power bit in the PDM frame turns on.
- [ ] Without HV the inverter can't precharge, so after **3 s** the VCU must go to a TS fault:
      TS_ON output off, TS-on LED blinking, error frame shows the precharge-timeout bit (0x04),
      log shows `Precharge timeout reached`. **This proves the fault path works.**
- [ ] Hold TS for 1 s and release to acknowledge: state goes back to TS_BUTTON_WAIT.
- [ ] A short press (under 0.5 s) of TS or R2D is always ignored.
- [ ] Press TS again (shutdown closed), then open the shutdown circuit within 3 s: TS fault
      within ~30 ms (not after the 3 s timeout).
- [ ] Measure the TS_ON and inverter power (STATUS) outputs right at power-up, and in USB mode
      (step 7): both must be off.
- [ ] At the end of the bench session, note the `RAM:` report: every task should still have spare
      stack (step 1b).

---

## Step 7: SD card and USB mass storage tests

### SD logging

- [ ] Insert a FAT32-formatted micro SD card and power up: the log shows `SD: logging to log_N.txt`
      (N is the first unused number).
- [ ] Cause a warning or error (e.g. unplug an APPS sensor, or do the precharge timeout test).
- [ ] Wait 10 s (the file is synced to the card every 10 s), power off, read the card on a PC:
      the file contains the WARN/ERROR lines (INFO lines are not written to the card).
- [ ] Power up without a card: the log says `SD card not detected!` and everything else works.

### USB mass storage mode

- [ ] Hold the **USER** button while powering up: the heartbeat LED blinks fast, and the log says
      `USB MSC: active, power cycle to exit`.
- [ ] Connect USB to a PC: a drive called "SUFST VCU SD Card" appears; the log files can be read,
      copied and deleted.
- [ ] Power cycle without the button to return to normal mode. The car can't be driven in USB mode.

---

## Step 8: Tests with the inverter and high voltage

**Follow the team's HV procedures and only with HV-qualified people present. Car on stands,
wheels off the ground, e-stop within reach.** Do these only after steps 1-7 pass.

### 8.1 First, check the inverter fault reporting

The port fixed a bug: the VCU used to ignore the inverter's whole **run fault low word** (so
faults like overcurrent or overspeed in that word never stopped the car). Those faults are now
latched. Before driving:

- [ ] Power the inverter (LV) and check the PM100 fault codes frame (0x0AB) with the CAN tool:
      **all four fault words should be 0**. If one is set during normal operation, the VCU will now
      refuse to drive (TS fault, `vcu_pm100_error` set) - find out why before going further.

### 8.2 Activation and driving

- [ ] TS button: AIRs close, precharge completes (log `Precharge complete`), state R2D_WAIT.
- [ ] R2D without brake: refused. With brake + 1 s press: RTDS sounds for 2 s, then TS_ON
      (state 4). The R2D LED changes. (The control loop keeps running during the RTDS, so opening
      the shutdown circuit during the sound gives a TS fault straight away.)
- [ ] Gentle pedal: wheels turn forward. Torque request (sensors frame) rises with the pedal and
      tops out at the mode's cap (endurance 110 Nm, max 250 Nm, crawl/reverse 20 Nm).
- [ ] Reverse mode: wheels turn backwards, RTDS beeps once a second while in R2D.
- [ ] Pedal released at standstill: the inverter enable bit in the command frame (0x0C0) goes to 0.
- [ ] R2D button while driving: torque goes to 0 immediately, 0.5 s later back to R2D_WAIT.
- [ ] TS button in R2D_WAIT: AIRs open, inverter power off.

### 8.3 Faults (each must cut torque immediately)

- [ ] Brake hard + more than 30 % pedal: APPS/BPS fault, torque 0. Recovery needs the pedal
      below 5 % (with brake released) for 0.1 s.
- [ ] Unplug an APPS sensor while driving: APPS fault, torque 0; plug back in: recovers.
- [ ] Press the e-stop / open the shutdown circuit while the wheels are turning: TS fault,
      torque 0, AIRs open. The inverter stays powered on purpose (the motor may still spin), and
      the fan and pump stay on while the inverter or motor is above 50 °C.
- [ ] Acknowledge each TS fault with the TS button.

---

## Step 9: Debugging with FreeRTOS awareness

The **"debug (openocd)"** configuration in VS Code (`.vscode/launch.json`) builds the debug
firmware, flashes it and starts the debugger with FreeRTOS awareness: the call stack panel lists
every task (Control, PM100, TICK, CANBC, LOG, ...) with its state, and you can look at each
task's stack. Needs the Cortex-Debug extension and OpenOCD (`brew install open-ocd`).

Handy things to look at when stopped:

- `vcu.ctrl.state` - control state (0 = TS_BUTTON_WAIT ... 4 = TS_ON, see `ctrl.h`)
- `vcu.ctrl.error`, `vcu.pm100.error` - latched error bits
- `vcu.pm100.faults` - the inverter's raw fault words
- `xFreeBytesRemaining` - free FreeRTOS heap

Each task's unused stack can be checked with `uxTaskGetStackHighWaterMark()`; stack sizes are set
in `config.c` (in bytes).

---

## Troubleshooting

| What you see | Meaning | What to do |
| ------------ | ------- | ---------- |
| Heartbeat LED blinks at 2 Hz | normal | - |
| Heartbeat blinks fast (~10 Hz) and the log says `USB MSC: active` | USB mass storage mode (USER button held at power-up) | power cycle without the button |
| Heartbeat blinks fast (~10 Hz) and the log says `Some services failed to initialise` | a service failed to start; the control task is stopped on purpose so the car can't be driven, and the TS is kept off | check the log lines just before it; usually the FreeRTOS heap (`configTOTAL_HEAP_SIZE` in `FreeRTOSConfig.h`) is too small after adding a task or making a stack bigger |
| LED blinks at ~5 Hz and **all CAN traffic stops** | `Error_Handler()`: a HAL error, a task stack overflow, the heap running out, a FreeRTOS `configASSERT` or a CPU fault (hard fault, bus fault, ...). Outputs are forced safe (TS off, RTDS off) | connect the debugger and halt: the call stack shows how it got there; for a stack overflow, increase that task's `stack_size` in `config.c` |
| LED stopped (stuck on or off), no CAN, no log at power-up | the VCU didn't get as far as starting FreeRTOS (clock or HAL init failure before the scheduler) | connect the debugger and halt |
| TS fault ~3 s after pressing TS | precharge didn't complete (no HV, inverter not reporting precharge, or inverter not on CAN-T) | check HV, the PM100 internal states frame (0x0AA) VSM state should reach 3 |
| TS fault as soon as torque is requested | the inverter reported a fault, or is in lockout | read the PM100 fault codes frame (0x0AB), see step 8.1 |
| `VCU was restarted by the watchdog` in the log, bit 0x20 in the error frame | the control loop stopped for > ~0.5 s and the watchdog restarted the VCU | a hang that should never happen: note what the car was doing and check the log/SD card just before it; the flag clears with the next fault acknowledgement or power cycle |
| `PM100 broadcast timeout` in the log | no inverter frames on CAN-T for 10 s | inverter power, CAN-T wiring/termination, bitrate 500 kbit/s |
| `CAN error 0x... on CAN_T/CAN_S` (at most once a second) | CAN bus errors (no other node, wiring, termination, bitrate) | check the bus; the VCU keeps running |
| No `SD: logging to ...` line | no card, card not FAT32, or card faulty | try another card, format FAT32 |

---

## Watchdog: what it does and doesn't do

- It is started by the control task on its first cycle and kicked every 10 ms by the control loop.
  If the control loop stops (deadlock, stuck loop, starved by another task, CPU lockup, memory
  corrupted by electrical noise) the MCU restarts and boots into the safe idle state (TS off,
  waiting for the TS button).
- It is **not** started if initialisation fails (the car can't be driven and the heartbeat blinks
  fast) or in USB mode, so those don't turn into a restart loop.
- In `Error_Handler()` (detected faults) it keeps being kicked, so the VCU stays in that safe state
  with the LED blinking instead of restarting, and the fault can be inspected with the debugger.
- It only watches the control loop. If another task stops (e.g. CAN broadcasts), the control loop
  still runs and the watchdog doesn't trigger.
- **Check on the schematic** what TS_ON (PE13) and inverter power (STATUS, PE15) do while the MCU
  is in reset: during a reset the pins float until the firmware configures them (now to "off"), so
  the board's pull-up/pull-down resistors decide their state for those few milliseconds.

## Known behaviour to decide on as a team

These are not changed by the port; the tests document them.

- **TS-on LED**: blinks during a TS fault but keeps blinking after the fault is acknowledged.
- **PM100 going silent while driving** is only detected after 10 s
  (`pm100.broadcast_timeout_ticks` in `config.c`); the VCU keeps sending torque requests until
  then. (The inverter's own CAN timeout should also stop it.) Consider a much shorter timeout.
- **After acknowledging an APPS fault** with the TS button, the state goes back to
  TS_BUTTON_WAIT and inverter power is switched off, but the AIRs (TS_ON) are left closed.
- The shutdown circuit isn't read directly while driving; a shutdown opening is detected through
  the inverter losing HV (tested: within 50 ms).

## Before committing

- Run `tests/run_tests.sh` (step 1).
- New folders that must be added to git: `src/Middlewares/Third_Party/`,
  `src/Middlewares/ST/STM32_USB_Device_Library/`, `src/Drivers/CMSIS/RTOS2/`, `src/FATFS/`,
  `src/USB_DEVICE/`, `tests/`, and the new files in `src/SUFST` and `src/Core`
  (`git status` lists them). `tests/build/` is ignored.
- If you regenerate code with STM32CubeMX, read the notes in
  [code-overview.md](code-overview.md#stm32cubemx-generation) first.
