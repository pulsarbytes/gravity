# Gravity

A basic 2d game engine that models gravity and orbital motion.

## Prerequisites

### GCC (build-essential)

Install the build-essential package (Debian and Ubuntu):

```
apt install build-essential
```

### GDB

Install the gdb package (Debian and Ubuntu):

```
apt install gdb
```

### SDL (Simple DirectMedia Layer)

Install the SDL development libraries (Debian and Ubuntu):

```
apt install libsdl2-dev libsdl2-gfx-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-net-dev libsdl2-ttf-dev
```

## How to build

### Visual Studio Code

Use Visual Studio Code to make use of the included configuration and build files.

Install the extension [C/C++ by Microsoft](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) to enable features such as IntelliSense and debugging.

### Makefile

Alternatively, a Makefile is provided as a convenience.

## Keyboard controls

| Key     | Action         |
| ------- | -------------- |
| `Space` | Thrust         |
| `Down`  | Reverse thrust |
| `Right` | Rotate right   |
| `Left`  | Rotate left    |
| `K`     | Toggle console |
| `C`     | Toggle camera  |
| `Esc`   | Exit program   |

## Console contents

-   `1st line` Frames per second
-   `2nd line` X axis distance from Sun
-   `3rd line` Y axis distance from Sun
-   `4th line` Velocity vector

## Common issues

See [BUGS](BUGS.md).

## Licence

    Copyright (c) 2020 Yannis Maragos.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 only,
    as published by the Free Software Foundation.

        https://www.gnu.org/licenses/

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
