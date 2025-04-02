#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#include "vm_dbg.h"

// Configuration and Debug Options
#define NOPS (16)                      // Number of operations
#define DEBUG_MODE 0                   // Set to 1 to enable debug output
#define MEMORY_TRACE 0                 // Set to 1 to enable memory access tracing
#define MEMORY_PROTECTION 1            // Set to 1 to enable memory protection

// Memory-mapped I/O addresses
#define KBSR 0xFE00                    // Keyboard status register
#define KBDR 0xFE02                    // Keyboard data register
#define DSR 0xFE04                     // Display status register
#define DDR 0xFE06                     // Display data register
#define MCR 0xFFFE                     // Machine control register

// Memory protection
#define MEM_PROTECTED_START 0x0000
#define MEM_PROTECTED_END   0x2FFF

// Instruction parsing macros
#define OPC(i) ((i)>>12)               // Extract operation code (bits 15-12)
#define DR(i) (((i)>>9)&0x7)           // Extract destination register (bits 11-9)
#define SR1(i) (((i)>>6)&0x7)          // Extract first source register (bits 8-6)
#define SR2(i) ((i)&0x7)               // Extract second source register (bits 2-0)
#define FIMM(i) ((i>>5)&01)            // Extract immediate mode flag (bit 5)
#define IMM(i) ((i)&0x1F)              // Extract immediate value (bits 4-0)
#define SEXTIMM(i) sext(IMM(i),5)      // Sign-extend immediate value
#define FCND(i) (((i)>>9)&0x7)         // Extract condition flags (bits 11-9)
#define POFF(i) sext((i)&0x3F, 6)      // Extract and sign-extend 6-bit offset
#define POFF9(i) sext((i)&0x1FF, 9)    // Extract and sign-extend 9-bit offset
#define POFF11(i) sext((i)&0x7FF, 11)  // Extract and sign-extend 11-bit offset
#define FL(i) (((i)>>11)&1)            // Extract addressing mode flag (bit 11)
#define BR(i) (((i)>>6)&0x7)           // Extract base register (bits 8-6)
#define TRP(i) ((i)&0xFF)              // Extract trap vector (bits 7-0)

// VM state
bool running = true;
bool debug_mode = DEBUG_MODE;
bool memory_trace = MEMORY_TRACE;

// Function type definitions
typedef void (*op_ex_f)(uint16_t i);
typedef void (*trp_ex_f)();

// Constants and enumerations
enum { trp_offset = 0x20 };            // Trap vector offset
enum regist { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };

// Memory and registers
uint16_t mem[UINT16_MAX] = {0};
uint16_t reg[RCNT] = {0};
uint16_t PC_START = 0x3000;

// Original terminal settings
struct termios original_tio;

// Function to restore terminal settings
void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\nCaught interrupt, exiting...\n");
    exit(-2);
}

// Check if key is pressed
uint16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

// Memory access functions with safety checks
static inline uint16_t mr(uint16_t address) {
    // Handle memory-mapped I/O
    if (address == KBSR) {
        if (check_key()) {
            mem[KBSR] = (1 << 15);
            mem[KBDR] = getchar();
        } else {
            mem[KBSR] = 0;
        }
    }
    
    if (memory_trace) {
        fprintf(stderr, "MEM READ:  [0x%04X] -> 0x%04X\n", address, mem[address]);
    }
    
    return mem[address];
}

static inline void mw(uint16_t address, uint16_t val) {
    if (MEMORY_PROTECTION && address >= MEM_PROTECTED_START && address <= MEM_PROTECTED_END) {
        fprintf(stderr, "Memory protection error: Cannot write to protected address 0x%04X\n", address);
        return;
    }
    
    // Handle memory-mapped I/O
    if (address == DDR) {
        putchar((char)val);
        fflush(stdout);
    } else if (address == MCR) {
        if ((val & (1 << 15)) == 0) {
            running = false;
        }
    } else {
        if (memory_trace) {
            fprintf(stderr, "MEM WRITE: [0x%04X] <- 0x%04X\n", address, val);
        }
        mem[address] = val;
    }
}

// Sign extension function
static inline uint16_t sext(uint16_t n, int b) { 
    return ((n>>(b-1))&1) ? (n|(0xFFFF << b)) : n; 
}

// Update flags based on register value
static inline void uf(enum regist r) {
    if (reg[r]==0) reg[RCND] = FZ;
    else if (reg[r]>>15) reg[RCND] = FN;
    else reg[RCND] = FP;
}

// Instruction implementations
static inline void add(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void and(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void ldi(uint16_t i)  { reg[DR(i)] = mr(mr(reg[RPC]+POFF9(i))); uf(DR(i)); }
static inline void not(uint16_t i)  { reg[DR(i)]=~reg[SR1(i)]; uf(DR(i)); }
static inline void br(uint16_t i)   { if (reg[RCND] & FCND(i)) { reg[RPC] += POFF9(i); } }
static inline void jsr(uint16_t i)  { reg[R7] = reg[RPC]; reg[RPC] = (FL(i)) ? reg[RPC] + POFF11(i) : reg[BR(i)]; }
static inline void jmp(uint16_t i)  { reg[RPC] = reg[BR(i)]; }
static inline void ld(uint16_t i)   { reg[DR(i)] = mr(reg[RPC] + POFF9(i)); uf(DR(i)); }
static inline void ldr(uint16_t i)  { reg[DR(i)] = mr(reg[SR1(i)] + POFF(i)); uf(DR(i)); }
static inline void lea(uint16_t i)  { reg[DR(i)] = reg[RPC] + POFF9(i); uf(DR(i)); }
static inline void st(uint16_t i)   { mw(reg[RPC] + POFF9(i), reg[DR(i)]); }
static inline void sti(uint16_t i)  { mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]); }
static inline void str(uint16_t i)  { mw(reg[SR1(i)] + POFF(i), reg[DR(i)]); }
static inline void rti(uint16_t i) { fprintf(stderr, "RTI instruction not implemented\n"); }
static inline void res(uint16_t i) { fprintf(stderr, "Reserved opcode used\n"); }

