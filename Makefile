# -*- Makefile -*-

CC=/usr/bin/gcc
COMPILER_FLAGS=-Wall -g
LINKER_FLAGS=`sdl2-config --libs --cflags` -lSDL2_gfx -lSDL2_image -lSDL2_ttf -lm

bin/gravity: build/main.o build/sdl.o build/physics.o build/math.o build/events.o build/helper.o build/console.o build/pcg.o
	$(CC) $(COMPILER_FLAGS) build/main.o build/sdl.o build/physics.o build/math.o build/events.o build/helper.o build/console.o build/pcg.o $(LINKER_FLAGS) -o bin/gravity

build/main.o: src/main.c include/common.h include/structs.h lib/pcg-c-basic-0.9/pcg_basic.h
	$(CC) -c $(COMPILER_FLAGS) src/main.c $(LINKER_FLAGS) -o build/main.o

build/sdl.o: src/sdl.c include/common.h
	$(CC) -c $(COMPILER_FLAGS) src/sdl.c $(LINKER_FLAGS) -o build/sdl.o

build/physics.o: src/physics.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/physics.c $(LINKER_FLAGS) -o build/physics.o

build/math.o: src/math.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/math.c $(LINKER_FLAGS) -o build/math.o

build/events.o: src/events.c include/common.h
	$(CC) -c $(COMPILER_FLAGS) src/events.c $(LINKER_FLAGS) -o build/events.o

build/helper.o: src/helper.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/helper.c $(LINKER_FLAGS) -o build/helper.o

build/console.o: src/console.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/console.c $(LINKER_FLAGS) -o build/console.o

build/pcg.o: lib/pcg-c-basic-0.9/pcg_basic.c lib/pcg-c-basic-0.9/pcg_basic.h
	$(CC) -c $(COMPILER_FLAGS) lib/pcg-c-basic-0.9/pcg_basic.c $(LINKER_FLAGS) -o build/pcg.o

clean:
	$(RM) bin/* build/*