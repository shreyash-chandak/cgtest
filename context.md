# Project Context: CSE7.302 Graphics Assignment 3 — Colosseum Scene

## Overview

This is a C++17/OpenGL 3.3 Core hierarchical 3D scene modelling assignment for the course **CSE7.302: Graphics, Spring 2026**. The assignment was originally a generic "road + buildings + car" scene. Through iterative development, it was redesigned into a **Roman Colosseum battle arena** theme. The codebase compiles with `g++ -std=c++17` and links against `libGL`, `libGLEW`, and `libglfw3`.

The project lives in a directory called `scene/`. The build system is a simple `Makefile`. All source files live in `scene/src/`. Two GLSL shader files live at the project root (`scene/vertex.glsl`, `scene/fragment.glsl`). Run with `make && ./scene` from the `scene/` directory.

---

## Assignment Requirements (Original Spec)

The original spec (`cga3.txt`) required:

- A circular/oval track like a tarred road
- A car that moves forward/backward with speed control (`f` = faster, `s` = slower), stationary at start
- Metallic shiny car body
- 4–5 buildings on the sides of the track, 1–3 stories, textured (brick, wood, etc.)
- Each building has a windmill/fan on the road-facing side that spins, speed controllable (`w` = faster, `Shift+W` = slower)
- Each building has a gimbal-mounted spotlight that swings ±30° left/right sinusoidally, pointing at the nearest road point, each a different colour
- A wall or boundary enclosing the scene
- Car steering: `l` = steer left, `r` = steer right (small angle per press)
- Collision detection: car stops when hitting wall or buildings (OBB check), keyboard action to reset (`x`)
- Day/night or lighting variations encouraged
- **Five camera views** (switchable via keyboard):
  1. Sky view — top-down, centre of arena
  2. Car view — roof-mounted, sees front of car
  3. Ground view — stationary near a building, swivels left/right
  4. Light-source view — rides the swiveling gimbal
  5. Helicopter cam — fixed offset behind the car, moves with it
- All rendering on GPU via vertex shaders; no hardcoded values

---

## What Was Built: The Colosseum Scene

The rectangular arena and flat ground were replaced with a **Roman Colosseum** world. The scene layout, described as a wheel from a top-down sketch:

```
Outermost ring  : Colosseum wall (3-tier hollow cylinder, stone textured)
                  24 stone pillars on the inner rim
                  16 fire torches spaced evenly on the inner rim
Ring 2          : Sand/seating area (between wall and outer track)
Outer track     : Elliptical asphalt ring (semi-axes 22×17)
Median strip    : Grass + 20 scattered trees (deterministic positions)
Inner track     : Elliptical asphalt ring (semi-axes 9×6.5)
Central island  : Grass + 5 medieval buildings + 1 warrior statue (centre)
```

---

## File Structure

```
scene/
├── Makefile
├── vertex.glsl          — Vertex shader (unchanged from original)
├── fragment.glsl        — Fragment shader (heavily modified)
└── src/
    ├── gl_common.h      — GLEW/GLFW/GLM include-order guard (MUST be first GL include)
    ├── constants.h      — All numeric constants (single source of truth)
    ├── types.h          — Mesh struct + BuildingInfo struct
    ├── globals.h        — extern declarations of all global state
    ├── globals.cpp      — definitions of all global state
    ├── shader.h/.cpp    — compile/link shaders, cache uniforms, material helpers
    ├── texture.h/.cpp   — procedural texture generation (brick/wood/concrete/stone/cobble)
    ├── mesh.h/.cpp      — CPU-side mesh generation for all geometry
    ├── world.h/.cpp     — initGL(), resetWorld(), checkCollision()
    ├── input.h/.cpp     — GLFW key callback
    ├── camera.h/.cpp    — Five camera views
    ├── update.h/.cpp    — Per-frame physics, animation, time-of-day
    ├── render.h/.cpp    — Full scene draw call sequence
    └── main.cpp         — Entry point: window + GLFW + main loop
```

