/* =====================================================================
 *  world.cpp — World initialisation, reset, and collision detection
 * ===================================================================== */
#include "gl_common.h"
#include "world.h"
#include "globals.h"
#include "constants.h"
#include "shader.h"
#include "texture.h"
#include "mesh.h"

#include <vector>
#include <cmath>
#include <iostream>

bool initGL() {
    std::string vSrc = readFile("vertex.glsl");
    std::string fSrc = readFile("fragment.glsl");
    if (vSrc.empty()) vSrc = VS_FALLBACK;
    if (fSrc.empty()) fSrc = FS_FALLBACK;

    shaderProg = buildProgram(vSrc.c_str(), fSrc.c_str());
    if (!shaderProg) return false;
    glUseProgram(shaderProg);
    cacheUniforms();
    glUniform1i(glGetUniformLocation(shaderProg, "diffuseTexture"), 0);

    /* Image-first textures (assignment assets) with procedural fallback */
    texBrick      = loadImageTex("images/building_view_closed.png");
    texWood       = loadImageTex("images/textures.png");
    texConcrete   = loadImageTex("images/building_with_gate.jpeg");
    texStone      = loadImageTex("images/colosseum_inner_wall.png");
    texCobble     = loadImageTex("images/ground_view.png");
    texTorch      = loadImageTex("images/burning_torch.png");
    texGate       = loadImageTex("images/building_with_gate.jpeg");
    texMud        = loadImageTex("images/mud_texture.jpg");
    texGatehouse  = loadImageTex("images/gatehouse.jpg");

    const int TW = 256, TH = 256;
    std::vector<unsigned char> buf(TW * TH * 3);
    if (!texBrick) {
        genBrickTex(buf.data(), TW, TH);
        texBrick = uploadTex(buf.data(), TW, TH);
    }
    if (!texWood) {
        genWoodTex(buf.data(), TW, TH);
        texWood = uploadTex(buf.data(), TW, TH);
    }
    if (!texConcrete) {
        genConcreteTex(buf.data(), TW, TH);
        texConcrete = uploadTex(buf.data(), TW, TH);
    }
    if (!texStone) {
        genStoneTex(buf.data(), TW, TH);
        texStone = uploadTex(buf.data(), TW, TH);
    }
    if (!texCobble) {
        genCobbleTex(buf.data(), TW, TH);
        texCobble = uploadTex(buf.data(), TW, TH);
    }
    if (!texTorch)    texTorch    = texWood;
    if (!texGate)     texGate     = texConcrete;
    /* Mud: procedural fallback — warm dark brown */
    if (!texMud) {
        for (int i = 0; i < TW * TH; i++) {
            unsigned n = (unsigned)(i * 1664525u + 1013904223u);
            n ^= (n << 13); n ^= (n >> 17); n ^= (n << 5);
            float v = (float)(n & 0xffu) / 255.0f;
            buf[i*3+0] = (unsigned char)(90  + v * 30);
            buf[i*3+1] = (unsigned char)(58  + v * 20);
            buf[i*3+2] = (unsigned char)(32  + v * 12);
        }
        texMud = uploadTex(buf.data(), TW, TH);
    }
    if (!texGatehouse) texGatehouse = texBrick;

    /* Meshes */
    mBox        = createBox();
    mCylinder   = createCylinderCapped(24);
    mSphere     = createSphere(20, 14);
    mGround     = createGround();
    mRoad       = createRoad();
    mRoadInner  = createRoadInner();

    /* Pre-compute torch world positions (constant, on colosseum inner rim) */
    for (int i = 0; i < NUM_TORCH; i++) {
        float ang = 2.0f * (float)M_PI * i / NUM_TORCH;
        torchWorldPos[i] = glm::vec3(
            TORCH_R * cosf(ang),
            COL_H + TORCH_H,          /* top of pole */
            TORCH_R * sinf(ang));
    }

    /* Pre-compute building geometry helpers.
       Gimbal base yaw tracks nearest-road orientation; final beam aim is updated per frame. */
    for (int i=0;i<NUM_B;i++) {
        bldg[i].nearRoad = nearestTrack(bldg[i].pos);
        glm::vec3 diff   = bldg[i].nearRoad - bldg[i].pos;
        diff.y = 0;
        bldg[i].toRoad   = glm::length(diff) > 0.001f ? glm::normalize(diff) : glm::vec3(1,0,0);
        bldg[i].roadYaw  = atan2f(bldg[i].toRoad.x, bldg[i].toRoad.z);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    return true;
}

void resetWorld() {
    /* Place car on the outer track at angle 0 (rightmost point), heading tangentially */
    carPos     = glm::vec3(TRK_A_OUT, 0.0f, 0.0f);
    carHeading = (float)M_PI / 2.0f;
    carSpeed   = 0.0f;
    carFrozen  = false;
    wheelRot   = 0.0f;
    headlightsOn = true;
    fanAngle   = 0.0f;
    fanSpeed   = FAN_BSPD;
    globalTime = 0.0f;
    camMode    = 0;
    gndSwivel  = 0.0f;
    bulletTime      = false;
    useChariot      = false;
    cheatMode       = false;
    cheatBuffer.clear();
    superMode       = false;
    showBoundingBox = false;
}

bool checkCollision(glm::vec3 pos, float heading) {
    float hl=CAR_L*0.5f, hw=CAR_W*0.5f;
    float ch=cosf(heading), sh=sinf(heading);
    glm::vec2 corners[4] = {
        {pos.x+ch*(-hw)+sh*(-hl), pos.z-sh*(-hw)+ch*(-hl)},
        {pos.x+ch*( hw)+sh*(-hl), pos.z-sh*( hw)+ch*(-hl)},
        {pos.x+ch*(-hw)+sh*( hl), pos.z-sh*(-hw)+ch*( hl)},
        {pos.x+ch*( hw)+sh*( hl), pos.z-sh*( hw)+ch*( hl)},
    };
    for (int c=0;c<4;c++) {
        float cx=corners[c].x, cz=corners[c].y;
        /* Colosseum inner wall (circular) */
        float r=sqrtf(cx*cx+cz*cz);
        if (r > COL_R_IN - 1.0f) return true;
        /* Buildings — skipped in superMode.
           Gate buildings keep a centered pass-through opening. */
        if (!superMode) {
            for (int b=0;b<NUM_B;b++) {
                float bx=bldg[b].pos.x, bz=bldg[b].pos.z;
                if (cx>bx-B_HALF&&cx<bx+B_HALF&&cz>bz-B_HALF&&cz<bz+B_HALF) {
                    bool hasGate = (bldg[b].texType == 1);
                    if (hasGate) {
                        float dx = cx - bx;
                        float dz = cz - bz;
                        bool gateAlongX = fabsf(bldg[b].toRoad.x) > fabsf(bldg[b].toRoad.z);
                        float gateHalf = B_GATE_HALF - 0.05f;
                        bool inOpening = gateAlongX ? (fabsf(dz) <= gateHalf)
                                                    : (fabsf(dx) <= gateHalf);
                        if (inOpening) continue;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}
