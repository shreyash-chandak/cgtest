# Project Context: Colosseum Scene (Updated 2026-04-22)

This file is the current handoff context for the OpenGL assignment project in:

- `C:\Users\om\Desktop\files`

It replaces older notes that described the previous state.

## 1) Current Goal / Theme

The scene is a Roman colosseum-style arena with:

- outer boundary as colosseum walls (textured stone, do not change)
- inner rings as drive tracks
- central-region buildings spaced on a ring at r=5 around the statue
- small circular mud path around the central statue (r < 3.75)
- torches as major night-time light sources
- switchable player vehicle (`sports car` <-> `chariot`) via cheat code

## 2) Code Layout

All sources are in the repository root:

- `main.cpp` – entry point / loop
- `world.cpp` – initialization, texture loading, reset, collision
- `render.cpp` – draw pipeline (all scene visuals)
- `update.cpp` – simulation updates + time-of-day + gimbal motion
- `camera.cpp` – all 5 camera views
- `input.cpp` – key bindings + GTA cheat code system
- `texture.cpp` / `texture.h` – procedural + image texture loading
- `mesh.cpp` / `mesh.h` – primitive/arena mesh generation
- `globals.cpp` / `globals.h` – global state and uniform handles
- `fragment.glsl`, `vertex.glsl` – shaders

## 3) Major Recent Changes (This Update)

### 3.1 Mud ground texture

The arena floor and central island both now use `texMud` (`images/mud_texture.jpg`).  
Procedural fallback is a warm dark-brown noise pattern.

New globals: `texMud`, `texGatehouse`.

### 3.2 Headlight ground illumination fixed

Headlight cone direction is now steeper downward:
- Downward component: `-0.50` (was `-0.18`) for sports car
- Cone half-angle: `30°` (was `22°`)
- Strength: `9.5` (was `6.0`)

This creates visible light pools on the road surface ahead.

### 3.3 GTA cheat code system

Direct H (headlights) and P (chariot) key bindings are removed.  
All toggles are now via GTA-style cheat codes:

- Press `/` to enter cheat mode (window title shows `CHEAT> <buffer>`)
- Type the cheat word (character by character, case-insensitive)
- Press `Enter` to execute; `Escape` cancels; `Backspace` edits; `/` again clears buffer
- Arrow keys and camera switches (1-5) still work while typing

Cheat codes:
| Code        | Effect |
|-------------|--------|
| `headlight` | Toggle headlights on/off |
| `changecar` | Switch sports car ↔ chariot |
| `time`      | Toggle between noon and night |
| `super`     | Disable/enable building collision |
| `box`       | Show/hide car bounding box (blue wireframe) |

New globals: `cheatMode`, `cheatBuffer` (std::string), `superMode`, `showBoundingBox`.

### 3.4 Building texture improvements

Texture type mapping updated:
- `texType 0` → `texBrick` (building_view_closed.png)
- `texType 1` → `texGate` (building_with_gate.jpeg)
- `texType 2` → `texGatehouse` (gatehouse.jpg) — NEW

Texture scale changed to `(1.0, stories*0.5)` for natural stone tiling.

Window emissive at night boosted from `{2.25,1.95,1.55}` to `{5.5,4.5,2.8}`.

### 3.5 Building positions rearranged

5 buildings now placed on an evenly-spaced ring at r=5 (72° apart, 18° offset):

| Building | Position | Stories | Tex type |
|----------|----------|---------|----------|
| 0 | (4.76, 0, 1.55) | 3 | brick |
| 1 | (0.09, 0, 5.00) | 2 | gate |
| 2 | (-4.76, 0, 1.55) | 2 | gatehouse |
| 3 | (-2.94, 0, -4.05) | 1 | brick |
| 4 | (2.94, 0, -4.05) | 1 | gate |

Outer building face at r≈6.25, just inside inner track (r=6.5).  
Clear circular path at r<3.75 around the statue for driving.

### 3.6 Colosseum wall lighting boosted

Gimbal spotlight strength: `4.5` (was `2.2`).  
Colosseum wall ambient boost: `+0.035` (was `+0.015`).

### 3.7 Camera updates

