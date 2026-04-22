# Project Context: Colosseum Scene (Updated 2026-04-22)

This file is the current handoff context for the OpenGL assignment project in:

- `C:\Users\om\Desktop\files`

It replaces older notes that described the previous state.

## 1) Current Goal / Theme

The scene is now a Roman colosseum-style arena with:

- outer boundary as colosseum walls
- inner rings as drive tracks
- central-region buildings
- torches as major night-time light sources
- switchable player vehicle (`sports car` <-> `chariot`)

## 2) Code Layout

All sources are currently in the repository root (not a `src/` subfolder):

- `main.cpp` – entry point / loop
- `world.cpp` – initialization, texture loading, reset, collision
- `render.cpp` – draw pipeline (all scene visuals)
- `update.cpp` – simulation updates + time-of-day + gimbal motion
- `camera.cpp` – all 5 camera views
- `input.cpp` – key bindings
- `texture.cpp` / `texture.h` – procedural + image texture loading
- `mesh.cpp` / `mesh.h` – primitive/arena mesh generation
- `globals.cpp` / `globals.h` – global state and uniform handles
- `fragment.glsl`, `vertex.glsl` – shaders

## 3) Major Recent Changes (This Update)

### 3.1 Image texture mapping is now wired

A file-based texture loader was added using `stb_image`:

- `stb_image.h` added to repo root
- `loadImageTex(...)` added in `texture.cpp`
- declaration added in `texture.h`

Scene now attempts to load textures from `images/` first, with procedural fallback.

Texture assignment in `world.cpp`:

- `texBrick`    <- `images/building_view_closed.png`
- `texWood`     <- `images/textures.png`
- `texConcrete` <- `images/building_with_gate.jpeg`
- `texStone`    <- `images/colosseum_inner_wall.png`
- `texCobble`   <- `images/ground_view.png`
- `texTorch`    <- `images/burning_torch.png`
- `texGate`     <- `images/building_with_gate.jpeg`

New globals:

- `texTorch`, `texGate`

### 3.2 Vehicle mode toggle (`p`)

New mode flag:

- `useChariot` (`false` = metallic sports car, `true` = chariot)

New input:

- `P` toggles vehicle style at runtime

Render behavior:

- sports car path retained
- new chariot path added (wooden body/rails, bigger wheels, spokes, lantern-like front lights)

### 3.3 Day/night behavior made darker at night

Two important fixes:

1. Fade timing fixed:
- sky blending and light fade now happen near **end of period** (not start)

2. Ambient correction:
- ambient term is now scaled by `sunStrength` in shaders
- at night, ambient becomes near-zero, so torch/headlight/emissive lights dominate

### 3.4 Camera updates

- **Sky view** tightened (less overly broad top view)
- **Car view** adjusted to reveal more front/hood context
- **Light-source view** reworked:
  - camera sits above bulb using gimbal normal axis
  - view points with fixture normal orientation (instead of only along beam)

### 3.5 Building windows at night

Buildings now render outward-facing panes on facades:

- deterministic pseudo-random pattern
- at least ~half lit
- lit panes get emissive warm glow at night (`lightsOn`)

### 3.6 Torches visual pass

Torch rendering improved:

- textured wood pole + textured bowl
- crossed textured flame billboards on top of existing emissive flame spheres

## 4) Current Controls

- `Up` / `F`       : accelerate
- `Down` / `S`     : decelerate
- `Left` / `L`     : steer left
- `Right` / `R`    : steer right
- `A` / `D`        : ground camera swivel
- `W`              : fan speed up
- `Shift+W`        : fan speed down
- `H`              : headlights on/off
- `P`              : sports car <-> chariot
- `1..5`           : camera modes
- `B`              : bullet time
- `X`              : reset world
- `Esc`            : quit

## 5) Camera Modes (Current)

1. Sky view (tightened top-down framing)
2. Car/chariot view (front/hood visible)
3. Ground view near building
4. Light-source view from gimbal fixture top/normal
5. Helicopter trailing view

## 6) Lighting Slots

`MAX_LIGHTS = 10` in current setup:

- `0..4`   building gimbal spotlights
- `5..6`   vehicle headlights / lantern cones
- `7..9`   nearest torch point lights

At deep night:

- sun is effectively off
- ambient is near-black
- practical lights dominate (headlights/torches/emissive windows, etc.)

## 7) Scene Layout Notes

Building positions were adjusted to fit the intended inner arena composition better and avoid center overlap.

Central statue remains in place.

## 8) Reset Behavior

`resetWorld()` now resets:

- vehicle transform and speed
- fan state
- time state
- camera mode/swivel
- bullet time flag
- `headlightsOn = true`
- `useChariot = false`

## 9) Known Environment Note

In this Windows shell session:

- `make` command was unavailable
- local OpenGL headers/libs were unavailable for full compile check in-session

So changes were applied from code reasoning + static consistency checks, but final runtime verification should be done on your target machine/toolchain.

## 10) Suggested Next Validation Checklist

When running locally, verify:

1. `P` cleanly toggles between sports car and chariot without light/camera glitches.
2. At night, scene is genuinely dark and readable mainly via torches/headlights/windows.
3. Camera 1/2/4 framing feels right in motion.
4. Window emissive pattern looks natural and outward-facing.
5. Texture files in `images/` are found correctly from working directory.
