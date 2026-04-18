#include <stdio.h>
#include <stdlib.h>

// CPU configuration
#define MEMORY_SIZE 256
#define NUM_REGISTERS 8

// Instruction set
#define MUL   0x05   // MUL Rx, Ry, Rz   -> Rx = Ry * Rz
#define MOV   0x06   // MOV Rx, val       -> Rx = immediate value
#define JNZ   0x07   // JNZ Rx, addr      -> jump to addr if Rx != 0
#define DEC   0x08   // DEC Rx            -> Rx = Rx - 1
#define LOAD  0x01   // LOAD Rx, addr    -> load memory[addr] into register Rx
#define STORE 0x02   // STORE Rx, addr   -> store register Rx into memory[addr]
#define ADD   0x03   // ADD Rx, Ry, Rz   -> Rx = Ry + Rz
#define SUB   0x04   // SUB Rx, Ry, Rz   -> Rx = Ry - Rz
#define HALT  0xFF   // stop execution

// CPU state
typedef struct {
    int registers[NUM_REGISTERS];  // R0 to R7
    int memory[MEMORY_SIZE];       // RAM
    int pc;                        // Program Counter
    int running;                   // is CPU running?
} CPU;

// Initialize CPU
void init_cpu(CPU* cpu) {
    for (int i = 0; i < NUM_REGISTERS; i++) cpu->registers[i] = 0;
    for (int i = 0; i < MEMORY_SIZE; i++) cpu->memory[i] = 0;
    cpu->pc = 0;
    cpu->running = 1;
    printf("CPU initialized. Registers: %d, Memory: %d bytes\n", NUM_REGISTERS, MEMORY_SIZE);
}

// Fetch-Decode-Execute cycle
void step(CPU* cpu) {
    // FETCH
    int instruction = cpu->memory[cpu->pc];
    int opcode = (instruction >> 24) & 0xFF;  // top byte = opcode
    int rx = (instruction >> 16) & 0xFF;  // second byte = dest register
    int ry = (instruction >> 8) & 0xFF;  // third byte = src register 1
    int rz = instruction & 0xFF;  // bottom byte = src register 2 / address

    printf("PC=%03d | Fetch: 0x%08X | ", cpu->pc, instruction);
    cpu->pc++;

    // DECODE + EXECUTE
    switch (opcode) {
    case LOAD:
        printf("DECODE: LOAD R%d, mem[%d] | EXECUTE: R%d = %d\n",
            rx, rz, rx, cpu->memory[rz]);
        cpu->registers[rx] = cpu->memory[rz];
        break;

    case STORE:
        printf("DECODE: STORE R%d, mem[%d] | EXECUTE: mem[%d] = %d\n",
            rx, rz, rz, cpu->registers[rx]);
        cpu->memory[rz] = cpu->registers[rx];
        break;

    case ADD:
        printf("DECODE: ADD R%d, R%d, R%d | EXECUTE: R%d = %d + %d = %d\n",
            rx, ry, rz, rx,
            cpu->registers[ry], cpu->registers[rz],
            cpu->registers[ry] + cpu->registers[rz]);
        cpu->registers[rx] = cpu->registers[ry] + cpu->registers[rz];
        break;

    case SUB:
        printf("DECODE: SUB R%d, R%d, R%d | EXECUTE: R%d = %d - %d = %d\n",
            rx, ry, rz, rx,
            cpu->registers[ry], cpu->registers[rz],
            cpu->registers[ry] - cpu->registers[rz]);
        cpu->registers[rx] = cpu->registers[ry] - cpu->registers[rz];
        break;

    case HALT:
        printf("DECODE: HALT | EXECUTE: stopping CPU\n");
        cpu->running = 0;
        break;
    case MUL:
        printf("DECODE: MUL R%d, R%d, R%d | EXECUTE: R%d = %d * %d = %d\n",
            rx, ry, rz, rx,
            cpu->registers[ry], cpu->registers[rz],
            cpu->registers[ry] * cpu->registers[rz]);
        cpu->registers[rx] = cpu->registers[ry] * cpu->registers[rz];
        break;

    case MOV:
        printf("DECODE: MOV R%d, %d | EXECUTE: R%d = %d\n", rx, rz, rx, rz);
        cpu->registers[rx] = rz;
        break;

    case JNZ:
        printf("DECODE: JNZ R%d, %d | EXECUTE: R%d=%d, %s\n",
            rx, rz, rx, cpu->registers[rx],
            cpu->registers[rx] != 0 ? "jumping" : "no jump");
        if (cpu->registers[rx] != 0) cpu->pc = rz;
        break;

    case DEC:
        printf("DECODE: DEC R%d | EXECUTE: R%d = %d - 1 = %d\n",
            rx, rx, cpu->registers[rx], cpu->registers[rx] - 1);
        cpu->registers[rx]--;
        break;

    default:
        printf("DECODE: UNKNOWN opcode 0x%02X | EXECUTE: skipping\n", opcode);
        break;
    }
}

// Print register state
void print_registers(CPU* cpu) {
    printf("\n--- Register State ---\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%d = %d\n", i, cpu->registers[i]);
    }
}

int main() {
    CPU cpu;
    init_cpu(&cpu);

    // Compute factorial of 5 (5! = 120)
    // R1 = counter (starts at 5, counts down)
    // R2 = accumulator (starts at 1, gets multiplied)
    // R3 = temp for decrement

    // MOV R1, 5        -> counter = 5
    cpu.memory[0] = (MOV << 24) | (1 << 16) | (0 << 8) | 5;
    // MOV R2, 1        -> accumulator = 1
    cpu.memory[1] = (MOV << 24) | (2 << 16) | (0 << 8) | 1;
    // MUL R2, R2, R1   -> accumulator = accumulator * counter
    cpu.memory[2] = (MUL << 24) | (2 << 16) | (2 << 8) | 1;
    // DEC R1           -> counter--
    cpu.memory[3] = (DEC << 24) | (1 << 16) | (0 << 8) | 0;
    // JNZ R1, 2        -> if counter != 0, jump back to instruction 2
    cpu.memory[4] = (JNZ << 24) | (1 << 16) | (0 << 8) | 2;
    // STORE R2, 50     -> store result in memory[50]
    cpu.memory[5] = (STORE << 24) | (2 << 16) | (0 << 8) | 50;
    // HALT
    cpu.memory[6] = (HALT << 24);

    printf("\n--- Running Program: 5! (factorial of 5) ---\n");
    while (cpu.running) {
        step(&cpu);
    }

    print_registers(&cpu);
    printf("\n5! = memory[50] = %d\n", cpu.memory[50]);

    return 0;
}