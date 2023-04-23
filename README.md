# Gravity

A vast procedural 2d universe that models gravity and orbital motion.

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

| Mode       | Key                              | Action               |
| ---------- | -------------------------------- | -------------------- |
| Navigation |                                  |                      |
|            | `Up`                             | Thrust               |
|            | `Down`                           | Reverse thrust       |
|            | `Right`                          | Rotate right         |
|            | `Left`                           | Rotate left          |
|            | `A`                              | Engage autopilot     |
|            | `C`                              | Toggle camera        |
|            | `M`                              | Enter Map mode       |
|            | `O`                              | Show orbits          |
|            | `S`                              | Stop ship            |
|            | `U`                              | Enter Universe mode  |
|            | `[ / Mouse Wheel Backward`       | Zoom out             |
|            | `] / Mouse Wheel Forward`        | Zoom in              |
|            | `Space`                          | Reset zoom scale     |
| Map        |                                  |                      |
|            | `Up`                             | Scroll up            |
|            | `Down`                           | Scroll down          |
|            | `Right`                          | Scroll right         |
|            | `Left`                           | Scroll left          |
|            | `N`                              | Enter Navigate mode  |
|            | `O`                              | Show orbits          |
|            | `U`                              | Enter Universe mode  |
|            | `W`                              | Center waypoint star |
|            | `[ / Mouse Wheel Backward`       | Zoom out             |
|            | `] / Mouse Wheel Forward`        | Zoom in              |
|            | `Space`                          | Reset zoom scale     |
|            | `Left Mouse Button Click`        | Select star          |
|            | `Left Mouse Button Double Click` | Center star          |
| Universe   |                                  |                      |
|            | `Up`                             | Scroll up            |
|            | `Down`                           | Scroll down          |
|            | `Right`                          | Scroll right         |
|            | `Left`                           | Scroll left          |
|            | `N`                              | Enter Navigate mode  |
|            | `M`                              | Enter Map mode       |
|            | `W`                              | Center waypoint star |
|            | `[ / Mouse Wheel Backward`       | Zoom out             |
|            | `] / Mouse Wheel Forward`        | Zoom in              |
|            | `Space`                          | Reset zoom scale     |
|            | `Left Mouse Button Double Click` | Center star          |
| `F`        |                                  | Toggle FPS           |
| `Esc`      |                                  | Show menu / Pause    |

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
