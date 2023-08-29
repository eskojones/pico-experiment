#ifndef _INSTRUCTIONS_H_
#define _INSTRUCTIONS_H_

#include "vm.h"

uint8_t vm_instruction_hlt (struct VM *vm);
uint8_t vm_instruction_nop (struct VM *vm);
uint8_t vm_instruction_stdout (struct VM *vm);
uint8_t vm_instruction_stdin (struct VM *vm);
uint8_t vm_instruction_int (struct VM *vm);
uint8_t vm_instruction_call (struct VM *vm);
uint8_t vm_instruction_cmp (struct VM *vm);

uint8_t vm_instruction_mov (struct VM *vm);

uint8_t vm_instruction_inc (struct VM *vm);
uint8_t vm_instruction_dec (struct VM *vm);
uint8_t vm_instruction_add (struct VM *vm);
uint8_t vm_instruction_sub (struct VM *vm);
uint8_t vm_instruction_mul (struct VM *vm);
uint8_t vm_instruction_div (struct VM *vm);
uint8_t vm_instruction_shl (struct VM *vm);
uint8_t vm_instruction_shr (struct VM *vm);

uint8_t vm_instruction_jmp (struct VM *vm);
uint8_t vm_instruction_je (struct VM *vm);
uint8_t vm_instruction_jne (struct VM *vm);
uint8_t vm_instruction_jz (struct VM *vm);
uint8_t vm_instruction_jnz (struct VM *vm);
uint8_t vm_instruction_jl (struct VM *vm);
uint8_t vm_instruction_jle (struct VM *vm);
uint8_t vm_instruction_jg (struct VM *vm);
uint8_t vm_instruction_jge (struct VM *vm);

#endif
