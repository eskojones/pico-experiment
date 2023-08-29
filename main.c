#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "stdbool.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/float.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "font.h"
#include "lcd.h"
#include "sprite.h"
#include "surface.h"
#include "types.h"

#include "vm.h"
#include "interrupts.h"

#define KEY_CR          0x0D
#define KEY_BACKSPACE   0x08
#define KEY_ESCAPE      0x1B
#define KEY_SPACE       0x20
#define KEY_TILDE       0x7E


Font font_small = {
    font_5x5,
    5, 5, 1,
    32, 126
};


float adc_read_temp () {
    adc_select_input(4);
    return 27.0f - ((float)adc_read() * (3.3f / (1 << 12)) - 0.706f) / 0.001721f;
}


float adc_read_vsys () {
    adc_gpio_init(PICO_VSYS_PIN);
    adc_select_input(PICO_VSYS_PIN - 26);
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);
    uint32_t vsys = 0;
    int ignore_count = 10;
    while (!adc_fifo_is_empty() || ignore_count-- > 0) adc_fifo_get_blocking();
    for(int i = 0; i < 10; i++) vsys += adc_fifo_get_blocking();
    adc_run(false);
    adc_fifo_drain();
    return (vsys / 10) * 3 * (3.3f / (1 << 12));
}


typedef struct {
    char input[256], *output, cursor[2];
    char escape_command[256];
    uint16_t *colours;
    bool input_finished;
    uint8_t width, height, cursor_x, cursor_y;
    uint8_t font_width, font_height;
    uint16_t fg_colour, bg_colour, cursor_colour;
} TermInfo;


void term_clear (TermInfo *term) {
    term->cursor_x = 0;
    term->cursor_y = 0;
    memset(term->output, 0, term->width * term->height);
    for (int i = 0, l = term->width * term->height; i < l; i++) term->colours[i] = term->fg_colour;
    //memset(term->colours, term->fg_colour, term->width * term->height);
}


TermInfo *term_create(uint8_t width, uint8_t height) {
    TermInfo *term = (TermInfo *)malloc(sizeof(TermInfo));
    term->output = (char *)malloc(width * height);
    term->colours = (uint16_t *)malloc(width * height);
    memset(term->input, 0, sizeof(term->input));
    memset(term->escape_command, 0, sizeof(term->escape_command));
    term->input_finished = false;
    term->cursor[0] = '_';
    term->cursor[1] = 0;
    term->width = width;
    term->height = height;
    term->font_width = 7 + 1;
    term->font_height = 7 + 1;
    term->fg_colour = GREEN;
    term->bg_colour = BLACK;
    term->cursor_colour = YELLOW;
    term_clear(term);
    return term;
}


void term_destroy (TermInfo * term) {
    free(term->output);
    free(term);
}


void term_gotoxy (TermInfo *term, uint8_t x, uint8_t y) {
    if (x >= term->width) term->cursor_x = term->width - 1;
    else term->cursor_x = x;
    if (y >= term->height) term->cursor_y = term->height - 1;
    else term->cursor_y = y;
}


void term_cursor_down (TermInfo *term) {
    if (++term->cursor_y >= term->height) {
        memcpy(term->output, (char *)(term->output + term->width), (term->width * term->height) - term->width);
        memset((char *)(term->output + ((term->width * term->height) - term->width)), 0, term->width);

        memcpy(
            (char *)term->colours, 
            (char *)(term->colours + (term->width * 2)), 
            (term->width * term->height * 2) - (term->width * 2)
        );

        memset(
            (char *)(term->colours + (term->width * term->height * 2) - (term->width * 2)), 
            0,
            term->width * 2
        );
        term->cursor_y = term->height - 1;
    }
}


void term_cursor_right (TermInfo *term) {
    if (++term->cursor_x >= term->width) {
        term->cursor_x = 0;
        term_cursor_down(term);
    }
}


void term_print (TermInfo *term, char *text) {
    for (int i = 0, l = strlen(text); i < l; i++) {
        if (text[i] == '\n') {
            term_cursor_down(term);
        } else if (text[i] == '\r') {
            term->cursor_x = 0;
        } else {
            int idx = term->cursor_y * term->width + term->cursor_x;
            term->output[idx] = text[i];
            term->colours[idx] = term->fg_colour;
            term_cursor_right(term);
        }
    }
}


void term_display (Surface *surface, TermInfo *term) {
    char tmp[2] = { 0, 0 };
    for (int x = 0, y = 0, i = 0, l = term->width * term->height; i < l; i++) {
        if (term->output[i] != 0) {
            tmp[0] = term->output[i];
            font_print(surface, &font_small, tmp, x * term->font_width, 1 + y * term->font_height, term->colours[i]);
        }
        if (++x >= term->width) {
            x = 0;
            y++;
        }
    }
    font_print(surface, &font_small, term->cursor, term->cursor_x * term->font_width, 1 + term->cursor_y * term->font_height, term->cursor_colour);
}


