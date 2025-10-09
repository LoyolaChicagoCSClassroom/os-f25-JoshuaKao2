
#include <stdint.h>
#include "rprintf.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6


const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}


unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
 '9', '0', '-', '=', '\b',     /* Backspace */
 '\t',                 /* Tab */
 'q', 'w', 'e', 'r',   /* 19 */
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
   0,                  /* 29   - Control */
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 39 */
'\'', '`',   0,                /* Left shift */
'\\', 'z', 'x', 'c', 'v', 'b', 'n',                    /* 49 */
 'm', ',', '.', '/',   0,                              /* Right shift */
 '*',
   0,  /* Alt */
 ' ',  /* Space bar */
   0,  /* Caps lock */
   0,  /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,  
   0,  /* < ... F10 */
   0,  /* 69 - Num lock*/
   0,  /* Scroll Lock */
   0,  /* Home key */
   0,  /* Up Arrow */
   0,  /* Page Up */
 '-',
   0,  /* Left Arrow */
   0,  
   0,  /* Right Arrow */
 '+',
   0,  /* 79 - End key*/
   0,  /* Down Arrow */
   0,  /* Page Down */
   0,  /* Insert Key */
   0,  /* Delete Key */
   0,   0,   0,  
   0,  /* F11 Key */
   0,  /* F12 Key */
   0,  /* All other keys are undefined */
};







#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

struct termbuf {
    char ascii;
    char color;
};

void scroll_up(void) {
    struct termbuf *vram = (struct termbuf*)0xB8000;

    for (int row = 1; row < SCREEN_HEIGHT; row++)
        for (int col = 0; col < SCREEN_WIDTH; col++)
            vram[(row-1)*SCREEN_WIDTH + col] = vram[row*SCREEN_WIDTH + col];

    for (int col = 0; col < SCREEN_WIDTH; col++) {
        vram[(SCREEN_HEIGHT-1)*SCREEN_WIDTH + col].ascii = ' ';
        vram[(SCREEN_HEIGHT-1)*SCREEN_WIDTH + col].color = 7;
    }
}

int x = 0;
int y = 0;

int putc(int c){
    struct termbuf *vram = (struct termbuf*)0xB8000;

    if (c == '\n'){
    x = 0;
    y++;
    } else if (c == '\r'){
    x = 0;
    } else{
	vram[y*SCREEN_WIDTH + x].ascii = c;
	vram[y*SCREEN_WIDTH + x].color = 7;
	x++;
    }
    if (x>= SCREEN_WIDTH) {
	x = 0;
	y++;
    }
    if (y >= SCREEN_HEIGHT) {
        scroll_up();
        y = SCREEN_HEIGHT - 1;
    }
    return c;
}
void main() {
    for (int i = 1; i <= 30; i++) {
        esp_printf(putc, "Line %d: Sphinx of black quartz, judge my vow.\r\n", i);
    }

    esp_printf(putc, "Current execution level: Kernel mode (Ring 0)\r\n");


	while(1) {
        uint8_t status = inb(0x64);
	
		if((status & 1) == 1) {
            uint8_t scancode = inb(0x60);
            if (scancode < 128) {
				esp_printf(putc, "0x%02x %c\n",  scancode, keyboard_map[scancode]);
	    }
	}

}
