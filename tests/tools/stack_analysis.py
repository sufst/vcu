#!/usr/bin/env python3
"""
Worst-case stack usage of every FreeRTOS task in the VCU firmware, and a RAM
budget, computed from the built ELF file.

How it works: the firmware is disassembled (arm-none-eabi-objdump), the stack
frame of every function is read from its prologue (push / vpush / sub sp),
and the call graph is built from the branch instructions. The worst case for
a task is the deepest path through the call graph from its entry function.
This includes the C library (printf etc.), which -fstack-usage can't see.

Calls through function pointers can't be followed automatically; the ones
that matter are listed in INDIRECT below. Any other indirect call site found
on a task's path is reported so it can be checked.

Usage:
    tests/tools/stack_analysis.py [build/release/VCU.elf] [--verbose]
"""
import re
import subprocess
import sys
from collections import defaultdict

ELF = next((a for a in sys.argv[1:] if not a.startswith("-")), "build/release/VCU.elf")
VERBOSE = "--verbose" in sys.argv

# Task entry functions -> (task name, stack size in bytes from config.c / vcu.c / FreeRTOSConfig.h)
# None = stack size read elsewhere (see notes in the output)
TASKS = [
    ("ctrl_thread_entry", "Control", 2048),
    ("pm100_thread_entry", "PM100", 2048),
    ("tick_thread_entry", "TICK", 2048),
    ("canbc_thread_entry", "CANBC", 2048),
    ("dash_thread_entry", "Dash", 2048),
    ("log_thread_entry", "LOG", 2048),
    ("sd_thread_entry", "SD", 8192),
    ("wheelspeed_thread_entry", "WHEELSPEED", 2048),
    ("fans_thread_entry", "FANS", 2048),
    ("remote_ctrl_thread_entry", "REMOTE", 2048),
    ("heartbeat_thread_entry", "HEARTBEAT", 2048),
    ("usb_msc_thread_entry", "USB_MSC (USB mode only)", 2048),
    ("vcu_init_task", "VCU init (start-up only)", 2048),
    ("rtcan_tx_thread_entry", "RTCAN Tx (x2)", 1024),
    ("rtcan_rx_thread_entry", "RTCAN Rx (x2)", 1024),
    ("prvTimerTask", "Tmr Svc (FreeRTOS timers)", 256 * 4),
    ("prvIdleTask", "IDLE", 256 * 4),
]

# Calls through function pointers that the analysis must know about:
# function containing the indirect call -> possible targets
INDIRECT = {
    "torque_map_apply": ["linear_torque_map", "exponential_torque_map", "null_torque_map"],
    "prvProcessExpiredTimer": ["toggle_ts_on_callback"],
    "prvProcessReceivedCommands": ["toggle_ts_on_callback"],
    "prvTimerTask": ["toggle_ts_on_callback"],
    # newlib-nano printf calls its output function through a pointer
    "_printf_i": ["__ssputs_r", "__sfputs_r"],
    "_printf_common": ["__ssputs_r", "__sfputs_r"],
    "_svfiprintf_r": ["_printf_i", "_printf_common", "__ssputs_r"],
}

# Interrupt handlers (run on the main stack, MSP) by NVIC priority; handlers
# with the same priority can't nest, a higher one can interrupt a lower one.
ISRS = {
    0: ["ADC_IRQHandler", "DMA2_Stream0_IRQHandler", "DMA2_Stream2_IRQHandler"],
    5: ["CAN1_TX_IRQHandler", "CAN1_RX0_IRQHandler", "CAN1_RX1_IRQHandler", "CAN1_SCE_IRQHandler",
        "CAN2_TX_IRQHandler", "CAN2_RX0_IRQHandler", "CAN2_RX1_IRQHandler", "CAN2_SCE_IRQHandler"],
    6: ["EXTI9_5_IRQHandler", "EXTI15_10_IRQHandler"],
    7: ["SDMMC1_IRQHandler", "DMA2_Stream3_IRQHandler", "DMA2_Stream6_IRQHandler"],
    15: ["TIM6_DAC_IRQHandler", "OTG_FS_IRQHandler", "SysTick_Handler", "PendSV_Handler"],
}

# Cortex-M7 with FPU: an exception pushes up to 26 words onto the interrupted
# stack; a FreeRTOS context switch saves another 9 + 16 (FPU) words on the
# task stack.
EXCEPTION_FRAME = 26 * 4
CONTEXT_SWITCH = 25 * 4