void term_special (TermInfo *term, char *cmd) {
    uint16_t term_colours[] = {
        BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, WHITE, WHITE
    };
    if (strlen(cmd) >= 4) {
        switch(cmd[1]) {
            case '[':
                if (cmd[2] == '3' && cmd[3]-48 < 10) 
                    term->fg_colour = term_colours[cmd[3] - 48];
                else if (cmd[2] == '4' && cmd[3]-48 < 10) 
                    term->bg_colour = term_colours[cmd[3] - 48];
                break;
            default:
                break;
        }
    }
}


bool term_input_poll (TermInfo *term) {
    char ch = getchar_timeout_us(500);
    if (ch == NULL) return false;
    if (ch == KEY_CR) {
        printf("\r\n");
        term->cursor_x = 0;
        term_cursor_down(term);
        term->input_finished = true;
        return true;
    } else if (ch == KEY_BACKSPACE) {
        if (strlen(term->input) > 0) {
            term->input[strlen(term->input)-1] = 0;
            if (--term->cursor_x < 0) term->cursor_x = 0;
        }
    } else if (ch == KEY_ESCAPE) {
        //start of escape sequence
        term->escape_command[0] = ch;
    } else if (ch >= KEY_SPACE && ch <= 126) {
        if (term->escape_command[0] != 0) {
            sprintf(term->escape_command, "%s%c", term->escape_command, ch);
            if (ch == 'm') {
                //end of escape sequence
                printf("\r\nterm_special: %s\r\n", (char *)(term->escape_command+1));
                term_special(term, term->escape_command);
                memset(term->escape_command, 0, sizeof(term->escape_command));
            }
        } else {
            printf("%c", ch);
            char tmp[2] = { ch, 0 };
            term_print(term, tmp);
            //term->output[term->cursor_y * term->width + term->cursor_x] = ch;
            snprintf(term->input, 256, "%s%c", term->input, ch);
            //term_cursor_right(term);
        }
    }
    return false;
}


