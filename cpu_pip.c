#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#define MEMORY_SIZE 256
#define NUM_REGISTERS 8

#define NOP   0x00
#define LOAD  0x01
#define STORE 0x02
#define ADD   0x03
#define SUB   0x04
#define MUL   0x05
#define MOV   0x06
#define HALT  0xFF

// One instruction moving through the pipeline
typedef struct {
    int instruction;   // raw encoded instruction
    int opcode;
    int rx, ry, rz;
    int result;        // computed during execute
    int valid;         // is this stage occupied?
    int is_halt;
} PipelineStage;

// CPU with 3-stage pipeline
typedef struct {
    int registers[NUM_REGISTERS];
    int memory[MEMORY_SIZE];
    int pc;
    int running;
    int cycle;
    int stall;         // stall counter

    // Pipeline registers (between stages)
    PipelineStage fetch_reg;    // holds fetched instruction
    PipelineStage decode_reg;   // holds decoded instruction
    PipelineStage execute_reg;  // holds executed instruction
} PipelinedCPU;

void init_cpu(PipelinedCPU* cpu) {
    memset(cpu, 0, sizeof(PipelinedCPU));
    cpu->running = 1;
    printf("Pipelined CPU initialized.\n");
    printf("%-8s %-20s %-20s %-20s %-10s\n",
        "Cycle", "FETCH", "DECODE", "EXECUTE", "Stall?");
    printf("--------------------------------------------------------------------\n");
}

// Check for data hazard between incoming instruction and stages in flight
int check_hazard(PipelinedCPU* cpu, int rx, int ry, int rz) {
    // Check if any in-flight instruction writes to a register we need to read
    int hazard = 0;

    if (cpu->decode_reg.valid && cpu->decode_reg.rx != 0) {
        if (cpu->decode_reg.rx == ry || cpu->decode_reg.rx == rz)
            hazard = 1;
    }
    if (cpu->execute_reg.valid && cpu->execute_reg.rx != 0) {
        if (cpu->execute_reg.rx == ry || cpu->execute_reg.rx == rz)
            hazard = 1;
    }
    return hazard;
}

void print_stage(PipelineStage* s) {
    if (!s->valid) { printf("%-20s", "---"); return; }
    char buf[32];
    switch (s->opcode) {
    case LOAD:  sprintf(buf, "LOAD R%d,m[%d]", s->rx, s->rz); break;
    case STORE: sprintf(buf, "STORE R%d,m[%d]", s->rx, s->rz); break;
    case ADD:   sprintf(buf, "ADD R%d,R%d,R%d", s->rx, s->ry, s->rz); break;
    case SUB:   sprintf(buf, "SUB R%d,R%d,R%d", s->rx, s->ry, s->rz); break;
    case MUL:   sprintf(buf, "MUL R%d,R%d,R%d", s->rx, s->ry, s->rz); break;
    case MOV:   sprintf(buf, "MOV R%d,%d", s->rx, s->rz); break;
    case HALT:  sprintf(buf, "HALT"); break;
    case NOP:   sprintf(buf, "NOP(stall)"); break;
    default:    sprintf(buf, "???"); break;
    }
    printf("%-20s", buf);
}

void tick(PipelinedCPU* cpu) {
    cpu->cycle++;

    // ---- EXECUTE stage ----
    PipelineStage ex = cpu->decode_reg;
    if (ex.valid) {
        switch (ex.opcode) {
        case ADD:
            ex.result = cpu->registers[ex.ry] + cpu->registers[ex.rz];
            cpu->registers[ex.rx] = ex.result;
            break;
        case SUB:
            ex.result = cpu->registers[ex.ry] - cpu->registers[ex.rz];
            cpu->registers[ex.rx] = ex.result;
            break;
        case MUL:
            ex.result = cpu->registers[ex.ry] * cpu->registers[ex.rz];
            cpu->registers[ex.rx] = ex.result;
            break;
        case MOV:
            cpu->registers[ex.rx] = ex.rz;
            break;
        case LOAD:
            cpu->registers[ex.rx] = cpu->memory[ex.rz];
            break;
        case STORE:
            cpu->memory[ex.rz] = cpu->registers[ex.rx];
            break;
        case HALT:
            cpu->running = 0;
            break;
        }
    }
    cpu->execute_reg = ex;

    // ---- DECODE stage ----
    PipelineStage dec = cpu->fetch_reg;
    if (dec.valid) {
        dec.opcode = (dec.instruction >> 24) & 0xFF;
        dec.rx = (dec.instruction >> 16) & 0xFF;
        dec.ry = (dec.instruction >> 8) & 0xFF;
        dec.rz = dec.instruction & 0xFF;

        // hazard detection
        if (check_hazard(cpu, dec.rx, dec.ry, dec.rz)) {
            // insert a NOP bubble — stall the pipeline
            PipelineStage bubble = { 0 };
            bubble.valid = 1;
            bubble.opcode = NOP;
            cpu->decode_reg = bubble;
            cpu->stall++;

            // print cycle with stall
            printf("%-8d ", cpu->cycle);
            print_stage(&cpu->fetch_reg);
            printf("%-20s", "** STALL **");
            print_stage(&cpu->execute_reg);
            printf("YES\n");
            return;
        }
    }
    cpu->decode_reg = dec;

    // ---- FETCH stage ----
    PipelineStage fet = { 0 };
    if (cpu->running && cpu->pc < MEMORY_SIZE) {
        fet.instruction = cpu->memory[cpu->pc];
        fet.valid = 1;
        cpu->pc++;
    }

    // print cycle
    printf("%-8d ", cpu->cycle);
    print_stage(&fet);
    print_stage(&cpu->decode_reg);
    print_stage(&cpu->execute_reg);
    printf("NO\n");

    cpu->fetch_reg = fet;
}

void print_registers(PipelinedCPU* cpu) {
    printf("\n--- Register State ---\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        if (cpu->registers[i] != 0)
            printf("R%d = %d\n", i, cpu->registers[i]);
}

int main() {
    PipelinedCPU cpu;
    init_cpu(&cpu);

    // Program: R1=10, R2=20, R3=R1+R2, R4=R3*2
    // deliberately has a data hazard: R3 used right after being written
    cpu.memory[0] = (MOV << 24) | (1 << 16) | (0 << 8) | 10;  // MOV R1, 10
    cpu.memory[1] = (MOV << 24) | (2 << 16) | (0 << 8) | 20;  // MOV R2, 20
    cpu.memory[2] = (ADD << 24) | (3 << 16) | (1 << 8) | 2;   // ADD R3, R1, R2
    cpu.memory[3] = (MOV << 24) | (4 << 16) | (0 << 8) | 2;   // MOV R4, 2
    cpu.memory[4] = (MUL << 24) | (5 << 16) | (3 << 8) | 4;   // MUL R5, R3, R4  <- hazard on R3
    cpu.memory[5] = (STORE << 24) | (5 << 16) | (0 << 8) | 100; // STORE R5, 100
    cpu.memory[6] = (HALT << 24);

    while (cpu.running || cpu.decode_reg.valid || cpu.fetch_reg.valid) {
        tick(&cpu);
        if (cpu.cycle > 20) break; // safety
    }

    print_registers(&cpu);
    printf("\nResult at memory[100] = %d\n", cpu.memory[100]);
    printf("Total cycles: %d | Stalls: %d\n", cpu.cycle, cpu.stall);

    return 0;
}