// Trap routines
static inline void tgetc() { 
    reg[R0] = getchar(); 
}

static inline void tout() { 
    fprintf(stdout, "%c", (char)reg[R0]); 
    fflush(stdout);
}

static inline void tputs() {
    uint16_t addr = reg[R0];
    // Add bounds checking
    if (addr >= UINT16_MAX) {
        fprintf(stderr, "Error: Invalid memory address in PUTS trap\n");
        return;
    }
    
    uint16_t *p = mem + addr;
    while(*p && p < mem + UINT16_MAX) {
        fprintf(stdout, "%c", (char)*p);
        p++;
    }
    fflush(stdout);
}

static inline void tin() { 
    reg[R0] = getchar(); 
    fprintf(stdout, "%c", (char)reg[R0]); 
    fflush(stdout);
}

static inline void tputsp() {
    uint16_t addr = reg[R0];
    if (addr >= UINT16_MAX) {
        fprintf(stderr, "Error: Invalid memory address in PUTSP trap\n");
        return;
    }
    
    uint16_t *p = mem + addr;
    while (*p && p < mem + UINT16_MAX) {
        char c1 = (*p) & 0xFF;
        fprintf(stdout, "%c", c1);
        
        char c2 = (*p) >> 8;
        if (c2) fprintf(stdout, "%c", c2);
        
        p++;
    }
    fflush(stdout);
}

static inline void thalt() { 
    running = false; 
    printf("\nHALT instruction executed\n");
}

static inline void tinu16() { 
    fscanf(stdin, "%hu", &reg[R0]); 
}

static inline void toutu16() { 
    fprintf(stdout, "%hu\n", reg[R0]); 
    fflush(stdout);
}

// Trap execution table
trp_ex_f trp_ex[8] = { tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16 };

static inline void trap(uint16_t i) { 
    uint8_t trapcode = TRP(i)-trp_offset;
    if (trapcode >= 8) {
        fprintf(stderr, "Invalid trap code: 0x%02X\n", TRP(i));
        return;
    }
    trp_ex[trapcode](); 
}

// Operation execution table
op_ex_f op_ex[NOPS] = { /*0*/ br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap };

// Debug function to print instruction details
void debug_instruction(uint16_t pc, uint16_t instr) {
    const char* op_names[] = {
        "BR", "ADD", "LD", "ST", "JSR", "AND", "LDR", "STR",
        "RTI", "NOT", "LDI", "STI", "JMP", "RES", "LEA", "TRAP"
    };
    
    fprintf(stderr, "PC: 0x%04X, Instr: 0x%04X, Op: %s\n", 
            pc, instr, op_names[OPC(instr)]);
    
    fprintf(stderr, "Registers: ");
    for (int i = 0; i <= R7; i++) {
        fprintf(stderr, "R%d=0x%04X ", i, reg[i]);
    }
    fprintf(stderr, "\n");
}

// Main VM execution loop
void start(uint16_t offset) { 
    reg[RPC] = PC_START + offset;
    
    while(running) {
        uint16_t pc = reg[RPC];
        uint16_t i = mr(reg[RPC]++);
        
        if (debug_mode) {
            debug_instruction(pc, i);
            
            // Optional: Wait for key press to continue in debug mode
            if (getchar() == 'q') {
                printf("Debug mode: quitting\n");
                break;
            }
        }
        
        op_ex[OPC(i)](i);
    }
}

// Load program from file
void ld_img(char *fname, uint16_t offset) {
    FILE *in = fopen(fname, "rb");
    if (NULL==in) {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);    
    }
    
    uint16_t *p = mem + PC_START + offset;
    size_t read = fread(p, sizeof(uint16_t), (UINT16_MAX-PC_START), in);
    
    if (read == 0) {
        fprintf(stderr, "Error: Could not read from file %s\n", fname);
        fclose(in);
        exit(1);
    }
    
    // Endian conversion (LC-3 is big endian)
    #if defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || \
        (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    uint16_t *curr = p;
    while (curr < p + read) {
        *curr = (*curr << 8) | (*curr >> 8);  // Swap bytes
        curr++;
    }
    #endif
    
    fclose(in);
    
    printf("Successfully loaded image file '%s' (%zu words)\n", fname, read);
}

int main(int argc, char **argv) {
    // Handle command line arguments
    bool debug_flag = false;
    bool memory_trace_flag = false;
    char *image_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_flag = true;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory-trace") == 0) {
            memory_trace_flag = true;
        } else if (image_file == NULL) {
            image_file = argv[i];
        }
    }
    
    if (image_file == NULL) {
        fprintf(stderr, "Usage: %s [options] <image-file>\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -d, --debug         Enable debug mode\n");
        fprintf(stderr, "  -m, --memory-trace  Enable memory access tracing\n");
        return 1;
    }
    
    debug_mode = debug_flag;
    memory_trace = memory_trace_flag;
    
    // Set up terminal and signal handlers
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();
    
    // Load and run program
    ld_img(image_file, 0x0);
    
    fprintf(stdout, "Occupied memory after program load:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);
    
    start(0x0); // START PROGRAM
    
    fprintf(stdout, "Occupied memory after program execution:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);
    
    fprintf(stdout, "Registers after program execution:\n");
    fprintf_reg_all(stdout, reg, RCNT);
    
    // Restore terminal settings
    restore_input_buffering();
    
    return 0;
}