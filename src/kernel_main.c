
#include <stdint.h>
#include "rprintf.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6


const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}


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
    while (1);  // stop here

}
