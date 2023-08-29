#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "vm.h"

//Interrupts
#define I_INFO           0 //Display VM info to stdout (version, credits, etc)
#define I_GPIO_CFG       1 //Configure Pico GPIO pin [pin, direction, pullup_pulldown]
#define I_GPIO_SET       2 //Set Pico GPIO pin high/low logic level [pin, level]
#define I_GPIO_GET       3 //Get Pico GPIO pin logic level [pin]
#define I_VIDEO_PUTPIXEL 4 //Set a pixel colour on the video display [x, y, colour]
#define I_VIDEO_GETPIXEL 5 //Get a pixel colour from the video display [x, y]
#define I_VIDEO_FILL     6 //Fill a rectangle on the video display [x, y, width, height, colour]
#define I_VIDEO_LINE     7 //Draw a line on the video display [sx, sy, dx, dy, colour]
#define I_VIDEO_CIRCLE   8 //Draw a circle on the video display [cx, cy, radius, colour]
#define I_VIDEO_PRINT    9 //Print a character to the video display [x, y, char, colour]
#define I_VIDEO_UPDATE   0x0a

uint8_t vm_int_info (struct VM *vm);
uint8_t vm_int_gpio_cfg (struct VM *vm);
uint8_t vm_int_gpio_get (struct VM *vm);
uint8_t vm_int_gpio_set (struct VM *vm);
uint8_t vm_int_video_putpixel (struct VM *vm);
uint8_t vm_int_video_getpixel (struct VM *vm);
uint8_t vm_int_video_fill (struct VM *vm);
uint8_t vm_int_video_line (struct VM *vm);
uint8_t vm_int_video_circle (struct VM *vm);
uint8_t vm_int_video_print (struct VM *vm);
uint8_t vm_int_video_update (struct VM *vm);

#endif
