# Project Context: Colosseum Scene

Updated: 2026-04-22
Workspace: `C:\Users\om\Desktop\files`

## Overview

OpenGL 3.3 C++ scene with:

- colosseum outer wall boundary
- two drivable track rings
- central statue zone
- interior buildings with moving gimbal lights
- day/night lighting + cheat-code input mode

## Key Files

- `render.cpp`: drawing, materials, light slot setup
- `update.cpp`: frame update, gimbal transform updates
- `camera.cpp`: 5 camera views
- `world.cpp`: texture load/init/reset/collision
- `globals.cpp`: building placement and global defaults
- `constants.h`: world/light slot constants
- `fragment.glsl`, `shader.cpp`: fragment shader + fallback shader

## Current Implemented State

### 1) Mud floor mapping fixed

Mud texture (`images/mud_texture.jpg`) is now visible as:

- a large world mud floor (outside arena too)
- an arena mud base disk
- the central island mud patch

This removed the "blank outside world" look.

### 2) Buildings reworked to requested style and scale

- `NUM_B = 4`
- all buildings are 4+ stories (4 or 5 stories)
- moved toward the wall to free center circular driving space
- exactly two use `building_view_closed.png`
- exactly two use `building_with_gate.jpeg`

Gate buildings are now modeled with a real opening (not a solid box).

### 3) Gate pass-through collision logic

Collision now keeps building solids, but for gate buildings (`texType == 1`) it leaves a centered drive-through corridor open.

Relevant files:

- `world.cpp`
- `constants.h` (`B_GATE_HALF`)

### 4) Headlights + dipper lights

Existing forward headlights remain.
Added two lower dipper lights aimed strongly downward so near-ground illumination appears in front of the vehicle.

Light slots:

- gimbals: `0-3`
- headlights: `4-5`
- dippers: `6-7`
- torches: `8-10`
- total `MAX_LIGHTS = 12`

### 5) Swinging/gimbal lights at night with visible beams

- gimbal lights now stay active at night (`gimbalStrength = lightsOn * max`)
- each building keeps its own light color
- visible emissive beam geometry is drawn from bulb to ground at night
- beam impact glow drawn on ground

### 6) Gimbal aim changed to floor targeting

Gimbal beam direction is now based on nearest ground point below the bulb, so beams point down to the floor.

### 7) Light-source camera (cam mode 3) corrected

Camera now:

- sits on a swinging bulb
- looks toward the wall
- uses a direction normal to the light beam (not beam-forward)
- chooses the best light where that beam-normal aligns with wall direction

### 8) Cheat-code mode and sky view

Already working and kept as-is:

- cheat mode starts with `/`
- codes: `headlight`, `changecar`, `time`, `super`, `box`
- default sky view framing remains improved

## Building Defaults (Current)

Defined in `globals.cpp`:

- `(18.5, 0, 9.0)` stories=4, closed facade
- `(8.0, 0, 17.5)` stories=5, gate facade
- `(-18.5, 0, 9.0)` stories=4, closed facade
- `(-8.0, 0, -17.5)` stories=5, gate facade

## Assets Used

From `images/`:

- `mud_texture.jpg`
- `building_view_closed.png`
- `building_with_gate.jpeg`
- `colosseum_inner_wall.png`
- `burning_torch.png`
- `textures.png`
- `ground_view.png`
- `gatehouse.jpg` (still available, not primary in current 4-building style)

## Notes

- Colosseum wall geometry kept unchanged.
- Local shell environment here does not have OpenGL headers (`GL/glew.h`) for compile/run verification, so runtime validation should be done on the local graphics setup.
