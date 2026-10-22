#!/bin/bash
#
# Runs the VCU test suite on a PC (macOS or Linux, needs a C compiler):
#
#   1. firmware build check  (only if arm-none-eabi-gcc is installed)
#   2. unit tests            (torque map, limiters, APPS/BPS, ... with the real config)
#                            + launch control tests (with its default tuning)
#   3. simulation scenarios  (the real VCU code on FreeRTOS with a fake HAL)
#
# Usage:
#   tests/run_tests.sh                 run everything
#   tests/run_tests.sh -k mode         only scenarios whose name contains "mode"
#   tests/run_tests.sh -j 8            run 8 scenarios in parallel (default 4)
#   tests/run_tests.sh --no-firmware   skip the ARM firmware build
#   tests/run_tests.sh -v              print every scenario's full output
#
# Logs of every scenario are kept in tests/build/logs/<scenario>.log
# Exit code is 0 only if everything passed.

set -u
TESTS="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$TESTS/.." && pwd)"
V="${VCU_SRC:-$ROOT/src}" # VCU_SRC=... tests another copy of the source
B="$TESTS/build"
SIM="$TESTS/sim"

# ------------------------------------------------------------------ one scenario
# (called through xargs, runs a single scenario with its environment)
if [ "${1:-}" = "--one" ]; then
    name="$2"
    log="$B/logs/$name.log"
    img="$B/sd/$name.img"
    export VCU_SIM_SD_IMAGE=""
    export VCU_SIM_SD_FAIL_AFTER=""
    export VCU_SIM_WATCHDOG_RESET=""
    case "$name" in
    watchdog_reset_reported)
        export VCU_SIM_WATCHDOG_RESET=1 ;;
    sd_logging)
        "$B/fatimg" mkfs "$img" 32 && "$B/fatimg" put "$img" log_1.txt "previous session\n" || exit 1
        export VCU_SIM_SD_IMAGE="$img" ;;
    sd_write_error)
        "$B/fatimg" mkfs "$img" 32 || exit 1
        export VCU_SIM_SD_IMAGE="$img" VCU_SIM_SD_FAIL_AFTER=8 ;;
    usb_mode|launch_ctrl|launch_ctrl_shadow)
        "$B/fatimg" mkfs "$img" 32 || exit 1
        export VCU_SIM_SD_IMAGE="$img" ;;
    esac

    # runs the scenario with a watchdog: the FreeRTOS POSIX port used by the
    # simulator can (rarely) deadlock on macOS when the tick signal hits two
    # threads at once. That is a simulator problem, not a VCU one, so a hung
    # run is killed and retried once.
    run_once() {
        "$B/vcu_sim" "$name" >"$log" 2>&1 &
        local pid=$!
        ( sleep 150; kill -9 $pid 2>/dev/null ) >/dev/null 2>&1 &
        local wd=$!
        wait $pid
        local r=$?
        kill $wd 2>/dev/null
        wait $wd 2>/dev/null
        return $r
    }
    start=$(date +%s)
    retried=""
    run_once
    rc=$?
    if [ $rc -eq 137 ]; then
        retried=" (simulator hung, retried)"
        run_once
        rc=$?
    fi

    # SD logging: read the card image back and check what was written
    if [ "$name" = "sd_logging" ] && [ $rc -eq 0 ]; then
        {
            echo "== SD card contents"
            "$B/fatimg" ls "$img"
            "$B/fatimg" cat "$img" log_2.txt >"$B/sd/log_2.txt"
            n_burst=$(grep -c "\[WARN\]: burst " "$B/sd/log_2.txt")
            n_apps=$(grep -c "\[WARN\]: APPS" "$B/sd/log_2.txt")
            n_info=$(grep -c "\[INFO\]" "$B/sd/log_2.txt")
            old=$("$B/fatimg" cat "$img" log_1.txt)
            pass=0; total=0
            chk() { total=$((total + 1)); if [ "$1" = 1 ]; then pass=$((pass + 1)); echo "  PASS: $2"; else echo "  FAIL: $2"; fi; }
            chk $([ "$n_burst" -eq 200 ] && echo 1 || echo 0) "all 200 burst warnings on the card, none dropped ($n_burst)"
            chk $([ "$n_apps" -ge 1 ] && echo 1 || echo 0) "APPS discrepancy warnings on the card ($n_apps)"
            chk $([ "$n_info" -eq 0 ] && echo 1 || echo 0) "INFO lines not written to the card (min SD level WARN)"
            chk $(echo "$old" | grep -q "^previous session" && echo 1 || echo 0) "existing log_1.txt left untouched"
            echo "RESULT: $pass/$total card checks passed"
            [ $pass -eq $total ]
        } >>"$log" 2>&1 || rc=1
    fi

    # launch control: read the LC lines back from the card
    # (LC,<tick>,<state>,<fault>,<driver>,<limit>,<output>,<v_ground>,<v_rear>,<v_front>)
    if { [ "$name" = "launch_ctrl" ] || [ "$name" = "launch_ctrl_shadow" ]; } && [ $rc -eq 0 ]; then
        {
            echo "== launch control lines on the SD card"
            "$B/fatimg" cat "$img" log_1.txt | grep "^LC," >"$B/sd/$name.csv"
            lc() { awk -F, "$1" "$B/sd/$name.csv" | wc -l | tr -d ' '; }
            n_all=$(lc '1')
            n_data=$(lc '$3 == 2')
            n_cut=$(lc '$3 == 2 && $7 < $5')
            n_want=$(lc '$3 == 2 && $6 < $5')
            n_done=$(lc '$3 == 3')
            n_armed=$(lc '$3 == 1')
            n_sign=$(lc '$3 == 4 && $4 == 3')
            n_bad=$(lc '$7 > $5')
            head -3 "$B/sd/$name.csv"; echo "..."
            pass=0; total=0
            chk() { total=$((total + 1)); if [ "$1" = 1 ]; then pass=$((pass + 1)); echo "  PASS: $2"; else echo "  FAIL: $2"; fi; }
            chk $([ "$n_data" -ge 50 ] && echo 1 || echo 0) "ACTIVE data lines on the card at ~50 Hz ($n_data of $n_all lines)"
            chk $([ "$n_bad" -eq 0 ] && echo 1 || echo 0) "no line with output above the driver request"
            chk $([ "$n_want" -ge 1 ] && echo 1 || echo 0) "lines where the limit is below the driver request ($n_want)"
            chk $([ "$n_done" -ge 1 ] && echo 1 || echo 0) "launch end (DONE) logged ($n_done)"
            chk $([ "$n_armed" -ge 1 ] && echo 1 || echo 0) "arming (ARMED) logged ($n_armed)"
            if [ "$name" = "launch_ctrl" ]; then
                chk $([ "$n_cut" -ge 1 ] && echo 1 || echo 0) "lines where torque was actually cut ($n_cut)"
                chk $([ "$n_sign" -ge 1 ] && echo 1 || echo 0) "MOTOR_REVERSE fault logged ($n_sign)"
            else
                chk $([ "$n_cut" -eq 0 ] && echo 1 || echo 0) "shadow mode: torque never cut ($n_cut)"
            fi
            echo "RESULT: $pass/$total card checks passed"
            [ $pass -eq $total ]
        } >>"$log" 2>&1 || rc=1
    fi

    secs=$(( $(date +%s) - start ))
    summary=$(grep -h "^RESULT:" "$log" | tr '\n' ' ')
    if [ $rc -eq 0 ]; then st="PASS"; else st="FAIL"; fi
    [ $rc -eq 137 ] && summary="simulator hung twice"
    printf "%-4s  %-32s %4ss  %s%s\n" "$st" "$name" "$secs" "$summary" "$retried" | tee "$B/results/$name"
    exit 0
