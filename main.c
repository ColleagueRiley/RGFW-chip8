#define RGFW_BUFFER
#define RGFW_IMPLEMENTATION
#include "RGFW.h"

u8 memory[4096];

void clear(RGFW_window* win) {
	size_t index;
	for (index = 0; index < win->r.w * win->r.h * 4; index += 4) {
		win->buffer[index] = 0x00;
		win->buffer[index + 1] = 0x00;
		win->buffer[index + 2] = 0x00;
		win->buffer[index + 3] = 0xFF;
	}	
}

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
	
	RGFW_setBufferSize(RGFW_AREA(64, 32));
	RGFW_window* win = RGFW_createWindow("RGFW Chip-8", RGFW_RECT(0, 0, 64, 32), RGFW_CENTER | RGFW_NO_RESIZE); 

	clear(win);

	size_t index;
	for (index = 0; index < win->r.w * win->r.h * 4; index += 4) {
		win->buffer[index] = 0x00;
		win->buffer[index + 1] = 0x00;
		win->buffer[index + 2] = 0x00;
		win->buffer[index + 3] = 0xFF;
	}
						
	u16 PC = 0;
	u8 registers[16];
	u16 stack[16];
	u8 stack_layer = 0;
	u8 delay_timer;
	u8 sound_timer;
	u16 I;

	i8 waitForKey = 0;

	while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
		while (RGFW_window_checkEvent(win) != NULL); {
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
		
		if (PC == size || waitForKey != -1)
			continue;
	
		if (PC < size) {
			PC += 2;
		}

		u16 X = opcode & 0x0F00;
		u16 Y = (0x00F0 & opcode);
		u16 N = 0x000F & opcode;
		u16 NN = 0x00FF & opcode;
		i16 NNN = 0x0FFF & opcode; 

		switch ((opcode & 0xF000)) {
			case 0x0000:
				switch (NN) {
					case 0xE0: { // 00E0 | clear()
						clear(win);
						break;
					}
					case 0xEE: // 00EE | return 
						if (stack_layer) {
							PC = stack[stack_layer - 1];
							stack_layer--;
						}
						break;
					default: // 0NNN | RCA 1802 at address NNN
						printf("TODO 0NNN 0 %i\n", NNN);
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
				size_t w, h, i;
				size_t offset = 0;

				u8 pixel;
				for (h = 0; h < 8; h++) {
					pixel = memory[I + h];
					for(w = 0; w < 8; w++){
						if((pixel & (0x80 >> w)) != 0){
							if(win->buffer[registers[X] + h + ((registers[Y] + w) * win->r.w) + offset] == 1){
								registers[15] = 1;
							}	
							win->buffer[(registers[X] + h + ((registers[Y] + w) * win->r.w)) + offset] ^= 1;
							offset += 4;
						}
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
	}

	RGFW_window_close(win);
}
