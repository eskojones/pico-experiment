
#include "vm.h"
#include "instructions.h"


uint8_t vm_get_opcode_from_string (struct VM *vm, char *opcode, char smode, char dmode) {
    for (uint8_t i = 0; i <= 255; i++) {
        if (strcmp(vm->opcodes[i].name, opcode) == 0 
         && vm->opcodes[i].smode == smode
         && vm->opcodes[i].dmode == dmode) {
            return i;
        }
    }
    #ifdef DEBUG
        printf("vm_get_opcode_from_string(%s): not found\n", opcode);
    #endif
    vm->flags[F_HALT] = 1;
    return 0x00;
}


void vm_load_instruction (struct VM *vm, uint8_t opcode, char *name, char smode, char dmode, uint8_t (*fn)(struct VM *)) {
    VM_Op op = {
        opcode,
        "",
        smode, dmode,
        fn
    };
    snprintf(op.name, 20, "%s", name);
    vm->opcodes[opcode] = op;
}


void vm_init (struct VM *vm) {
    #ifdef DEBUG
        printf("[+] initialising vm...\n");
    #endif
    memset(vm, 0, sizeof(struct VM));

    vm_load_instruction(vm, 0x00, "hlt",    ' ', ' ', vm_instruction_hlt);
    vm_load_instruction(vm, 0x01, "nop",    ' ', ' ', vm_instruction_nop);
    vm_load_instruction(vm, 0x02, "stdout", ' ', ' ', vm_instruction_stdout);
    vm_load_instruction(vm, 0x03, "stdin",  ' ', 'r', vm_instruction_stdin);
    vm_load_instruction(vm, 0x04, "int",    'i', ' ', vm_instruction_int);
    vm_load_instruction(vm, 0x05, "call",   'i', 'i', vm_instruction_call);
    vm_load_instruction(vm, 0x06, "call",   'r', 'r', vm_instruction_call);
    vm_load_instruction(vm, 0x07, "cmp",    'i', ' ', vm_instruction_cmp);
    vm_load_instruction(vm, 0x08, "cmp",    'r', ' ', vm_instruction_cmp);
    
    vm_load_instruction(vm, 0x10, "mov",    'i', 'r', vm_instruction_mov);
    vm_load_instruction(vm, 0x11, "mov",    'm', 'r', vm_instruction_mov);
    vm_load_instruction(vm, 0x12, "mov",    'p', 'r', vm_instruction_mov);
    vm_load_instruction(vm, 0x13, "mov",    'r', 'm', vm_instruction_mov);
    vm_load_instruction(vm, 0x14, "mov",    'r', 'p', vm_instruction_mov);
    vm_load_instruction(vm, 0x15, "mov",    'r', 'r', vm_instruction_mov);
    

    vm_load_instruction(vm, 0x20, "inc",    'r', ' ', vm_instruction_inc);
    vm_load_instruction(vm, 0x21, "dec",    'r', ' ', vm_instruction_dec);
    vm_load_instruction(vm, 0x22, "add",    'r', ' ', vm_instruction_add);
    vm_load_instruction(vm, 0x23, "sub",    'r', ' ', vm_instruction_sub);
    vm_load_instruction(vm, 0x24, "mul",    'r', ' ', vm_instruction_mul);
    vm_load_instruction(vm, 0x25, "div",    'r', ' ', vm_instruction_div);
    vm_load_instruction(vm, 0x26, "shl",    'r', ' ', vm_instruction_shl);
    vm_load_instruction(vm, 0x27, "shr",    'r', ' ', vm_instruction_shr);

    vm_load_instruction(vm, 0xe0, "jmp",    'i', ' ', vm_instruction_jmp);
    vm_load_instruction(vm, 0xe1, "jmp",    'r', ' ', vm_instruction_jmp);
    vm_load_instruction(vm, 0xe2, "je",     'i', ' ', vm_instruction_je);
    vm_load_instruction(vm, 0xe3, "je",     'r', ' ', vm_instruction_je);
    vm_load_instruction(vm, 0xe4, "jne",    'i', ' ', vm_instruction_jne);
    vm_load_instruction(vm, 0xe5, "jne",    'r', ' ', vm_instruction_jne);
    vm_load_instruction(vm, 0xe6, "jz",     'i', ' ', vm_instruction_jz);
    vm_load_instruction(vm, 0xe7, "jz",     'r', ' ', vm_instruction_jz);
    vm_load_instruction(vm, 0xe8, "jnz",    'i', ' ', vm_instruction_jnz);
    vm_load_instruction(vm, 0xe9, "jnz",    'r', ' ', vm_instruction_jnz);
    vm_load_instruction(vm, 0xea, "jl",     'i', ' ', vm_instruction_jl);
    vm_load_instruction(vm, 0xeb, "jl",     'r', ' ', vm_instruction_jl);
    vm_load_instruction(vm, 0xec, "jle",    'i', ' ', vm_instruction_jle);
    vm_load_instruction(vm, 0xed, "jle",    'r', ' ', vm_instruction_jle);
    vm_load_instruction(vm, 0xee, "jg",     'i', ' ', vm_instruction_jg);
    vm_load_instruction(vm, 0xef, "jg",     'r', ' ', vm_instruction_jg);
    vm_load_instruction(vm, 0xf0, "jge",    'i', ' ', vm_instruction_jge);
    vm_load_instruction(vm, 0xf1, "jge",    'r', ' ', vm_instruction_jge);
}