def objdump(elf):
    out = subprocess.run(["arm-none-eabi-objdump", "-d", "--no-show-raw-insn", elf],
                         capture_output=True, text=True, check=True).stdout
    funcs = {}   # name -> dict(addr, insns)
    order = []
    cur = None
    for line in out.splitlines():
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
        if m:
            cur = m.group(2)
            funcs[cur] = {"addr": int(m.group(1), 16), "insns": []}
            order.append(cur)
            continue
        m = re.match(r"^\s+([0-9a-f]+):\s+(\S+)\s*(.*)$", line)
        if m and cur:
            funcs[cur]["insns"].append((int(m.group(1), 16), m.group(2), m.group(3)))
    return funcs


def reglist_size(ops):
    m = re.search(r"\{(.*)\}", ops)
    if not m:
        return 0
    n = 0
    for part in m.group(1).split(","):
        part = part.strip()
        if "-" in part:
            a, b = part.split("-")
            pa = re.match(r"([a-z]+)(\d+)", a.strip())
            pb = re.match(r"([a-z]+)(\d+)", b.strip())
            if pa and pb:
                cnt = int(pb.group(2)) - int(pa.group(2)) + 1
                n += cnt * (8 if pa.group(1) == "d" else 4)
        elif part:
            n += 8 if part.startswith("d") else 4
    return n


def analyse_function(insns, addr2func, name):
    """returns (frame bytes, dynamic?, callees set, indirect call?)"""
    frame = 0
    dynamic = False
    callees = set()
    indirect = False
    for i, (addr, op, ops) in enumerate(insns):
        if op in ("push", "push.w", "stmdb", "stmdb.w") and ("sp!" in ops or op.startswith("push")):
            frame += reglist_size(ops)
        elif op in ("vpush", "vpush.w", "vstmdb") and ("sp!" in ops or op.startswith("vpush")):
            frame += reglist_size(ops)
        elif op in ("sub", "sub.w", "subw") and re.match(r"sp,\s*(sp,\s*)?#", ops):
            frame += int(re.search(r"#(\d+)", ops).group(1))
        elif op in ("sub", "sub.w") and re.match(r"sp,\s*(sp,\s*)?r\d+", ops):
            dynamic = True
        elif op in ("bl", "blx", "b", "b.w", "b.n") or op.startswith("b") and re.match(r"^[0-9a-f]+ <", ops):
            m = re.match(r"^([0-9a-f]+) <([^>+]+)", ops)
            if m:
                target = int(m.group(1), 16)
                tname = addr2func.get(target)
                if tname and tname != name:
                    callees.add(tname)
            elif op in ("blx",) or (op == "bx" and "lr" not in ops):
                indirect = True
        if op == "blx" and re.match(r"^r\d+|^ip|^lr", ops):
            indirect = True
        if op == "bx" and not ops.startswith("lr"):
            indirect = True
    return frame, dynamic, callees, indirect


