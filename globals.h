#pragma once
/* =====================================================================
 *  globals.h — Declarations for all global mutable state.
 * ===================================================================== */
#include "types.h"
#include "constants.h"
#include <stack>
#include <string>

/* ── Window ── */
extern int WIN_W;
extern int WIN_H;
extern const char* WIN_TITLE;

/* ── Shader programme ── */
extern GLuint shaderProg;

/* ── Cached uniform locations ── */
extern GLint uModel, uView, uProj, uNormMat;
extern GLint uObjCol, uUseTex, uTexScale, uSpecCol, uShine, uAmbi, uEmissive;
extern GLint uViewPos, uSunDir, uSunCol, uSunStr;
extern GLint uNumLights;
extern GLint uLightPos     [MAX_LIGHTS];
extern GLint uLightCol     [MAX_LIGHTS];
extern GLint uLightDir     [MAX_LIGHTS];
extern GLint uLightCut     [MAX_LIGHTS];
extern GLint uLightStr     [MAX_LIGHTS];   /* NEW: per-light strength */
extern GLint uFogCol, uFogDen;

/* ── Meshes ── */
extern Mesh mBox, mCylinder, mSphere, mRoad, mRoadInner, mGround;

/* ── Textures ── */
extern GLuint texBrick, texWood, texConcrete, texStone, texCobble, texTorch, texGate;
extern GLuint texMud, texGatehouse;

/* ── Building data ── */
extern BuildingInfo bldg[NUM_B];

/* ── Car state ── */
extern glm::vec3 carPos;
extern float     carHeading;
extern float     carSpeed;
extern bool      carFrozen;
extern float     wheelRot;
extern bool      headlightsOn;          /* NEW: toggleable headlights */
extern bool      useChariot;            /* false = sports car, true = chariot */

/* ── Animation state ── */
extern float fanAngle;
extern float fanSpeed;
extern float globalTime;               /* wall-clock accumulator (seconds) */

/* ── Time of day ──
   period: 0=early morning, 1=noon, 2=evening, 3=night
   periodFrac: 0..1 progress within the current period (for cross-fade) */
extern int   todPeriod;
extern float todFrac;
extern float lightsOn;                 /* 0=off, 1=on — driven by period   */

/* ── Torch world positions (set once at startup) ── */
extern glm::vec3 torchWorldPos[NUM_TORCH];

/* ── Camera ── */
extern int   camMode;
extern float gndSwivel;

/* ── Bonus ── */
extern bool bulletTime;

/* ── Cheat code system ── */
extern bool        cheatMode;
extern std::string cheatBuffer;

/* ── Cheat-activated flags ── */
extern bool superMode;        /* disable building collision */
extern bool showBoundingBox;  /* show car bounding box in blue */

/* ── Matrix stack ── */
extern std::stack<glm::mat4> matStack;

/* ── Spotlight world positions & directions (updated each frame) ── */
extern glm::vec3 spotPos[NUM_B];
extern glm::vec3 spotDir[NUM_B];
extern glm::mat4 spotGimbalMat[NUM_B];
