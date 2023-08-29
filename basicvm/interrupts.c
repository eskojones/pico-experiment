#include <stdio.h>
#include "interrupts.h"


uint8_t vm_int_info (struct VM *vm) {
    printf("DCM64 https://github.com/eskojones/basicvm\n");
    return 0;
}

uint8_t vm_int_gpio_cfg (struct VM *vm) {
    uint8_t pin = LBYTE(vm->reg[1]);
    uint8_t dir = LBYTE(vm->reg[2]);
    uint8_t pullup = LBYTE(vm->reg[3]);
    printf("INT_GPIO_CFG: NYI (pin:%d, dir:%d, pullup:%d)\n", pin, dir, pullup);
    #ifdef PICO_LCD_BASE
        //GPIO stuff...
    #endif
    return 0;
}

uint8_t vm_int_gpio_set (struct VM *vm) {
    uint8_t pin = LBYTE(vm->reg[1]);
    uint8_t level = LBYTE(vm->reg[2]);
    printf("INT_GPIO_SET: NYI (pin:%d, level:%d)\n", pin, level);
    #ifdef PICO_LCD_BASE
        //GPIO stuff...
    #endif
    return 0;
}

uint8_t vm_int_gpio_get (struct VM *vm) {
    uint8_t pin = LBYTE(vm->reg[1]);
    printf("INT_GPIO_GET: NYI (pin:%d)\n", pin);
    #ifdef PICO_LCD_BASE
        //GPIO stuff...
    #endif
    return 0;
}

uint8_t vm_int_video_putpixel (struct VM *vm) {
    uint16_t x = vm->reg[1];
    uint16_t y = vm->reg[2];
    uint16_t colour = vm->reg[3];
    printf("INT_VIDEO_PUTPIXEL: (x:%d, y:%d, colour:0x%04x)\n", x, y, colour);
    #ifdef PICO_LCD_BASE
        surface_putpixel(vm->video, x, y, colour);
    #endif
    return 0;
}

uint8_t vm_int_video_getpixel (struct VM *vm) {
    uint16_t x = vm->reg[1];
    uint16_t y = vm->reg[2];
    printf("INT_VIDEO_GETPIXEL: (x:%d, y:%d)\n", x, y);
    #ifdef PICO_LCD_BASE
        vm->reg[0] = surface_getpixel(vm->video, x, y);
    #endif
    return 0;
}

uint8_t vm_int_video_fill (struct VM *vm) {
    uint16_t x = vm->reg[1];
    uint16_t y = vm->reg[2];
    uint16_t width = vm->reg[3];
    uint16_t height = vm->reg[4];
    uint16_t colour = vm->reg[5];
    printf("INT_VIDEO_FILL: (x:%d, y:%d, w:%d, h:%d, colour:0x%04x)\n", 
        x, y, width, height, colour
    );
    #ifdef PICO_LCD_BASE
        Surface *fill = surface_create(4, 4);
        surface_fill(fill, colour);
        Rect src = { 0, 0, 4, 4 };
        Rect dst = { x, y, width, height };
        surface_scaleblit(vm->video, fill, &dst, &src);
    #endif
    return 0;
}

uint8_t vm_int_video_line (struct VM *vm) {
    uint16_t sx = vm->reg[1];
    uint16_t sy = vm->reg[2];
    uint16_t dx = vm->reg[3];
    uint16_t dy = vm->reg[4];
    uint16_t colour = vm->reg[5];
    printf("INT_VIDEO_LINE: (sx:%d, sy:%d, dx:%d, dy:%d, colour:0x%04x)\n", 
        sx, sy, dx, dy, colour
    );
    #ifdef PICO_LCD_BASE
        surface_line(vm->video, sx, sy, dx, dy, colour);
    #endif
    return 0;
}

uint8_t vm_int_video_circle (struct VM *vm) {
    uint16_t cx = vm->reg[1];
    uint16_t cy = vm->reg[2];
    uint16_t radius = vm->reg[3];
    uint16_t colour = vm->reg[4];
    printf("INT_VIDEO_CIRCLE: NYI (cx:%d, cy:%d, radius:%d, colour:0x%04x)\n", 
        cx, cy, radius, colour
    );
    #ifdef PICO_LCD_BASE
        //soon(tm)
    #endif
    return 0;
}

uint8_t vm_int_video_print (struct VM *vm) {
    uint16_t x = vm->reg[1];
    uint16_t y = vm->reg[2];
    uint8_t ch = LBYTE(vm->reg[3]);
    uint16_t colour = vm->reg[4];
    printf("INT_VIDEO_PRINT: (x:%d, y:%d, ch:'%c', colour:0x%04x)\n", 
        x, y, ch, colour
    );
    #ifdef PICO_LCD_BASE
        char text[] = { ch, 0 };
        font_print(vm->video, vm->font, text, x, y, colour);
    #endif
    return 0;
}

uint8_t vm_int_video_update (struct VM *vm) {
    printf("INT_VIDEO_UPDATE\n");
    #ifdef PICO_LCD_BASE
        lcd_draw_surface(vm->video);
    #endif
    return 0;
}
