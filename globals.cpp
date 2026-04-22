/* =====================================================================
 *  globals.cpp — Definitions of all global mutable state
 * ===================================================================== */
#include "gl_common.h"
#include "globals.h"

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

BuildingInfo bldg[NUM_B] = {
    { {-3.6f, 0, -1.4f}, 3, 0, {1.0f,0.55f,0.10f}, {},{},0.0f },
    { { 4.2f, 0, -2.0f}, 2, 1, {0.3f,0.8f, 1.0f }, {},{},0.0f },
    { {-2.9f, 0,  3.8f}, 2, 2, {0.8f,0.25f,0.25f}, {},{},0.0f },
    { { 3.4f, 0,  3.2f}, 1, 0, {0.3f,1.0f, 0.45f}, {},{},0.0f },
    { { 0.6f, 0, -4.4f}, 1, 1, {0.9f,0.9f, 0.2f }, {},{},0.0f },
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

bool bulletTime  = false;

std::stack<glm::mat4> matStack;

glm::vec3 spotPos[NUM_B];
glm::vec3 spotDir[NUM_B];
glm::mat4 spotGimbalMat[NUM_B];