fi

# ---------------------------------------------------------------------- options
JOBS=4
FILTER=""
FIRMWARE=1
VERBOSE=0
while [ $# -gt 0 ]; do
    case "$1" in
    -j) JOBS="$2"; shift ;;
    -k) FILTER="$2"; shift ;;
    --no-firmware) FIRMWARE=0 ;;
    -v) VERBOSE=1 ;;
    -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
    *) echo "unknown option $1"; exit 2 ;;
    esac
    shift
done

CC="${CC:-cc}"
mkdir -p "$B/logs" "$B/results" "$B/sd" "$B/fatimg_conf"
rm -f "$B"/results/* "$B"/logs/* "$B"/sd/*
failed=0

section() { printf "\n\033[1m== %s\033[0m\n" "$1"; }

# ------------------------------------------------------------ 1. firmware build
if [ $FIRMWARE -eq 1 ]; then
    section "Firmware build (arm-none-eabi-gcc)"
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        for cfg in "" "DEBUG=1"; do
            label=${cfg:-release}
            if out=$(make -C "$ROOT" -j8 $cfg 2>&1); then
                warns=$(echo "$out" | grep -E ": warning:" | grep -v "log.c:.*Wformat-truncation" | grep -v "RWX permissions" | sort -u)
                if [ -n "$warns" ]; then
                    echo "WARN  $label build has new warnings:"; echo "$warns"
                else
                    echo "PASS  $label build"
                fi
            else
                echo "FAIL  $label build"; echo "$out" | grep -E "error" | head -20; failed=1
            fi
        done
        # worst-case stack of every task + RAM budget, from the built firmware
        for cfg in release debug; do
            if python3 "$TESTS/tools/stack_analysis.py" "$ROOT/build/$cfg/VCU.elf" >"$B/logs/stack_$cfg.log" 2>&1; then
                echo "PASS  $cfg stack/RAM analysis: $(grep "peak use" "$B/logs/stack_$cfg.log" | sed 's/^ *//')"
            else
                echo "FAIL  $cfg stack/RAM analysis (see tests/build/logs/stack_$cfg.log):"
                grep -E "LOW|OVERFLOW|too small" "$B/logs/stack_$cfg.log"; failed=1
            fi
        done
    else
        echo "SKIP  arm-none-eabi-gcc not installed"
    fi