def main():
    funcs = objdump(ELF)
    addr2func = {f["addr"]: n for n, f in funcs.items()}
    info = {}
    for n, f in funcs.items():
        info[n] = analyse_function(f["insns"], addr2func, n)

    graph = {n: set(i[2]) for n, i in info.items()}
    for caller, targets in INDIRECT.items():
        if caller in graph:
            graph[caller] |= {t for t in targets if t in graph}

    memo = {}
    warnings = defaultdict(set)

    def depth(fn, stack=()):
        if fn in memo:
            return memo[fn]
        if fn in stack:
            warnings["recursion"].add(" -> ".join(stack[stack.index(fn):] + (fn,)))
            return (0, [fn])
        frame, dynamic, _, indirect = info.get(fn, (0, False, set(), False))
        if dynamic:
            warnings["dynamic stack (VLA/alloca)"].add(fn)
        if indirect and fn not in INDIRECT:
            warnings["unresolved indirect call"].add(fn)
        best = (0, [])
        for c in graph.get(fn, ()):
            d = depth(c, stack + (fn,))
            if d[0] > best[0]:
                best = d
        res = (frame + best[0], [fn] + best[1])
        memo[fn] = res
        return res

    print(f"Stack analysis of {ELF}\n")
    print(f"{'task':28} {'entry function':28} {'worst case':>10} {'+ switch':>9} {'stack':>7} {'margin':>8}")
    total_ok = True
    for entry, tname, size in TASKS:
        if entry not in funcs:
            print(f"{tname:28} {entry:28} {'(not in this build)':>10}")
            continue
        warnings.clear()
        d, path = depth(entry)
        need = d + EXCEPTION_FRAME + CONTEXT_SWITCH
        margin = size - need
        flag = "" if margin >= size * 0.25 else ("  <-- LOW" if margin >= 0 else "  <-- OVERFLOW")
        if margin < size * 0.25:
            total_ok = False
        print(f"{tname:28} {entry:28} {d:>8} B {need:>7} B {size:>5} B {margin:>6} B{flag}")
        if VERBOSE or margin < size * 0.25:
            print("      deepest path: " + " -> ".join(f"{p}({info.get(p, (0,))[0]})" for p in path))
        for kind, items in warnings.items():
            items = sorted(i for i in items if not i.startswith("__"))
            if items and (VERBOSE or kind != "unresolved indirect call"):
                print(f"      note - {kind}: {', '.join(items[:6])}{' ...' if len(items) > 6 else ''}")

    # interrupts: worst nesting = sum over priority levels of the deepest handler at each level
    print("\nInterrupts (main stack, MSP):")
    msp = 0
    for prio in sorted(ISRS, reverse=True):
        worst = 0
        worst_name = "-"
        for h in ISRS[prio]:
            if h in funcs:
                d = depth(h)[0]
                if d > worst:
                    worst, worst_name = d, h
        msp += worst + EXCEPTION_FRAME
        print(f"  priority {prio:>2}: deepest {worst_name:28} {worst:>5} B (+{EXCEPTION_FRAME} B frame)")
    main_path = depth("main")[0] if "main" in funcs else 0
    print(f"  worst case with all priority levels nested: {msp} B; main() before the scheduler: {main_path} B")

    # static RAM from the ELF
    sym = {}
    out = subprocess.run(["arm-none-eabi-nm", ELF], capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    sizes = subprocess.run(["arm-none-eabi-size", "-A", ELF], capture_output=True, text=True).stdout
    sec = {}
    for line in sizes.splitlines():
        p = line.split()
        if len(p) >= 3 and p[0].startswith("."):
            sec[p[0]] = int(p[1])
    ram_total = 320 * 1024
    static = sec.get(".data", 0) + sec.get(".bss", 0)
    reserved = sec.get("._user_heap_stack", 0)  # linker-reserved heap + main stack
    free_top = sym.get("_estack", 0x20050000) - sym.get("_end", 0) - reserved if "_end" in sym else None
    print("\nRAM budget (320 KB):")
    print(f"  .data + .bss (globals, buffers, FreeRTOS heap): {static:>7} B  ({static * 100 / ram_total:.0f} %)")
    if free_top is not None:
        print(f"  main stack reserved by the linker: {reserved - 0x200:>7} B  "
              f"(worst case needs ~{msp + main_path} B)")
        print(f"  unused (not even reserved):        {free_top:>7} B")
        if msp + main_path > reserved - 0x200:
            total_ok = False
            print("  <-- main stack reservation (_Min_Stack_Size) too small")
    # FreeRTOS heap (heap_4): every task created with xTaskCreate/osThreadNew
    # takes its stack + TCB from it (+ 8 bytes heap header each, 8-byte aligned)
    heap = 65536
    tcb = 96 + 8
    dynamic_tasks = [(n, sz) for e, n, sz in TASKS
                     if e not in ("prvTimerTask", "prvIdleTask", "usb_msc_thread_entry")]
    used = 0
    for n, sz in dynamic_tasks:
        mult = 2 if "(x2)" in n else 1
        used += mult * (sz + 8 + tcb)
    used += 4 * (80 + 8)  # RTCAN semaphores (osSemaphoreNew)
    print(f"\nFreeRTOS heap ({heap} B, configTOTAL_HEAP_SIZE):")
    print(f"  peak use at start-up (all task stacks + TCBs + semaphores): {used} B  ({used * 100 / heap:.0f} %)")
    print(f"  free after start-up: {heap - used} B (the start-up task's 2 KB is given back once it ends)")

    print("\nResult: " + ("all task stacks have at least 25 % margin" if total_ok
                          else "some task stacks have less than 25 % margin (see above)"))
    return 0 if total_ok else 1


if __name__ == "__main__":
    sys.exit(main())