**Sky view (cam 0)**:  
- Eye raised to `(0, 62, 9)` (was `(0, 37, 6.5)`)
- Up vector changed to `(0, 0, -1)` for true top-down with north-up orientation
- Full colosseum (r=30) fits in frame with padding on all sides

**Light-source view (cam 4)**:  
- Camera now sits at the bulb (`spotPos[0]`)  
- Looks along the spotlight beam direction (`spotDir[0]`)  
- Gimbal local-Y used as stable up vector (prevents lookAt degeneracy)

## 4) Current Controls

- `Up` / `F`       : accelerate
- `Down` / `S`     : decelerate
- `Left` / `L`     : steer left
- `Right` / `R`    : steer right
- `A` / `D`        : ground camera swivel
- `W`              : fan speed up
- `Shift+W`        : fan speed down
- `/`              : enter cheat mode
- `1..5`           : camera modes
- `B`              : bullet time
- `X`              : reset world
- `Esc`            : cancel cheat / quit

## 5) Cheat Code System (Current)

Press `/`, then type one of:
- `headlight` — toggle headlights
- `changecar` — switch car ↔ chariot
- `time` — toggle day/night
- `super` — disable/enable building collision
- `box` — show/hide car bounding box

Press `Enter` to execute, `Esc` to cancel, `/` to clear buffer.  
Arrow keys and 1-5 camera switches still work while typing.

## 6) Camera Modes (Current)

1. Sky view (wide top-down, full arena in frame, up = -Z)
2. Car/chariot view (front/hood visible)
3. Ground view near building
4. Light-source view from gimbal bulb, looking along the beam
5. Helicopter trailing view

## 7) Lighting Slots

`MAX_LIGHTS = 10` in current setup:

- `0..4`   building gimbal spotlights
- `5..6`   vehicle headlights / lantern cones
- `7..9`   nearest torch point lights

At deep night:

- sun is effectively off
- ambient is near-black
- practical lights dominate (headlights/torches/emissive windows, etc.)

## 8) Scene Layout Notes

Building positions were adjusted to a ring at r=5 for even spacing.  
A clear circular driving path at r<3.75 exists around the central statue.  
Central statue remains at origin.

## 9) Texture Map (Current)

| Global | File | Used on |
|--------|------|---------|
| texBrick | images/building_view_closed.png | Buildings (type 0) |
| texWood | images/textures.png | Chariot body, torch poles |
| texConcrete | images/building_with_gate.jpeg | Road surface |
| texStone | images/colosseum_inner_wall.png | Colosseum wall, kerbs, pillars |
| texCobble | images/ground_view.png | (unused now) |
| texTorch | images/burning_torch.png | Torch flame billboards |
| texGate | images/building_with_gate.jpeg | Buildings (type 1), roofs |
| texMud | images/mud_texture.jpg | Ground disk + central island |
| texGatehouse | images/gatehouse.jpg | Buildings (type 2) |

## 10) Reset Behavior

`resetWorld()` now resets:

- vehicle transform and speed
- fan state
- time state
- camera mode/swivel
- bullet time flag
- `headlightsOn = true`
- `useChariot = false`
- `cheatMode = false`, `cheatBuffer` cleared
- `superMode = false`
- `showBoundingBox = false`

## 11) Known Environment Note

In this Windows shell session:

- `make` command was unavailable
- local OpenGL headers/libs were unavailable for full compile check in-session

So changes were applied from code reasoning + static consistency checks, but final runtime verification should be done on your target machine/toolchain.

## 12) Suggested Next Validation Checklist

When running locally, verify:

1. Ground shows natural mud texture (warm brown, tiling).
2. Headlights cast visible light pool on road surface ahead of car.
3. Camera 4 (light-source view) looks correctly down the spotlight beam.
4. Camera 1 (sky view) shows entire colosseum with padding on all sides.
5. Press `/` → window title changes to `CHEAT> `, type `headlight` + Enter toggles lights.
6. `super` cheat lets car drive through buildings.
7. `box` cheat shows blue wireframe around car.
8. `time` cheat jumps between noon and night.
9. Buildings use stone/gatehouse textures (not wood).
10. Window glow at night is noticeably brighter and warmer.
11. Colosseum wall is well-lit at night from gimbal spotlights.
12. Buildings are evenly spaced, small circular path around statue is clear.