int main () {
    //init std in/out
    stdio_init_all();

    //init the LCD panel and the surface we'll use to draw
    lcd_init();
    lcd_set_backlight(50);
    Surface *screen = surface_create(LCD_WIDTH, LCD_HEIGHT);

    /*
    //configure the ADC so we can read temp sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_gpio_init(PICO_VSYS_PIN);
    */

    TermInfo *term = term_create(LCD_WIDTH / 6, LCD_HEIGHT / 6);
    

    struct VM vm;
    vm_init(&vm);
    vm.video = screen;
    vm.font = &font_small;

    char vm_test_program[] = {
        OPCODE(&vm, "nop", ' ', ' '),
        OPCODE(&vm, "mov", 'i', 'r'),    0, 0x12, 0x34,    //mov R0, 0x1234
        OPCODE(&vm, "mov", 'r', 'm'),    0x55, 0x22, 0,    //mov 0x5522, R0
        OPCODE(&vm, "mov", 'm', 'r'),    1, 0x55, 0x22,    //mov R1, [0x5522]
        OPCODE(&vm, "mov", 'p', 'r'),    0, 0x55, 0x22,    //mov R0, [[0x5522]]
        OPCODE(&vm, "mov", 'r', 'r'),    2, 1,
        OPCODE(&vm, "stdout", ' ', ' '), 
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_INFO,
        OPCODE(&vm, "cmp", 'i', ' '),    0x55, 0x22,       //cmp 0x5522<=>R0
        OPCODE(&vm, "cmp", 'r', ' '),    0,                //cmp R0<=>R0
        OPCODE(&vm, "mov", 'i', 'r'),    0, 0x11, 0x22,    //mov R0, 0x1122
        OPCODE(&vm, "mov", 'i', 'r'),    4, 0x00, 0x02,    //mov R0, 0x0002
        OPCODE(&vm, "mov", 'i', 'r'),    5, 0x33, 0x44,    //mov R5, 0x3344
        OPCODE(&vm, "inc", 'r', ' '),    0,                //inc R0
        OPCODE(&vm, "dec", 'r', ' '),    0,                //dec R0
        OPCODE(&vm, "add", 'r', ' '),    5,                //add R0, R5
        OPCODE(&vm, "sub", 'r', ' '),    5,                //sub R0, R5
        OPCODE(&vm, "mul", 'r', ' '),    4,                //mul R0, R4
        OPCODE(&vm, "div", 'r', ' '),    4,                //div R0, R4
        OPCODE(&vm, "shl", 'r', ' '),    0,                //shl R0
        OPCODE(&vm, "shr", 'r', ' '),    0,                //shr R0
        //interrupts-----------------
        //GPIO Configure (pin, direction, pullup/pulldown)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 0x04, //pin
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 0x01, //dir
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0x00, 0x01, //pullup/pulldown
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_GPIO_CFG,
        //GPIO Set (pin, level)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 0x04, //pin
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 0x01, //level
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_GPIO_SET,
        //GPIO Get (pin)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 0x04, //pin
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_GPIO_GET,
        //VIDEO Put Pixel (x, y, colour)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 0x12, //x
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 0x34, //y
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0xf0, 0x0f, //colour
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_PUTPIXEL,
        //VIDEO Get Pixel (x, y)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 0x12, //x
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 0x34, //y
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_GETPIXEL,
        //VIDEO Fill (x, y, width, height, colour)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 50, //x
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 50, //y
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0x00, 100,  //width
        OPCODE(&vm, "mov", 'i', 'r'),    4, 0x00, 100,  //height
        OPCODE(&vm, "mov", 'i', 'r'),    5, 0xf0, 0x0f, //colour
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_FILL,
        //VIDEO Line (sx, sy, dx, dy, colour)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 10, //sx
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 10, //sy
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0x00, 40, //dx
        OPCODE(&vm, "mov", 'i', 'r'),    4, 0x00, 50, //dy
        OPCODE(&vm, "mov", 'i', 'r'),    5, 0x0f, 0xf0, //colour
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_LINE,
        //VIDEO Circle (cx, cy, radius, colour)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 10, //cx
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 10, //cy
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0x00, 40, //radius
        OPCODE(&vm, "mov", 'i', 'r'),    4, 0x7f, 0xf7, //colour
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_CIRCLE,
        //VIDEO Print (x, y, character, colour)
        OPCODE(&vm, "mov", 'i', 'r'),    1, 0x00, 50, //x
        OPCODE(&vm, "mov", 'i', 'r'),    2, 0x00, 25, //y
        OPCODE(&vm, "mov", 'i', 'r'),    3, 0x00, 'A', //character
        OPCODE(&vm, "mov", 'i', 'r'),    4, 0xf5, 0x00, //colour
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_PRINT,
        //VIDEO Update
        OPCODE(&vm, "int", 'i', ' '),    0x00, I_VIDEO_UPDATE,
        //---------------------------
        OPCODE(&vm, "hlt", ' ', ' ')    
    };

    srand(1337);

    /*
    char str[256];
    int frame = 0;
    while(1) {
        //clear screen
        surface_fill(screen, term->bg_colour);

        if (term_input_poll(term)) {
            char *args = NULL;
            for (int i = 0; i < strlen(term->input); i++) {
                if (term->input[i] == ' ') {
                    term->input[i] = 0;
                    args = (term->input + i + 1);
                    printf("\r\nARGS=%s\r\n", args);
                    break;
                }
            }
            if (strcmp(term->input, "ver") == 0) {
                term_print(term, "====================");
                term_print(term, "= dcm shell  0.0.1 =");
                term_print(term, "====================");
            } else if (strcmp(term->input, "temp") == 0) {
                sprintf(str, "TEMP: %.2f C\r\n", adc_read_temp());
                term_print(term, str);
            } else if (strcmp(term->input, "vsys") == 0) {
                sprintf(str, "VSYS: %f V\r\n", adc_read_vsys());
                term_print(term, str);
            } else if (strcmp(term->input, "bl") == 0) {
                if (args != NULL) {
                    int val = atoi(args);
                    lcd_set_backlight(val);
                    sprintf(str, "Backlight: %d\r\n", val);
                    term_print(term, str);
                } else {
                    term_print(term, "usage:\r\nbl <value>\r\n");
                }
            } else if (strcmp(term->input, "run") == 0) {
                vm_load(&vm, vm_test_program, sizeof(vm_test_program), 0x0200);
                while(vm.flags[F_HALT] == 0) {
                    vm_step(&vm);
                }
                sleep_ms(5000);
            } else {
                term_clear(term);
            }
            memset(term->input, 0, sizeof(term->input));
            term->input_finished = false;
        }

        term_display(screen, term);
        lcd_draw_surface(screen);
        sleep_ms(50);
        frame++;
    }
    */

    while(1) {
        if (term_input_poll(term)) {
            surface_fill(screen, 0x0000);
            vm_load(&vm, vm_test_program, sizeof(vm_test_program), 0x0200);
            while (vm.flags[F_HALT] == 0) {
                vm_step(&vm);
            }
            vm_init(&vm);
            vm.video = screen;
            vm.font = &font_small;
            memset(term->input, 0, sizeof(term->input));
            term->input_finished = false;
        }
        lcd_draw_surface(screen);
        sleep_ms(100);
    }

    free(term);
    return 0;
}
