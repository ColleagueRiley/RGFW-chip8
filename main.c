#define RGFW_BUFFER
#define RGFW_IMPLEMENTATION
#include "RGFW.h"

#include "util.h"

#define RGFW_getTimeMS() (u32)((u64)RGFW_getTimeNS() / 1e+6)

b8 screen[C8_SCREEN_WIDTH * C8_SCREEN_HEIGHT];

u8 memory[4096];

int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "No input rom\n");
		return -1;
	}
	
	b8 oldFlag = (argc > 2 && argv[2][0] == '8');
	
	FILE* f = fopen(argv[1], "rb");

	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	fread(&memory[ROM_ADDRESS], 1, size, f);
	fclose(f);

	memcpy(&memory[FONT_ADDRESS], fontset, FONT_SIZE);
	
	srand(time(NULL));

	RGFW_setBufferSize(RGFW_AREA(SCREEN_WIDTH, SCREEN_HEIGHT));
	RGFW_window* win = RGFW_createWindow("RGFW Chip-8", RGFW_RECT(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), RGFW_CENTER | RGFW_NO_RESIZE); 


	memset(screen, 0, C8_SCREEN_WIDTH * C8_SCREEN_HEIGHT);	
	RGFW_window_swapBuffers(win);

	u16 PC = 0x200;
	u8 registers[16];
	u16 stack[16];
	u8 stack_pointer = 0;
	u8 delay_timer = 0;
	u8 sound_timer = 0;
	u16 I;

	i8 waitForKey = -1;
	
	b8 draw_queue = RGFW_FALSE;
	u32 lastTime = RGFW_getTimeMS();

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
		
		if (draw_queue) {
			draw(win->buffer, screen);
			RGFW_window_swapBuffers(win);
			draw_queue = RGFW_FALSE;
		}
			
		if (waitForKey != -1) {
			continue;
		}
		
		u16 opcode = (memory[PC] << 8) | memory[PC + 1];
		PC += 2;
			
		/* tick the timer */ 
		u32 time = RGFW_getTimeMS();

		if (((time - lastTime)) >= ((float)(1.0f / 60.0f)) * 1000 ) {
			if (sound_timer)
				sound_timer--;
		
			if (delay_timer) {
				delay_timer--;
				beep();
			}

			lastTime = time;
		}
		
		u16 X = (opcode & 0x0F00) >> 8;
		u16 Y = (opcode & 0x00F0) >> 4;
		u16 N = (opcode & 0x000F);
		u16 NN = (opcode & 0x00FF);
		i16 NNN = (opcode & 0x0FFF); 
		
		switch (opcode & 0xF000) {
			case 0x0000:
				switch (NN) {
					case 0xE0: { // 00E0 | clear()	
						memset(screen, 0, C8_SCREEN_WIDTH * C8_SCREEN_HEIGHT);	
						draw_queue = RGFW_TRUE;
						break;
					}
					case 0xEE: // 00EE | return 
						if (stack_pointer) {
							stack_pointer--;
							PC = stack[stack_pointer];
						}
						break;
					default: // 0NNN | RCA 1802 at address NNN
						break;
				}
				break;
			case 0x1000: // 1NNN | goto NNN
				PC = NNN;
				break;
			case 0x2000: // 2NNN | call function at NNN *(NNN)()
				stack[stack_pointer] = PC;
				PC = NNN;
				stack_pointer++;
				break;
			case 0x3000: // 3XNN | if (Vx == NN)
				if (registers[X] == NN)
					PC += 2; // skip next line if false
				break;
			case 0x4000: // 4XNN | if (Vx != NN)
				if (registers[X] != NN)
					PC += 2; // skip next line if false
				break;
			case 0x5000: // 5XY0 | if (Vx == Vy)
				if (registers[X] == registers[Y])
					PC += 2;
				break;
			case 0x6000: // 6XNN | Vx = NN
				registers[X] = (u8)NN;
				break;
			case 0x7000: // 7XNN | Vx += NN
				registers[X] += (u8) NN;
				break;
			case 0x8000:
				switch (N) {
					case 0x0000: // 8XY0 | Vx = Vy
						registers[X] = registers[Y];
						break;		
					case 0x0001: // 8XY1 | Vx |= Ny
						registers[X] |= registers[Y];
						break;
					case 0x0002: // 8XY2 | Vx &= Vy
						registers[X] &= registers[Y];
						break;	
					case 0x0003: // 8XY3 | Vx ^= Vy
						registers[X] ^= registers[Y];
						break;
					case 0x0004: { // 8XY4 | Vx += Vy
						if ((u16)((u16)registers[X] + (u16)registers[Y]) > 255) 
							registers[15] = 1;
						else
							registers[15] = 0;

						registers[X] += registers[Y];
						break;
					}
					case 0x0005: // 8XY5 | Vx -= Vy
						if ((i16)((i16)registers[X] - (i16)registers[Y]) < 0) 
							registers[15] = 1;
						else
							registers[15] = 0;
						
						registers[X] -= registers[Y];
						break;
					case 0x0006: { // 8XY6 | Vx >> Vy
						if(oldFlag){
							registers[X] = registers[Y];
						}
						
						registers[15] = registers[X] & 1;
						registers[X] >>= 1;

						break;
					}
					case 0x0007: // 8XY7 | Vx = Vy - Vx
						if(registers[Y] < registers[Y] - registers[X])
							registers[15] = 0;
						else
							registers[15] = 1;

						registers[X] = registers[Y] - registers[X];
						break;
					case 0x000E: // 8XYE | Vx << Vy
						if(oldFlag){
							registers[X] = registers[Y];
						}
						
						registers[15] = (registers[X] >> 7) & 1;
						registers[X] <<= 1;

						break;
					default:
						// jump with offset
						if(oldFlag) 
							PC = NNN + registers[0];
						else 
							PC = NNN + registers[X];
						break;
					}
					break;
			case 0x9000: // 9XY0 | if (Vx != Vy)
				if (registers[X] != registers[Y])
					PC += 2;
				break;
			case 0xA000: // ANNN | I = NNN
				I = NNN;
				break;
			case 0xB000: // BNNN | PC = V0 + NNN
				PC = registers[0] + NNN;
				break;
			case 0xC000: // CXNN | VX = rand() & NN
				registers[X] = (rand() % NN) & NN;
				break;
			case 0xD000: { // DXYN | draw(Vx, Vy, N)
				u8 pixel;
				
				if (registers[X] >= C8_SCREEN_WIDTH || (registers[Y] >= C8_SCREEN_HEIGHT)) 
					break;
				
				size_t x, y;
				registers[0xF] = 0;
				for(y = 0; y < N; y++){
					pixel = memory[I + y];
					for(x = 0; x < 8; x++) {
						if((pixel & (0x80 >> x))) {
							if (screen[((registers[Y] + y) * C8_SCREEN_WIDTH) + (registers[X] + x)])
								registers[0xF] = 1;
						
							screen[((registers[Y] + y) * C8_SCREEN_WIDTH) + (registers[X] + x)] ^= 1;
						}
					}
				}
				
				draw_queue = RGFW_TRUE;
				break;
			}
			case 0xE000:
				switch (NN) {
					case 0x9E: // EX9E | if (key() == Vx)
						if (c8_keymap[registers[X]])
							PC += 2;
						break;
					case 0xA1: // EXA1 | if (key != Vx)
						if (c8_keymap[registers[X]] == 0)
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
						I = I + registers[X];
						break;
					case 0x33: // FX33 | set_BCD(Vx)
						memory[I] = registers[X] / 100;
						memory[I+ 1] = (registers[X] / 10) % 10;
						memory[I + 2] = registers[X] % 10;
						break;
					case 0x55: // FX55 | reg_dump(Vx, &I);
						memcpy(&memory[I], registers, X);
						
						if (oldFlag) I += X + 1;
						break;
					case 0x65: // FX65 | reg_load(Vx, &I);
						memcpy(registers, &memory[I], X);
						
						if (oldFlag) I += X = 1;
						break;
					default: break;
				}
				break;
			default: break;
		}
		RGFW_window_checkFPS(win, 200);
	}	
	RGFW_window_close(win);
}