void vm_load (struct VM *vm, char *program, uint16_t length, uint16_t address) {
    #ifdef DEBUG
        printf("[+] loading program (%d bytes) at 0x%04x...", length, address);
        fflush(stdout);
    #endif
    for (uint16_t i = 0; i < length; i++) {
        vm->mem[address + i] = program[i];
    }
    #ifdef DEBUG
        printf("done.\n");
    #endif
    vm->pc = address;
}


//called before execution of the instruction
void vm_fetch (struct VM *vm) {
    uint16_t addr, ptr_addr, size = 1;
    vm->opcode = vm->mem[vm->pc];
    
    switch (vm->opcodes[vm->opcode].dmode) {
        case 'r':
            size += 1;
            break;
        case 'm':
            size += 2;
            break;
        case 'p':
            size += 2;
            break;
    }
    
    //fetch src data for the instruction
    switch (vm->opcodes[vm->opcode].smode) {
        case 'r': //src is a register index
            vm->op_src = vm->reg[vm->mem[vm->pc + size + 0]];
            size += 1;
            break;
        case 'i': //src is an immediate value
            vm->op_src = SHORT(vm->mem[vm->pc + size + 0], vm->mem[vm->pc + size + 1]);
            size += 2;
            break;
        case 'm': //src is a memory address
            addr = SHORT(vm->mem[vm->pc + size], vm->mem[vm->pc + size + 1]);
            vm->op_src = SHORT(vm->mem[addr], vm->mem[addr + 1]);
            size += 2;
            break;
        case 'p': //src is a pointer to a memory address
            ptr_addr = SHORT(vm->mem[vm->pc + size + 0], vm->mem[vm->pc + size + 1]);
            addr = SHORT(vm->mem[ptr_addr], vm->mem[ptr_addr + 1]);
            vm->op_src = SHORT(vm->mem[addr], vm->mem[addr + 1]);
            size += 2;
            break;
        default: //src is R0
            vm->op_src = vm->reg[0];
    }

    //bytes to increment PC after operation
    vm->op_size = size;
}


//called after execution of the instruction
void vm_result (struct VM *vm) {
    uint16_t ptr_addr, addr;
    switch (vm->opcodes[vm->opcode].dmode) {
        case 'r':
            vm->reg[vm->mem[vm->pc + 1]] = vm->op_dst;
            break;
        case 'm':
            addr = SHORT(vm->mem[vm->pc + 1], vm->mem[vm->pc + 2]);
            vm->mem[addr] = HBYTE(vm->op_dst);
            vm->mem[addr + 1] = LBYTE(vm->op_dst);
            break;
        case 'p':
            ptr_addr = SHORT(vm->mem[vm->pc + 1], vm->mem[vm->pc + 2]);
            addr = SHORT(vm->mem[ptr_addr], vm->mem[ptr_addr + 1]);
            vm->mem[addr] = HBYTE(vm->op_dst);
            vm->mem[addr + 1] = LBYTE(vm->op_dst);
            break;
        default:
            vm->reg[0] = vm->op_dst;
    }
}


char *vm_debug_opcode2name (struct VM *vm, uint8_t op) {
    return vm->opcodes[op].name;
}

void vm_debug_op (struct VM *vm) {
    char dmode = vm->opcodes[vm->opcode].dmode;
    char smode = vm->opcodes[vm->opcode].smode;
    if (dmode == ' ') dmode = '.';
    if (smode == ' ') smode = '.';
    printf("0x%04x %s (%c%c) ", 
           vm->pc, 
           vm_debug_opcode2name(vm, vm->opcode), 
           dmode, smode
    );
    for (int i = 1; i < vm->op_size; i++) {
        printf("%02x ", vm->mem[vm->pc + i]);
    }
    printf("\n");
}

void vm_debug_mem (struct VM *vm, uint16_t addr, uint16_t len) {
    char fmt[256];
    snprintf(fmt, 256, "0x%04x ", vm->pc);
    for (uint16_t i = addr; i < addr + len; i++) {
        snprintf(fmt, 256, "%s%02x ", fmt, vm->mem[i]);
    }
    printf("[MEM] %s\n", fmt);
}


void vm_debug_reg (struct VM *vm, uint8_t start, uint8_t count) {
    printf("[REG] ");
    for (uint8_t c = 0, i = start; i < start + count; i++) {
        printf("R%d:%04x ", i, vm->reg[i]);
    }
    printf("\n");
}


void vm_debug_flags (struct VM *vm) {
    printf("[FLAGS] ");
    for (uint8_t i = 0; i < FLAG_COUNT; i++) {
        printf("%s:%d ", FLAG_NAMES[i], vm->flags[i]);
    }
    printf("\n");
}


void vm_step (struct VM *vm) {
    if (vm->flags[F_HALT] != 0) return;
    vm_fetch(vm);
    #ifdef DEBUG_OP
        vm_debug_op(vm);
    #endif
    if (vm->opcodes[vm->opcode].func(vm) == 0) {
        vm_result(vm);
        vm->pc += vm->op_size;
    }
    #ifdef DEBUG_REG
        vm_debug_reg(vm, 0, 10);
    #endif
    #ifdef DEBUG_FLAGS
        vm_debug_flags(vm);
    #endif
}


