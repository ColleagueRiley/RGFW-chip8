#ifndef RGFW_HEADER // for clangd
#define RGFW_BUFFER
#include "RGFW.h"
#endif

#define SCREEN_SCALE 10 

#define C8_SCREEN_WIDTH 64
#define C8_SCREEN_HEIGHT 32 

#define SCREEN_WIDTH C8_SCREEN_WIDTH * SCREEN_SCALE
#define SCREEN_HEIGHT C8_SCREEN_HEIGHT * SCREEN_SCALE

RGFW_ENUM(u8, c8_key) {
	c8_0 = 0, c8_1, c8_2, c8_3, c8_4, c8_5, c8_6, c8_9, 
	c8_a, c8_b, c8_c, c8_d, c8_e, ch8_f,
	c8_last
};

u8 c8_keymap[c8_last] = { 0 };

u8 RGFW_c8_LUT[] =  {
	[RGFW_1] = 0x1, [RGFW_2] = 0x2, [RGFW_3] = 0x3, [RGFW_4] = 0xC,
	[RGFW_q] = 0x4, [RGFW_w] = 0x5,  [RGFW_e] = 0x6, [RGFW_r] = 0xD,  
	[RGFW_a] = 0x7, [RGFW_s] = 0x8, [RGFW_d] = 0x9, [RGFW_f] = 0xE,
	[RGFW_z] = 0xA, [RGFW_x] = 0x0, [RGFW_c] = 0xB, [RGFW_v] = 0xF
};

#define FONT_SIZE 80
const u8 fontset[FONT_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

b8 drawPixel(u8* buffer, u32 x, u32 y, u8 color, u8 force) {
	x *=  SCREEN_SCALE;
	y *= SCREEN_SCALE;
	
	size_t offset = ((y * SCREEN_WIDTH) + x) * 4;
	if (buffer[offset] == color && force == RGFW_FALSE)
		return RGFW_FALSE;
	
	u32 X, Y; 
	for (Y = 0; Y < SCREEN_SCALE; Y++) {
		for (X = 0; X < SCREEN_SCALE; X++) {
			offset = (((y + Y) * SCREEN_WIDTH) + (x + X)) * 4;
			buffer[offset] = color;
			buffer[offset + 1] = color;
			buffer[offset + 2] = color;
			buffer[offset + 3] = 0xFF;
		}
	}

	return RGFW_TRUE;
}

void clear(u8* buffer) {
	u32 x, y;
	for (y = 0; y < C8_SCREEN_HEIGHT; y++) {
		for (x = 0; x < C8_SCREEN_WIDTH; x++) {
			drawPixel(buffer, x, y, 0, 1);
		}
	}
}