fi

# ----------------------------------------------------------------- common flags
INC="-I$SIM/inc -I$V/SUFST/Inc -I$V/SUFST/Inc/Functions -I$V/SUFST/Inc/Interfaces -I$V/SUFST/Inc/Services
-I$V/SUFST/Middlewares/rtcan/inc -I$V/SUFST/Middlewares/can-defs/out -I$V/Middlewares/Third_Party/FreeRTOS/Source/include
-I$SIM/posix_port -I$SIM/posix_port/utils -I$V/Drivers/CMSIS/RTOS2/Include
-I$V/Middlewares/Third_Party/FatFs/src -I$V/FATFS/Target"
CFLAGS="-std=gnu11 -g -O1 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers
-Dsection(x)=used -pthread ${EXTRA_CFLAGS:-}"

# ----------------------------------------------------------------- 2. unit tests
section "Unit tests"
if $CC $CFLAGS $INC "$TESTS/unit/test_units.c" "$V/SUFST/Src/config.c" "$V"/SUFST/Src/Functions/*.c \
    "$V/SUFST/Src/Interfaces/scs.c" "$V/SUFST/Src/Interfaces/apps.c" "$V/SUFST/Src/Interfaces/bps.c" \
    -lm -o "$B/unit_tests" 2>"$B/logs/unit_build.log"; then
    if "$B/unit_tests" >"$B/logs/unit_tests.log" 2>&1; then
        echo "PASS  $(grep RESULT "$B/logs/unit_tests.log")"
    else
        echo "FAIL  unit tests:"; grep -E "FAIL|RESULT" "$B/logs/unit_tests.log"; failed=1
    fi
else
    echo "FAIL  unit tests did not compile:"; head -30 "$B/logs/unit_build.log"; failed=1
fi

# launch control: pure C, no VCU headers, so built strictly and with sanitizers
# (AddressSanitizer only where it works: some macOS versions crash it at start-up)
SANITIZE="-fsanitize=undefined"
echo 'int main(void){return 0;}' >"$B/asan_probe.c"
if $CC -fsanitize=address "$B/asan_probe.c" -o "$B/asan_probe" 2>/dev/null && sh -c "\"$B/asan_probe\" 2>/dev/null" 2>/dev/null; then
    SANITIZE="-fsanitize=address,undefined"
fi
if $CC -std=c11 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion -O2 -g \
    $SANITIZE -I"$V/SUFST/Inc/Functions" \
    "$TESTS/unit/test_launch_ctrl.c" "$TESTS/unit/launch_ctrl_config_default.c" \
    "$V/SUFST/Src/Functions/launch_ctrl.c" -lm -o "$B/launch_ctrl_tests" 2>"$B/logs/launch_ctrl_build.log"; then
    if "$B/launch_ctrl_tests" >"$B/logs/launch_ctrl_tests.log" 2>&1; then
        echo "PASS  launch control $(grep RESULT "$B/logs/launch_ctrl_tests.log")"
    else
        echo "FAIL  launch control tests:"; grep -E "FAIL|RESULT" "$B/logs/launch_ctrl_tests.log"; failed=1
    fi
else
    echo "FAIL  launch control tests did not compile:"; head -30 "$B/logs/launch_ctrl_build.log"; failed=1
fi

# ---------------------------------------------------------- 3. simulation build
section "Simulation build"
# FAT image tool: same FatFs + config as the firmware, plus f_mkfs
sed -E 's/^#define FF_USE_MKFS[[:space:]]+0/#define FF_USE_MKFS 1/' "$V/FATFS/Target/ffconf.h" >"$B/fatimg_conf/ffconf.h"
$CC -std=gnu11 -O1 -I"$B/fatimg_conf" -I"$V/Middlewares/Third_Party/FatFs/src" \
    "$TESTS/tools/fatimg.c" "$V/Middlewares/Third_Party/FatFs/src/ff.c" -o "$B/fatimg" || { echo "FAIL  fatimg"; exit 1; }

SRCS="$(ls "$V"/SUFST/Src/*.c "$V"/SUFST/Src/Services/*.c "$V"/SUFST/Src/Functions/*.c "$V"/SUFST/Src/Interfaces/*.c \
        | grep -v -e 'adc_scan.c' -e 'sd_card.c' -e 'rtos_utils.c' -e 'watchdog.c')
$V/SUFST/Middlewares/rtcan/src/rtcan.c $V/SUFST/Middlewares/rtcan/src/rtcan_osal_freertos.c
$V/SUFST/Middlewares/can-defs/out/can_t.c $V/SUFST/Middlewares/can-defs/out/can_s.c
$V/Middlewares/Third_Party/FatFs/src/ff.c $V/FATFS/Target/sd_diskio.c
$(ls "$V"/Middlewares/Third_Party/FreeRTOS/Source/*.c) $V/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
$SIM/posix_port/port.c $SIM/posix_port/utils/wait_for_event.c
$(ls "$SIM"/src/*.c)"

# rtos_utils.c is built separately so task creation failures can be injected
if $CC $CFLAGS $INC -DxTaskCreate=sim_xTaskCreate -c "$V/SUFST/Src/rtos_utils.c" -o "$B/rtos_utils.o" \
        2>"$B/logs/sim_build.log" &&
   $CC $CFLAGS $INC $SRCS "$B/rtos_utils.o" -lm -o "$B/vcu_sim" 2>>"$B/logs/sim_build.log"; then
    if grep -q "warning:" "$B/logs/sim_build.log"; then
        echo "PASS  (with warnings, see tests/build/logs/sim_build.log)"
    else
        echo "PASS"
    fi
else
    echo "FAIL  simulation did not compile:"; grep -E "error" "$B/logs/sim_build.log" | head -30; exit 1
fi

# ------------------------------------------------------- 4. simulation scenarios
section "Simulation scenarios (real time, $JOBS in parallel)"
names=$("$B/vcu_sim" --list | awk '{print $1}' | grep -e "$FILTER")
n=$(echo "$names" | wc -l | tr -d ' ')
echo "$n scenarios"
echo "$names" | xargs -P "$JOBS" -I{} "$0" --one {}

section "Summary"
pass=$(grep -l "^PASS" "$B"/results/* 2>/dev/null | wc -l | tr -d ' ')
fail=$(grep -l "^FAIL" "$B"/results/* 2>/dev/null | wc -l | tr -d ' ')
echo "scenarios: $pass passed, $fail failed (of $n)"
if [ "$fail" -gt 0 ]; then
    failed=1
    for f in $(grep -l "^FAIL" "$B"/results/*); do
        s=$(basename "$f")
        echo; echo "--- $s (tests/build/logs/$s.log)"
        grep -E "FAIL|RESULT|ASSERT|ERROR_HANDLER|MALLOC" "$B/logs/$s.log" | head -20
    done
fi
notes=$(grep -h "NOTE:" "$B"/logs/*.log 2>/dev/null)
[ -n "$notes" ] && { echo; echo "Notes:"; echo "$notes"; }
[ $VERBOSE -eq 1 ] && cat "$B"/logs/*.log

echo
if [ $failed -eq 0 ]; then echo "ALL TESTS PASSED"; else echo "SOME TESTS FAILED"; fi
exit $failed
