# -*- Makefile -*-

CC=/usr/bin/gcc
COMPILER_FLAGS=-Wall -g
LINKER_FLAGS=`sdl2-config --libs --cflags` -lSDL2_gfx -lSDL2_image -lSDL2_ttf -lm

bin/gravity: build/main.o build/sdl.o build/game.o build/menu.o build/controls.o build/physics.o build/maths.o build/graphics.o build/events.o build/utilities.o build/console.o build/stars.o build/galaxies.o build/pcg.o
	$(CC) $(COMPILER_FLAGS) build/main.o build/sdl.o build/game.o build/menu.o build/controls.o build/physics.o build/maths.o build/graphics.o build/events.o build/utilities.o build/console.o build/stars.o build/galaxies.o build/pcg.o $(LINKER_FLAGS) -o bin/gravity

build/main.o: src/main.c include/constants.h include/enums.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/main.c $(LINKER_FLAGS) -o build/main.o

build/sdl.o: src/sdl.c include/constants.h include/enums.h include/structs.h include/sdl.h
	$(CC) -c $(COMPILER_FLAGS) src/sdl.c $(LINKER_FLAGS) -o build/sdl.o

build/game.o: src/game.c include/constants.h include/enums.h include/structs.h include/game.h
	$(CC) -c $(COMPILER_FLAGS) src/game.c $(LINKER_FLAGS) -o build/game.o

build/menu.o: src/menu.c include/constants.h include/enums.h include/structs.h include/menu.h
	$(CC) -c $(COMPILER_FLAGS) src/menu.c $(LINKER_FLAGS) -o build/menu.o

build/controls.o: src/controls.c include/constants.h include/enums.h include/structs.h include/controls.h
	$(CC) -c $(COMPILER_FLAGS) src/controls.c $(LINKER_FLAGS) -o build/controls.o

build/physics.o: src/physics.c include/constants.h include/enums.h include/structs.h include/physics.h
	$(CC) -c $(COMPILER_FLAGS) src/physics.c $(LINKER_FLAGS) -o build/physics.o

build/maths.o: src/maths.c include/constants.h include/enums.h include/structs.h include/maths.h
	$(CC) -c $(COMPILER_FLAGS) src/maths.c $(LINKER_FLAGS) -o build/maths.o

build/graphics.o: src/graphics.c include/constants.h include/enums.h include/structs.h include/graphics.h
	$(CC) -c $(COMPILER_FLAGS) src/graphics.c $(LINKER_FLAGS) -o build/graphics.o

build/events.o: src/events.c include/constants.h include/enums.h include/structs.h include/events.h
	$(CC) -c $(COMPILER_FLAGS) src/events.c $(LINKER_FLAGS) -o build/events.o

build/utilities.o: src/utilities.c include/constants.h include/enums.h include/structs.h include/utilities.h
	$(CC) -c $(COMPILER_FLAGS) src/utilities.c $(LINKER_FLAGS) -o build/utilities.o

build/console.o: src/console.c include/constants.h include/enums.h include/structs.h include/console.h
	$(CC) -c $(COMPILER_FLAGS) src/console.c $(LINKER_FLAGS) -o build/console.o

build/stars.o: src/stars.c include/constants.h include/enums.h include/structs.h include/stars.h
	$(CC) -c $(COMPILER_FLAGS) src/stars.c $(LINKER_FLAGS) -o build/stars.o

build/galaxies.o: src/galaxies.c include/constants.h include/enums.h include/structs.h include/galaxies.h
	$(CC) -c $(COMPILER_FLAGS) src/galaxies.c $(LINKER_FLAGS) -o build/galaxies.o

build/pcg.o: lib/pcg-c-basic-0.9/pcg_basic.c
	$(CC) -c $(COMPILER_FLAGS) lib/pcg-c-basic-0.9/pcg_basic.c $(LINKER_FLAGS) -o build/pcg.o

clean:
	$(RM) bin/* build/*