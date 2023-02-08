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

| Mode       | Key     | Action         |
| ---------- | ------- | -------------- |
| Navigation |         |                |
|            | `Up`    | Thrust         |
|            | `Down`  | Reverse thrust |
|            | `Right` | Rotate right   |
|            | `Left`  | Rotate left    |
|            | `S`     | Stop ship      |
|            | `C`     | Toggle camera  |
|            | `[`     | Zoom out       |
|            | `]`     | Zoom in        |
| Map        | `M`     | Enter Map      |
|            | `Up`    | Scroll up      |
|            | `Down`  | Scroll down    |
|            | `Right` | Scroll right   |
|            | `Left`  | Scroll left    |
|            | `[`     | Zoom out       |
|            | `]`     | Zoom in        |
|            | `Space` | Reset Map      |
|            | `Esc`   | Exit Map       |
| `K`        |         | Toggle console |
| `P`        |         | Pause game     |
| `Esc`      |         | Exit program   |

## Console contents

-   `1st line` Frames per second
-   `2nd line` X offset
-   `3rd line` Y offset
-   `4th line` Velocity magnitude
-   `5th line` Game scale

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
