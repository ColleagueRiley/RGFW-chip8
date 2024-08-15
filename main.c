/*
	TODO :
	- fix bugs and make sure everything actually works
	
	- handle program end properly
	handle instructions:
		- 0NNN | RCA 1802 at address NNN
		- FX29 | I = sprite_addr[Vx]
		- FX33 | set_BCD(Vx)
*/


#define RGFW_BUFFER
#define RGFW_IMPLEMENTATION
#include "RGFW.h"

#include "util.h"

u8 memory[4096];

int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "No input rom\n");
		return -1;
	}
	
	FILE* f = fopen(argv[1], "rb");

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	fread(&memory, 1, size, f);
	fclose(f);

	memcpy(&memory[size], fontset, FONT_SIZE);
	
	RGFW_setBufferSize(RGFW_AREA(SCREEN_WIDTH, SCREEN_HEIGHT));
	RGFW_window* win = RGFW_createWindow("RGFW Chip-8", RGFW_RECT(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), RGFW_CENTER | RGFW_NO_RESIZE); 


	clear(win->buffer);
	
	u16 PC = 0;
	u8 registers[16];
	u16 stack[16];
	u8 stack_layer = 0;
	u8 delay_timer = 0;
	u8 sound_timer = 0;
	u16 I;

	i8 waitForKey = 0;

	while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
		while (RGFW_window_checkEvent(win) != NULL) {
			switch (win->event.type) {
				case RGFW_quit: break;
				case RGFW_keyPressed:
					c8_keymap[ RGFW_c8_LUT[win->event.keyCode] ] = 1;
				
					if (waitForKey != -1) {
						registers[waitForKey] = RGFW_c8_LUT[win->event.keyCode]; 
						waitForKey = -1;
					}
					break;
				case RGFW_keyReleased:
					c8_keymap[ RGFW_c8_LUT[win->event.keyCode] ] = 0;
					break;
				defualt: break;	
			}
		}
		
		RGFW_window_swapBuffers(win);
		u16 opcode = (memory[PC] << 8) | memory[PC + 1];
		
		/*
		if (PC == size || waitForKey != -1) {
			RGFW_window_checkFPS(win, 60);
			continue;
		}
		*/

		if (PC < size) {
			PC += 2;
		}
		
		if (sound_timer)
			sound_timer--;
		
		if (delay_timer)
			delay_timer--;

		u16 X = (opcode & 0x0F00) >> 8;
		u16 Y = (opcode & 0x00F0) >> 4;
		u16 N = (opcode & 0x000F);
		u16 NN = (opcode & 0x00FF);
		i16 NNN = (opcode & 0x0FFF); 
		
		switch ((opcode & 0xF000)) {
			case 0x0000:
				switch (NN) {
					case 0xE0: { // 00E0 | clear()
						clear(win->buffer);
						break;
					}
					case 0xEE: // 00EE | return 
						if (stack_layer) {
							PC = stack[stack_layer - 1];
							stack_layer--;
						}
						break;
					default: // 0NNN | RCA 1802 at address NNN
						//printf("TODO 0NNN 0 %i\n", NNN);
						break;
				}
				break;
			case 0x1000: // 1NNN | goto NNN
				PC = NNN;
				break;
			case 0x2000: // 2NNN | call function at NNN *(NNN)()
				PC = NNN;
				stack[stack_layer] = PC;
				stack_layer++;
				break;
			case 0x3000: // 3XNN | if (Vx == NN)
				if (registers[X] != NN)
					PC += 2; // skip next line if false
				break;
			case 0x4000: // 4XNN | if (Vx != NN)
				if (registers[X] == NN)
					PC += 2; // skip next line if false
				break;
			case 0x5000: // 5XY0 | if (Vx == Vy)
				if (registers[X] != registers[Y])
					PC += 2;
				break;
			case 0x6000: // 6XNN | Vx = NN
				registers[X] = (u16)NN;
				break;
			case 0x7000: // 7XNN | Vx += NN
				registers[X] += (u16) NN;
				break;
			case 0x8000:
				switch (N) {
					case 0x0: // 8XY0 | Vx = Vy
						registers[X] = registers[Y];
						break;		
					case 0x1: // 8XY1 | Vx |= Ny
						registers[X] |= registers[Y];
						break;
					case 0x2: // 8XY2 | Vx &= Vy
						registers[X] &= registers[Y];
						break;	
					case 0x3: // 8XY3 | Vx ^= Vy
						registers[X] ^= registers[Y];
						break;
					case 0x4: // 8XY4 | Vx += Vy
						registers[X] += registers[Y];
						break;
					case 0x5: // 8XY5 | Vx -= Vy
						registers[X] -= registers[Y];
						break;
					case 0x6: // 8XY6 | Vx >>= Vy
						registers[X] >>= registers[Y];
						break;
					case 0x7: // 8XY7 | Vx = Vy - Vx
						registers[X] = registers[Y] - registers[X];
						break;
					case 0xE: // 8XYE | Vx <<= 1
						registers[X] <<= 1;
						break;
				}
				break;
			case 0x9000: // 9XY0 | if (Vx != Vy)
				if (registers[X] == registers[Y])
					PC += 2;
				break;
			case 0xA000: // ANNN | I = NNN
				I = NNN;
				break;
			case 0xB000: // BNNN | PC = V0 + NNN
				PC = registers[0] + NNN;
				break;
			case 0xC000: // CXNN | VX = rand() & NN
				registers[X] = rand() % NN;
				break;
			case 0xD000: { // DXYN | draw(Vx, Vy, N)
				size_t x, y;
				u8 pixel_row;
				break;
				for (y = 0; y < 8; y++) {
					pixel_row = memory[I + y];
					for(x = 0; x < 8; x++) {
						u8 pixel = (pixel_row & 0x80) >> x;
						printf("0x%x\n", pixel_row);
						if (drawPixel(win->buffer, registers[X] + x, registers[Y] + y, pixel * 255, 0)) 
							registers[15] = 1;
					}
				}
				break;
			}
			case 0xE000:
				switch (Y) {
					case 0x9E: // EX9E | if (key() == Vx)
						if (c8_keymap[X] == 0)
							PC += 2;
						break;
					case 0xA1: // EXA1 | if (key != Vx)
						if (c8_keymap[X])
							PC += 2;
						break;
					default: break;
				}

				break;
			case 0xF000:
				switch (NN) {
					case 0x07: // FX07 | Vx = get_delay() 
						registers[X] = delay_timer;
						break;
					case 0x0A: // FX0A | Vx = get_key()
						waitForKey = X;
						break;
					case 0x15: // FX15 | delay_timer(Vx)
						delay_timer = registers[X];
						break;
					case 0x18: // FX18 | sound_timer(Vx)
						sound_timer = registers[X];
						break;
					case 0x1E: // FX1E | I += Vx
						I += registers[X];
						break;
					case 0x29: // FX29 | I = sprite_addr[Vx]
						printf("TODO (FX29) F %x 29", X);
						break;
					case 0x33: // FX33 | set_BCD(Vx)
						printf("TODO (FX33) F %x 33", X);
						break;
					case 0x55: // FX55 | reg_dump(Vx, &I);
						memcpy(&memory[I], registers, X);
						break;
					case 0x65: // FX65 | reg_load(Vx, &I);
						memcpy(registers, &memory[I], X);
						break;
					default: break;
				}
				break;
			default: break;
		}
			
		RGFW_window_checkFPS(win, 60);
	}

	RGFW_window_close(win);
}
