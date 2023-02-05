# Gravity

An infinite procedural 2d universe that models gravity and orbital motion.

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

```
make
```

## Keyboard controls

| Key     | Action         |
| ------- | -------------- |
| `Up`    | Thrust         |
| `Down`  | Reverse thrust |
| `Right` | Rotate right   |
| `Left`  | Rotate left    |
| `C`     | Toggle camera  |
| `K`     | Toggle console |
| `P`     | Pause game     |
| `S`     | Stop ship      |
| `Esc`   | Exit program   |

## Console contents

-   `1st line` Frames per second
-   `2nd line` X axis distance from Sun
-   `3rd line` Y axis distance from Sun
-   `4th line` Velocity vector

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
