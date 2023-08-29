#include "vm.h"
#include "instructions.h"
#include "interrupts.h"


//00
uint8_t vm_instruction_hlt (struct VM *vm) {
    vm->flags[F_HALT] = 1;
    return 0;
}

//01
uint8_t vm_instruction_nop (struct VM *vm) {
    return 0;
}

//02
uint8_t vm_instruction_stdout (struct VM *vm) {
    char ch = LBYTE(vm->reg[0]);
    #ifdef DEBUG
        printf("stdout: 0x%02x '%c'\n", ch, (ch < 32 ? '.' : (ch > 127 ? '.' : ch)));
    #endif
    #ifndef DEBUG
        putchar(ch);
        fflush(stdout);
    #endif
    return 0;
}

//03
uint8_t vm_instruction_stdin (struct VM *vm) {
    uint8_t ch;
    fcntl(0, F_SETFL, O_NONBLOCK);
    int byte_read = read(0, &ch, 1);
    fcntl(0, F_SETFL, ~O_NONBLOCK);
    if (byte_read > 0) {
        vm->op_dst = ch;
        vm->flags[F_ZERO] = 0;
        vm->flags[F_DATA] = 1;
    } else {
        vm->flags[F_ZERO] = 1;
        vm->flags[F_DATA] = 0;
    }
    return 0;
}

//04
uint8_t vm_instruction_int (struct VM *vm) {
    switch (vm->op_src) {
        case I_INFO:
            return vm_int_info(vm);
            break;
        case I_GPIO_CFG: //pin, direction, pullup/pulldown
            return vm_int_gpio_cfg(vm);
            break;
        case I_GPIO_SET: //pin, level
            return vm_int_gpio_set(vm);
            break;
        case I_GPIO_GET: //pin
            return vm_int_gpio_get(vm);
            break;
        case I_VIDEO_PUTPIXEL: //x, y, colour
            return vm_int_video_putpixel(vm);
            break;
        case I_VIDEO_GETPIXEL: //x, y
            return vm_int_video_getpixel(vm);
            break;
        case I_VIDEO_FILL: //x, y, w, h, colour
            return vm_int_video_fill(vm);
            break;
        case I_VIDEO_LINE: //sx, sy, dx, dy, colour
            return vm_int_video_line(vm);
            break;
        case I_VIDEO_CIRCLE: //cx, cy, radius, colour
            return vm_int_video_circle(vm);
            break;
        case I_VIDEO_PRINT: //x, y, char, colour
            return vm_int_video_print(vm);
            break;
    }
    return 0;
}

//05..06
uint8_t vm_instruction_call (struct VM *vm) {
    return 0;
}

//07..08
uint8_t vm_instruction_cmp (struct VM *vm) {
    uint16_t val = vm->op_src;
    vm->flags[F_GREATER] = vm->reg[0] > val ? 1 : 0;
    vm->flags[F_LESS] = vm->reg[0] < val ? 1 : 0;
    vm->flags[F_EQUAL] = val == vm->reg[0];
    vm->flags[F_ZERO] = vm->reg[0] == 0 ? 1 : 0;
    return 0;
}

//10..15
uint8_t vm_instruction_mov (struct VM *vm) {
    vm->op_dst = vm->op_src;
    return 0;
}

//20
uint8_t vm_instruction_inc (struct VM *vm) {
    vm->flags[F_OVERFLOW] = vm->op_src == 0xffff;
    vm->op_dst = vm->op_src + 1;
    return 0;
}

//21
uint8_t vm_instruction_dec (struct VM *vm) {
    vm->flags[F_UNDERFLOW] = vm->op_src == 0x0000;
    vm->op_dst = vm->op_src - 1;
    return 0;
}

//22
uint8_t vm_instruction_add (struct VM *vm) {
    vm->flags[F_OVERFLOW] = (uint32_t)(vm->reg[0] + vm->op_src) > 0xffff;
    vm->op_dst = vm->reg[0] + vm->op_src;
    return 0;
}

//23
uint8_t vm_instruction_sub (struct VM *vm) {
    vm->flags[F_UNDERFLOW] = (int32_t)(vm->reg[0] - vm->op_src) < 0x0000;
    vm->op_dst = vm->reg[0] - vm->op_src;
    return 0;
}

//24
uint8_t vm_instruction_mul (struct VM *vm) {
    vm->flags[F_OVERFLOW] = (uint32_t)(vm->reg[0] * vm->op_src) > 0xffff;
    vm->op_dst = vm->reg[0] * vm->op_src;
    return 0;
}

//25
uint8_t vm_instruction_div (struct VM *vm) {
    vm->op_dst = vm->reg[0] / vm->op_src;
    return 0;
}

//26
uint8_t vm_instruction_shl (struct VM *vm) {
    vm->op_dst = vm->op_src << 1;
    return 0;
}

//27
uint8_t vm_instruction_shr (struct VM *vm) {
    vm->op_dst = vm->op_src >> 1;
    return 0;
}

//e0..e1
uint8_t vm_instruction_jmp (struct VM *vm) {
    vm->pc = vm->op_src;
    return 0;
}

//e2..e3
uint8_t vm_instruction_je (struct VM *vm) {
    if (vm->flags[F_EQUAL] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//e4..e5
uint8_t vm_instruction_jne (struct VM *vm) {
    if (vm->flags[F_EQUAL] == 1) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//e6..e7
uint8_t vm_instruction_jz (struct VM *vm) {
    if (vm->flags[F_ZERO] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//e8..e9
uint8_t vm_instruction_jnz (struct VM *vm) {
    if (vm->flags[F_ZERO] == 1) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//ea..eb
uint8_t vm_instruction_jl (struct VM *vm) {
    if (vm->flags[F_LESS] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//ec..ed
uint8_t vm_instruction_jle (struct VM *vm) {
    if (vm->flags[F_LESS] == 0 && vm->flags[F_EQUAL] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//ee..ef
uint8_t vm_instruction_jg (struct VM *vm) {
    if (vm->flags[F_GREATER] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}

//f0..f1
uint8_t vm_instruction_jge (struct VM *vm) {
    if (vm->flags[F_GREATER] == 0 && vm->flags[F_EQUAL] == 0) return 0;        
    vm->pc = vm->op_src;
    return 1;
}


