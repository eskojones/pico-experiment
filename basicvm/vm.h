#ifndef _VM_H_
#define _VM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <fcntl.h>

#ifdef PICO_LCD_BASE
#include "lcd.h"
#include "surface.h"
#include "font.h"
#endif

#define DEBUG
//#define DEBUG_REG
//#define DEBUG_FLAGS
#define DEBUG_OP

#define SHORT(h,l) ((h & 0x00ff) << 8) + (l & 0x00ff)
#define HBYTE(i) (i >> 8) & 0xff
#define LBYTE(i) i & 0xff
#define OPCODE(vm, opname, smode, dmode) vm_get_opcode_from_string(vm, opname, smode, dmode)


#define F_HALT    0
#define F_GREATER 1
#define F_LESS    2
#define F_EQUAL   3
#define F_ZERO    4
#define F_DATA    5
#define F_OVERFLOW 6
#define F_UNDERFLOW 7
#define FLAG_COUNT 8

static const char FLAG_NAMES[][4] = {
    "HLT", " GT", " LT", " EQ", "ZRO", "DAT", "OVR", "UND"
};




struct VM;

typedef struct {
    uint8_t opcode;
    char name[20];
    char smode, dmode;
    uint8_t (*func)(struct VM *);
} VM_Op;


struct VM {
    uint16_t pc;            //Program Counter
    uint16_t reg[10];       //Registers
    uint8_t mem[65536];     //Memory for Program and Data
    #ifdef PICO_LCD_BASE
    Surface *video;
    Font *font;
    #endif
    uint8_t flags[10];      //Status Flags
    VM_Op opcodes[256];     //OpCodes available
    uint8_t opcount;        //Count of OpCodes loaded
    uint8_t opcode;
    uint16_t op_src, op_dst;
    uint8_t op_size;
};


uint8_t vm_get_opcode_from_string (struct VM *vm, char *opcode, char smode, char dmode);
void vm_load_instruction (struct VM *vm, uint8_t opcode, char *name, char smode, char dmode, uint8_t (*fn)(struct VM *));
void vm_init (struct VM *vm);
void vm_load (struct VM *vm, char *program, uint16_t length, uint16_t address);
void vm_fetch (struct VM *vm);
void vm_debug_mem (struct VM *vm, uint16_t addr, uint16_t len);
void vm_debug_reg (struct VM *vm, uint8_t start, uint8_t count);
void vm_debug_flags (struct VM *vm);
void vm_step (struct VM *vm);


#endif
