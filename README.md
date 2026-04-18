# CPU Simulator

A CPU simulator in C implementing the fetch-decode-execute cycle and a 3-stage pipeline with hazard detection.

## Files
- `cpu_sim.c` — basic CPU with LOAD, STORE, ADD, SUB, MUL, MOV, JNZ, DEC, HALT
- `cpu_pip.c` — pipelined CPU with fetch/decode/execute stages and data hazard stall detection

## Features
- 8 general purpose registers, 256 bytes memory
- Basic CPU runs factorial of 5 (5! = 120) using a loop and conditional branch
- Pipelined CPU detects data hazards and inserts NOP stall bubbles
- Per-cycle trace showing all 3 pipeline stages simultaneously

## Sample pipeline output
7 instructions complete in 12 cycles with 2 stalls due to data hazards on R2 and R3.
