/* =====================================================================
 *  globals.cpp — Definitions of all global mutable state
 * ===================================================================== */
#include "gl_common.h"
#include "globals.h"
#include <string>

int WIN_W = 1280;
int WIN_H = 720;
const char* WIN_TITLE = "Colosseum Scene — CG Assignment 3";

GLuint shaderProg = 0;

GLint uModel, uView, uProj, uNormMat;
GLint uObjCol, uUseTex, uTexScale, uSpecCol, uShine, uAmbi, uEmissive;
GLint uViewPos, uSunDir, uSunCol, uSunStr;
GLint uNumLights;
GLint uLightPos[MAX_LIGHTS];
GLint uLightCol[MAX_LIGHTS];
GLint uLightDir[MAX_LIGHTS];
GLint uLightCut[MAX_LIGHTS];
GLint uLightStr[MAX_LIGHTS];
GLint uFogCol, uFogDen;

Mesh mBox, mCylinder, mSphere, mRoad, mRoadInner, mGround;

GLuint texBrick=0, texWood=0, texConcrete=0, texStone=0, texCobble=0, texTorch=0, texGate=0;
GLuint texMud=0, texGatehouse=0;

/* Buildings form a circle at radius ~13.5 (halfway between centre and
   colosseum wall at r=27).  Angles are roughly 72° apart.
   Modify the .pos field to relocate any building.
   texType 0 = closed facade (building_view_closed.png).
   texType 1 = gate facade (building_with_gate.jpeg). */
BuildingInfo bldg[NUM_B] = {
    { { 13.0f, 0,   0.0f}, 5, 0, {1.00f,0.55f,0.18f}, {},{},0.0f },  /* 0°   closed */
    { {  4.0f, 0,  12.4f}, 4, 0, {0.22f,0.78f,1.00f}, {},{},0.0f },  /* 72°  closed */
    { {-10.5f, 0,   7.6f}, 5, 1, {0.80f,0.25f,0.95f}, {},{},0.0f },  /* 144° gate   */
    { {-10.5f, 0,  -7.6f}, 4, 1, {0.55f,1.00f,0.30f}, {},{},0.0f },  /* 216° gate   */
    { {  4.0f, 0, -12.4f}, 4, 0, {0.90f,0.90f,0.20f}, {},{},0.0f },  /* 288° closed */
};

glm::vec3 carPos;
float     carHeading  = 0.0f;
float     carSpeed    = 0.0f;
bool      carFrozen   = false;
float     wheelRot    = 0.0f;
bool      headlightsOn= true;
bool      useChariot  = false;

float fanAngle   = 0.0f;
float fanSpeed   = FAN_BSPD;
float globalTime = 0.0f;

int   todPeriod  = 1;     /* start at noon */
float todFrac    = 0.0f;
float lightsOn   = 0.0f;

glm::vec3 torchWorldPos[NUM_TORCH];

int   camMode    = 0;
float gndSwivel  = 0.0f;
glm::vec2 freeCamCenterXZ(0.0f, 0.0f);
float     freeCamZoom     = 50.0f;
float     freeCamYawDeg   = 45.0f;
float     freeCamPitchDeg = 50.0f;
bool      freeCamDragging = false;
double    freeCamLastX    = 0.0;
double    freeCamLastY    = 0.0;

bool bulletTime  = false;

bool        cheatMode   = false;
std::string cheatBuffer;
bool        superMode        = false;
bool        showBoundingBox  = false;

std::stack<glm::mat4> matStack;

glm::vec3 spotPos[NUM_B];
glm::vec3 spotDir[NUM_B];
glm::mat4 spotGimbalMat[NUM_B];