### Critical Include Rule
**`gl_common.h` must be the first include in every `.cpp` file.** GLEW must be included before any OpenGL or GLFW header or the compiler errors with "gl.h included before glew.h". The chain is: `*.cpp → gl_common.h` (which includes `<GL/glew.h>`, `<GLFW/glfw3.h>`, `<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`, `<glm/gtc/type_ptr.hpp>`).

---

## Makefile

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Isrc
LDFLAGS  = -lGL -lGLEW -lglfw -lm -ldl
TARGET   = scene

SRCS = src/main.cpp src/globals.cpp src/shader.cpp src/texture.cpp \
       src/mesh.cpp src/world.cpp src/input.cpp src/camera.cpp \
       src/update.cpp src/render.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
run: $(TARGET)
	./$(TARGET)
clean:
	rm -f $(TARGET) src/*.o
.PHONY: all clean run
```

---

## Key Controls

| Key | Action |
|-----|--------|
| `↑` / `f` | Accelerate car |
| `↓` / `s` | Decelerate car |
| `←` / `l` | Steer left |
| `→` / `r` | Steer right |
| `a` | Swivel ground camera left (view 3 only) |
| `d` | Swivel ground camera right (view 3 only) |
| `w` | Increase fan/windmill speed |
| `Shift+W` | Decrease fan/windmill speed |
| `h` | Toggle car headlights |
| `1` | Camera: Sky view (top-down) |
| `2` | Camera: Car roof view |
| `3` | Camera: Ground view (near building 0) |
| `4` | Camera: Light-source view (rides gimbal 0) |
| `5` | Camera: Helicopter view (behind car) |
| `b` | Toggle bullet-time (slow-motion, factor 0.1×) |
| `x` | Reset world to initial state |
| `Escape` | Quit |

**Note:** Arrow keys were originally used for camera swivel in the starter code. They were **remapped** — arrows now steer/accelerate the car; `a`/`d` handle ground camera swivel.

---

## constants.h — All Tunable Values

```cpp
// Colosseum
COL_R_OUT = 30.0f   // outer wall radius
COL_R_IN  = 27.0f   // inner wall radius
COL_H     = 10.0f   // total wall height
COL_TIERS = 3.0f    // seating tiers
COL_SEG   = 64      // cylinder tessellation

// Outer elliptical track centre-line
TRK_A_OUT = 22.0f, TRK_B_OUT = 17.0f

// Inner elliptical track centre-line
TRK_A_IN  = 9.0f,  TRK_B_IN  = 6.5f

ROAD_W    = 3.5f    // road ring width (both tracks)
TRK_SEG   = 128     // road mesh tessellation
ARENA_R   = 26.0f   // ground disk radius

// Buildings
NUM_B     = 5
B_HALF    = 1.25f   // footprint half-size
STORY_H   = 2.5f    // height per storey

// Torches
NUM_TORCH = 16      // evenly spaced on inner colosseum rim
TORCH_R   = 25.5f   // placement radius
TORCH_H   = 0.5f    // pole height above colosseum top
FLAME_R   = 0.12f   // flame sphere base radius

NUM_TREE  = 20      // scattered in median strip

// Car
CAR_L=2.4, CAR_W=1.2, CAR_BH=0.40, CAR_CH=0.35
WHL_R=0.18, WHL_W=0.12
SPD_INC=0.5, STR_DEG=2.5 (degrees), MAX_SPD=15.0

// Fan
FAN_R=0.65, FAN_BW=0.13, FAN_BT=0.03
FAN_BSPD=90.0 (deg/s), FAN_SINC=15.0

// Gimbal spotlight
LT_ARM=1.2, LT_BULB_R=0.12
SW_SPD=1.5 (rad/s), SW_MAX=30 (degrees)

// Lighting slot layout (MAX_LIGHTS=10)
LIGHT_GIMBAL_0  = 0   // slots 0-4: building gimbals
LIGHT_HEADLIGHT = 5   // slots 5-6: car headlights
LIGHT_TORCH_0   = 7   // slots 7-9: 3 nearest torches

// Time of day
NUM_PERIODS = 4
PERIOD_DUR  = 30.0f  // seconds per period
PERIOD_FADE = 5.0f   // cross-fade seconds at boundaries

// Camera
CAM_NEAR=0.1, CAM_FAR=300.0, CAM_FOV=60.0
GND_SWIV=30.0 (degrees max swivel)

BULLET_F = 0.1f   // slow-motion time scale
```

---

## Global State (globals.h / globals.cpp)

### Uniform location caches
All GLSL uniform locations are cached once at startup via `cacheUniforms()` in `shader.cpp`. Arrays are sized `MAX_LIGHTS=10`.

```cpp
GLint uModel, uView, uProj, uNormMat;
GLint uObjCol, uUseTex, uTexScale, uSpecCol, uShine, uAmbi, uEmissive;
GLint uViewPos, uSunDir, uSunCol, uSunStr;
GLint uNumLights;
GLint uLightPos[10], uLightCol[10], uLightDir[10], uLightCut[10], uLightStr[10];
GLint uFogCol, uFogDen;
```

### Meshes
```cpp
Mesh mBox, mCylinder, mSphere, mRoad, mRoadInner, mGround;
```
`mCylinder` is created via `createCylinderCapped(24)` (has caps). `createCylinder()` (open shell, no caps) is used internally within render.cpp for tree trunks and statue limbs.

### Textures
```cpp
GLuint texBrick, texWood, texConcrete, texStone, texCobble;
```
All procedurally generated (256×256 RGB) at startup. No external image files needed.

### Building data
```cpp
BuildingInfo bldg[5] = {
    // pos              stories  texType  lightCol
    { { 0.0, 0,  0.0},    3,      0,   {1.0, 0.55, 0.10} }, // centre, amber
    { {-5.5, 0, -4.0},    2,      1,   {0.3, 0.80, 1.00} }, // NW, blue
    { { 5.5, 0, -4.0},    2,      2,   {0.8, 0.25, 0.25} }, // NE, red
    { {-5.0, 0,  5.0},    1,      0,   {0.3, 1.00, 0.45} }, // SW, green
    { { 5.0, 0,  5.0},    1,      1,   {0.9, 0.90, 0.20} }, // SE, yellow
};
// texType: 0=brick, 1=wood, 2=concrete
// Computed at initGL(): nearRoad, toRoad, roadYaw
```

Buildings are positioned in the **central island** (inside the inner track). Each building's gimbal targets the **outer track** (via `nearestTrack()`).

### Car state
```cpp
glm::vec3 carPos;         // starts at (TRK_A_OUT, 0, 0) = (22, 0, 0)
float carHeading = 0;     // radians; 0 = +Z direction
float carSpeed   = 0;     // m/s; negative = reverse
bool  carFrozen  = false; // set true on collision
float wheelRot   = 0;     // visual wheel spin angle
bool  headlightsOn = true;
```

### Time-of-day state
```cpp
int   todPeriod;   // 0=early morning, 1=noon, 2=evening, 3=night
float todFrac;     // 0.0..1.0 progress within current period
float lightsOn;    // 0.0=off, 1.0=fully on; smoothly interpolated
                   // Lights on during periods 2 and 3
```

### Torch positions
```cpp
glm::vec3 torchWorldPos[16];  // pre-computed at initGL()
// formula: (TORCH_R * cos(2π*i/16), COL_H + TORCH_H, TORCH_R * sin(2π*i/16))
```

### Gimbal state (updated every frame in update.cpp)
```cpp
glm::vec3 spotPos[5];        // world position of each building's bulb
glm::vec3 spotDir[5];        // spotlight direction (unit vector)
glm::mat4 spotGimbalMat[5];  // full 4×4 gimbal transform (used by camera view 4)
```

---

## Shader Architecture (fragment.glsl)

### Vertex layout (per vertex, 8 floats)
`pos(3) | normal(3) | uv(2)` — attribute locations 0, 1, 2 respectively.

### Fragment shader uniforms

**Material:**
- `vec3 objectColor` — base albedo when texture off
- `bool useTexture` — switch
- `sampler2D diffuseTexture` — always unit 0
- `vec2 texScale` — UV tiling multiplier
- `vec3 specularColor`, `float shininess` — Blinn-Phong
- `float ambientStrength`
- `vec3 emissiveColor` — self-glow (flame, bulbs, headlights)

**Sun (directional):**
- `vec3 sunDir` — direction FROM sun (points toward scene)
- `vec3 sunColor`, `float sunStrength`

**Point/Spot lights (10 slots):**
- `int numLights` — always set to MAX_LIGHTS=10
- `vec3 lightPos[10]`, `vec3 lightColor[10]`
- `vec3 lightDirection[10]` — spot cone axis
- `float lightCutoff[10]` — cos(half-angle); **≥1.0 = omnidirectional point light**
- `float lightStrength[10]` — per-light intensity multiplier

**Atmosphere:**
- `vec3 fogColor`, `float fogDensity` — exponential fog
- Reinhard tone-mapping applied at end

### Light slot semantics
| Slots | Type | When active |
|-------|------|-------------|
| 0–4 | Building gimbal spotlights (cos 35° cone) | `lightsOn * 2.5` strength |
| 5–6 | Car headlights (cos 22° cone, tilted -18° down) | `headlightsOn ? 6.0 : 0.0` |
| 7–9 | 3 nearest torches (cutoff=1.1 → point light) | `lightsOn * 3.5` strength |

Torches are sorted by distance to car position each frame; the 3 nearest get shader slots 7–9.

---

## Time-of-Day System (update.cpp)

Four discrete periods, each `PERIOD_DUR=30` seconds long, with `PERIOD_FADE=5` second cross-fades.

| Period | Name | Sky | Sun | Lights |
|--------|------|-----|-----|--------|
| 0 | Early morning | Pink-orange `(0.82, 0.58, 0.42)` | Low warm, strength 0.45 | OFF (fade OFF at start) |
| 1 | Noon | Deep blue `(0.42, 0.72, 0.95)` | Directly above, strength 1.0 | OFF |
| 2 | Evening | Amber `(0.80, 0.38, 0.15)` | Low from side, strength 0.50 | ON (fade ON at start) |
| 3 | Night | Dark `(0.04, 0.04, 0.12)` | None (strength 0.0) | ON |

`lightsOn` logic:
- Periods 0 and 1: `lightsOn = 0.0`
- Period 2 start: smoothly ramps from 0→1 over `PERIOD_FADE` seconds
- Period 3: `lightsOn = 1.0`
- Period 0 start: smoothly ramps from 1→0 over `PERIOD_FADE` seconds

`blendSky()` in render.cpp linearly interpolates all sky parameters between consecutive periods during the fade zone.

Bullet-time (`b` key) scales `dt` by `BULLET_F=0.1`, slowing all animation and physics including the time-of-day cycle.

---

## Mesh System (mesh.h / mesh.cpp)

All meshes use the same vertex layout: `pos(3) | normal(3) | uv(2)`.

| Function | Description |
|----------|-------------|
| `createBox()` | Unit box, 6 faces, 12 triangles |
| `createCylinder(slices)` | Open shell, no caps. Used for tree trunks, statue limbs, wheel cylinders |
| `createCylinderCapped(slices)` | With top+bottom caps. Used for mCylinder (the global mesh) |
| `createSphere(slices, stacks)` | UV sphere, radius 0.5. Used for wheels hubs, flame, bulbs, statue head |
| `createGround()` | Circular disk fan (80 triangles), radius=ARENA_R=26, UV tiled for cobble |
| `createColosseum()` | 3-tier hollow cylinder. Both inner+outer faces rendered (double-sided wall). Per-tier taper of 0.55 units inward per tier. UV tiled 12× horizontally |
| `createRoad()` | Outer elliptical ring (22×17). Width ROAD_W=3.5. UV tiled 6× along loop |
| `createRoadInner()` | Inner elliptical ring (9×6.5). Same width |
| `nearestTrack(p)` | Nearest point on outer track centre-line (360-sample brute force) |
| `nearestTrackInner(p)` | Same for inner track |

**Note:** `createColosseum()` is called with `static Mesh colMesh = createColosseum();` inside `render()` — it is constructed once on the first frame. This is intentional (the mesh is too large to be a named global without changing world.cpp). If refactoring, move it to a named global in `globals.h/.cpp` and call `createColosseum()` in `initGL()`.

---

## Texture System (texture.h / texture.cpp)

All textures are procedurally generated at 256×256, RGB, uploaded as mipmapped repeating GL textures.

| Function | Description | Used on |
|----------|-------------|---------|
| `genBrickTex` | Red brick with mortar lines | Buildings (texType=0) |
| `genWoodTex` | Wood grain via sine + noise | Buildings (texType=1) |
| `genConcreteTex` | Flat grey noise | Buildings (texType=2) |
| `genStoneTex` | Large sandstone blocks with mortar | Colosseum wall |
| `genCobbleTex` | Round cobbles with dark grout | Ground disk |

All use `hashNoise(int x, int y)` — a deterministic integer hash for noise variation.

---

## Render System (render.cpp)

### Material helpers (shader.cpp)

```cpp
void setModel(const glm::mat4& m);
// Sets uModel and computes+uploads the normal matrix (transpose(inverse(mat3(model))))

void setMaterial(vec3 col, vec3 spec, float shine, float ambi=0.15, vec3 emit=0);
// Sets objectColor, specularColor, shininess, ambientStrength, emissiveColor
// Sets useTexture=false

void setTexMaterial(GLuint tex, vec2 scale, vec3 spec, float shine);
// Binds tex to unit 0, sets useTexture=true, texScale, spec, shine, ambi=0.15, emit=0
```

### Matrix stack

A `std::stack<glm::mat4> matStack` is used for hierarchical transforms. Helpers:
```cpp
static void pushM(const glm::mat4& m)  // push new frame
static void popM()                      // pop
static glm::mat4& top()                 // current transform
```

### Draw order in render()

1. Compute `PeriodSky` via `blendSky(todPeriod, todFrac)` — interpolated sky/sun parameters
2. `glClearColor` + `glClear`
3. Upload sun uniforms (`uSunDir`, `uSunCol`, `uSunStr`)
4. Upload fog uniforms
5. Upload all 10 light slots via `setLight(slot, pos, col, dir, cutCos, strength)`
6. `getCamera()` — uploads view+projection, returns eye position
7. Reset matrix stack to identity
8. Draw cobblestone ground disk (cobble texture)
9. Draw central grass island (scaled ground disk)
10. Draw outer road + 2 kerb strips (scaled copies of mRoad, white/beige)
11. Draw inner road + 2 kerb strips
12. Draw sand/seating ring (scaled road mesh tinted sandy)
13. Draw colosseum wall (stone texture, static mesh)
14. Draw 24 stone pillars on inner rim
15. Draw 20 trees (deterministic positions from `rnd(i, seed)`)
16. Draw central warrior statue (pedestal + bronze figure with spear+shield)
17. Draw 16 fire torches (pole + bowl + flickering flame spheres, emissive scales with `lightsOn`)
18. Draw 5 buildings (textured body, pitched roof, fan, gimbal arm+bulb)
19. Draw car (metallic red body, cabin, chrome bumpers, emissive headlights+taillights, black wheels, silver alloy hubs)

### Ambient boost at night
```cpp
glUniform1f(uAmbi, sky.ambStr + lightsOn * 0.12f);
```
Applied to the ground so the cobblestones don't go pitch black at night.

---

## Camera System (camera.cpp)

All views upload `uView`, `uProj`, `uViewPos` before returning.

| Mode | Key | Description |
|------|-----|-------------|
| 0 | `1` | Sky: eye=(0,50,0), look down, up=(0,0,-1) |
| 1 | `2` | Car roof: eye = carPos + (0, roofY, 0) + fwd*0.3 |
| 2 | `3` | Ground: stationary 3m above building 0's road-facing wall. Swivels by `gndSwivel` degrees via `a`/`d` keys |
| 3 | `4` | Light-source: eye=`spotPos[0]`, looks along `spotDir[0]`. Up-vector is column 1 of `spotGimbalMat[0]` to avoid degenerate lookAt |
| 4 | `5` | Helicopter: eye = carPos - fwd*8 + (0,5,0), looks at carPos+(0,1,0) |

**Gimbal camera bug fix:** The original implementation used `glm::lookAt(eye, eye+spotDir, {0,1,0})`. When the spotlight pitches nearly straight down, `spotDir ≈ (0,-1,0)` is nearly parallel to the world-up vector, causing `lookAt` to produce a degenerate (rolling/flipping) matrix. Fix: `update.cpp` stores `spotGimbalMat[i]` (the full 4×4 gimbal transform) each frame. `camera.cpp` extracts `glm::vec3(spotGimbalMat[0][1])` as the camera up-vector — this is the gimbal's local Y axis, always perpendicular to `spotDir`.

---

## Physics and Animation (update.cpp)

Each frame receives `dt` (seconds since last frame, clamped to 0.1s in main.cpp). If `bulletTime`, `eff = dt * 0.1`. Otherwise `eff = dt`.

**Car movement:**
- `fwd = (sin(carHeading), 0, cos(carHeading))`
- `next = carPos + fwd * carSpeed * eff`
- If `checkCollision(next, heading)` → `carFrozen=true`, `carSpeed=0`
- Else `carPos = next`
- `wheelRot += carSpeed * eff / WHL_R` (visual spin)

**Collision detection (world.cpp):**
- Computes 4 ground-plane corners of the car OBB
- Checks each corner: `sqrt(cx²+cz²) > COL_R_IN - 1.0` → wall hit
- Checks each corner against each building's AABB (`±B_HALF`)
- `carFrozen=true` on first collision; reset with `x`

**Gimbal update:** For each building `i`:
```
mount = bldg[i].pos + (0, bldgH+0.3, 0)
toRd  = bldg[i].nearRoad - mount
baseYaw = atan2(toRd.x, toRd.z)
pitch   = atan2(-toRd.y, sqrt(toRd.x²+toRd.z²))
swing   = radians(SW_MAX) * sin(globalTime * SW_SPD + i)
M = translate(mount) * rotateY(baseYaw+swing) * rotateX(pitch)
spotGimbalMat[i] = M
spotPos[i] = M * (0, 0, LT_ARM, 1)
spotDir[i] = normalize(M * (0, 0, 1, 0))
```

**Fan update:** `fanAngle += radians(fanSpeed) * eff`

---

## Car Geometry Detail

The car is drawn in a local frame (`carPos`, rotated by `carHeading`):

- **Body:** box `(CAR_W × CAR_BH × CAR_L)`, centre at `y = WHL_R + CAR_BH/2`. Metallic red `(0.72, 0.06, 0.06)`, high shininess 160, spec `(0.90, 0.80, 0.80)`.
- **Cabin:** box `(CAR_W*0.88 × CAR_CH × CAR_L*0.5)` on top of body. Darker red.
- **Headlights:** two small white boxes at front corners. Emissive `(1.8, 1.7, 1.2)` when `headlightsOn`, dim otherwise. Chrome housing rings behind them.
- **Tail-lights:** two red boxes at rear. Emissive scales with `lightsOn` (dim in day, bright at night).
- **Bumpers:** thin chrome strips front and back.
- **Wheels:** 4× capped cylinders (black rubber), rotated 90° to be sideways, spin with `wheelRot`.
- **Alloy hubs:** silver spheres at wheel centres.

**Headlight as actual light sources (slots 5–6):**
- Position: front bumper corners `carPos + fwd*CAR_L*0.52 ± right*CAR_W*0.35 + (0, WHL_R+CAR_BH*0.4, 0)`
- Direction: `normalize(fwd*1.0 + (0, -0.18, 0))` — forward and slightly down to hit road
- Cone: cos(22°) — wide enough to illuminate road ahead
- Strength: 6.0 when `headlightsOn`, 0 otherwise
- Colour: warm white `(1.0, 0.97, 0.85)`

---

## Central Statue

Replaces the "green bush" from the sketch. A Roman warrior statue in bronze (green patina):
- Stone pedestal (box) + step slab
- Torso (box), head (sphere), helmet crest (red box)
- Left arm raised (cylinder, -55° tilt) + spear shaft (tall thin cylinder) + spear tip (sphere)
- Right arm down (cylinder, +20° tilt) + shield (box)
- Two legs (cylinders)
- Material: `(0.35, 0.60, 0.42)` bronze-green with spec `(0.55, 0.80, 0.60)`, shininess 80

---

## Fire Torches

16 torches placed at radius `TORCH_R=25.5` on the inner colosseum rim, at height `COL_H=10` (top of the wall).

Each torch:
- Stone pole: thin capped cylinder
- Bowl: flat capped cylinder
- Flame: sphere scaled by `FLAME_R * (0.4 + 0.6*lightsOn) * flicker`
- Secondary wisp: smaller sphere slightly offset (animated)
- `flicker = 0.88 + 0.12*sin(globalTime*8.7 + 0.5*cos(globalTime*3.1))`
- Emissive: `mix((0.02,0.01,0.01), (2.80,0.85,0.08), lightsOn)`
- Colour: `mix((0.28,0.22,0.18), (1.0,0.44,0.04), lightsOn)`

Torches are also actual light sources in slots 7–9 (3 nearest to car), as orange point lights.

---

## Building Windmill / Fan

Each building has a fan on its road-facing wall near the top:
- Hub: small sphere
- 4 rectangular blades (`FAN_BW × FAN_R × FAN_BT`) rotated 90° apart
- Transform chain: `translate(fanOff) → rotateY(roadYaw) → rotateZ(fanAngle)`
- `roadYaw = atan2(toRoad.x, toRoad.z)` so blades face the road
- `fanAngle` accumulates at `fanSpeed` deg/s (default 90 deg/s)

---

## Known Design Decisions and Non-Obvious Choices

1. **`createColosseum()` as a static local in render():** The colosseum mesh is complex and is initialized once on first render call with `static Mesh colMesh`. If you want to move it to a global, add `Mesh mColosseum;` to `globals.h`, call `mColosseum = createColosseum();` in `initGL()`, and replace `colMesh` with `mColosseum` in render.cpp.

2. **Building gimbals target outer track:** All 5 buildings are in the central island. Their gimbals are aimed at `nearestTrack(bldg[i].pos)` (outer ellipse 22×17), not the inner track, so the light beams sweep across the outer driving lane — the more dramatic visual.

3. **Collision uses circular boundary:** The rectangular wall was replaced with a circular check (`r > COL_R_IN - 1.0`). The `1.0` margin accounts for the car's half-width so the car stops before visually penetrating the colosseum wall.

4. **Shader `numLights` is always MAX_LIGHTS=10:** Zero-strength lights (slots with `strength=0`) contribute nothing to the scene but the loop still runs. This avoids branching on light count. If performance is a concern, reduce active slots at night vs day.

5. **Trees use deterministic `rnd(i, seed)` scatter:** No `std::rand()` or `srand()`. Same tree positions every run, same between sessions. The `rnd()` function is a Xorshift32 variant seeded with `i` and a constant.

6. **`nearestTrack()` uses 360-sample brute force:** O(360) per building (computed once at initGL). Fine for 5 buildings. If increasing NUM_B significantly, replace with Newton-Raphson ellipse projection.

7. **Ground disk UVs use polar coordinates scaled by `ARENA_R/8.0`:** This tiles the cobble texture roughly 3× across the radius, giving visible stone pattern at normal viewing distances.

8. **The `uAmbi` uniform is set per-draw-group**, not once per frame. The ground gets `sky.ambStr + lightsOn*0.12` (extra ambient at night); the colosseum wall gets `sky.ambStr + 0.05` (slightly brighter than average to show the stone detail); the statue gets `sky.ambStr + 0.08`. Most other objects use `sky.ambStr` directly.

---

## Things Not Yet Implemented (Potential Extensions)

- Lane markings on the road (could be done with a second UV-based alpha pass or decal mesh)
- Shadow mapping
- The assignment bonus: "bullet time" is implemented (slowdown), but "showcase" quality is not specified
- Street lights between the two tracks
- Spectator crowd in the colosseum seating area
- The inner track is not directly drivable from the start position — the car spawns on the outer track. There's no mechanism to switch tracks.
- ARENA_R=26 is slightly smaller than COL_R_IN=27, so the ground disk doesn't quite reach the colosseum wall — a minor gap. Fix: set `ARENA_R = COL_R_IN - 0.2`.
- The `HEADLIGHT_KEY = 0` in constants.h is a placeholder; the actual key is `GLFW_KEY_H` hard-coded in input.cpp.

---

## Shader Fallback

Both `shader.cpp`'s `VS_FALLBACK` and `FS_FALLBACK` strings are embedded in the binary. If `vertex.glsl` or `fragment.glsl` are missing from the working directory, the embedded fallbacks are used. However, the embedded FS fallback is an older version (pre-MAX_LIGHTS=10). **Always ensure `vertex.glsl` and `fragment.glsl` are present in the same directory as the `scene` binary when running.**

---

## How to Extend

### Adding a new mesh type
1. Declare it in `mesh.h`
2. Implement in `mesh.cpp`
3. Add a `Mesh m<Name>;` global in `globals.h` and `globals.cpp`
4. Call `m<Name> = create<Name>();` in `world.cpp::initGL()`
5. Draw via `m<Name>.draw()` in render.cpp

### Adding a new texture
1. Add `void genXTex(unsigned char*, int, int)` to `texture.h/.cpp`
2. Add `GLuint texX = 0;` to `globals.h/.cpp`
3. In `initGL()`: `genXTex(buf.data(), TW, TH); texX = uploadTex(buf.data(), TW, TH);`

### Adding a new light slot
Currently slots 0–9 are all used. To add more: change `MAX_LIGHTS` in both `constants.h` and `fragment.glsl` simultaneously. Add corresponding array entries to `globals.h/.cpp`.

### Changing time-of-day durations
Change `PERIOD_DUR` in `constants.h`. The full cycle = `PERIOD_DUR * NUM_PERIODS` seconds. `PERIOD_FADE` must be ≤ `PERIOD_DUR`.

### Changing building positions
Edit the `bldg[]` array in `globals.cpp`. Buildings must stay within the inner track boundary (radius < `TRK_A_IN - ROAD_W/2 - B_HALF ≈ 6.5`) or outside the outer track (radius > `TRK_A_OUT + ROAD_W/2 + B_HALF ≈ 25`). Gimbal targets are recomputed automatically at `initGL()`.