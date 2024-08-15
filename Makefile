CC = gcc

# used for the examples
CFLAGS = 

LIBS := -static -lgdi32 -lm -lopengl32 -lwinmm -ggdb
EXT = .exe

#WARNINGS =  -Wall -Werror -Wstrict-prototypes -Wextra -Wstrict-prototypes -Wold-style-definition -Wno-missing-field-initializers -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-braces -Wno-missing-variable-declarations -Wno-redundant-decls -Wno-unused-function -Wno-unused-label -Wno-unused-result -Wno-incompatible-pointer-types -Wno-format -Wno-format-extra-args -Wno-implicit-function-declaration -Wno-implicit-int -Wno-pointer-sign -Wno-switch -Wno-switch-default -Wno-switch-enum -Wno-unused-value -Wno-type-limits
OS_DIR = \\

detected_OS = windows

ROM := TETRIS.ch8

# not using a cross compiler
ifeq (,$(filter $(CC),x86_64-w64-mingw32-gcc i686-w64-mingw32-gcc x86_64-w64-mingw32-g++ /opt/msvc/bin/x64/cl.exe /opt/msvc/bin/x86/cl.exe))
	detected_OS := $(shell uname 2>/dev/null || echo Unknown)

	ifeq ($(detected_OS),Darwin)        # Mac OS X
		LIBS := -lm -framework Foundation -framework AppKit -framework OpenGL -framework CoreVideo
		EXT =
		OS_DIR = /
	endif
	ifeq ($(detected_OS),Linux)
    	LIBS := -lXrandr -lX11 -lm -lGL -ldl -lpthread
		EXT =
		OS_DIR = /
	endif
else
	OS_DIR = /
endif

output: main.c RGFW.h
	$(CC) $< $(LIBS) $(WARNINGS) -o $@

clean:
	rm -f output

debug: output
	./output $(ROM)
	make clean
