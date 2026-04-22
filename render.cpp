/* =====================================================================
 *  render.cpp — Colosseum scene rendering
 *
 *  Light slot layout (MAX_LIGHTS = 10):
 *    0-4  : building gimbal spotlights
 *    5-6  : car headlights (two cones, toggleable)
 *    7-9  : 3 nearest torches (wide point lights, night only)
 *
 *  Time-of-day sky palette:
 *    0 Early morning — pink-orange horizon, soft warm sun
 *    1 Noon          — deep blue sky, white sun high
 *    2 Evening       — amber/purple dusk, low orange sun
 *    3 Night         — dark blue-black, no sun, torch glow
 * ===================================================================== */
#include "gl_common.h"
#include "render.h"
#include "globals.h"
#include "constants.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"

#include <cmath>
#include <algorithm>
#include <array>

/* ── Matrix stack helpers ── */
static void pushM(const glm::mat4& m) { matStack.push(m); }
static void popM()                     { matStack.pop();   }
static glm::mat4& top()               { return matStack.top(); }

/* ── Deterministic scatter RNG ── */
static float rnd(int a, int b) {
    unsigned n = (unsigned)(a*1664525 + b*1013904223 + 123456789);
    n ^= (n<<13); n ^= (n>>17); n ^= (n<<5);
    return (float)(n & 0x7fffffffu) / 2147483647.0f;
}

/* ── Upload a single light slot ── */
static void setLight(int slot,
                     glm::vec3 pos, glm::vec3 col, glm::vec3 dir,
                     float cutCos,  float strength)
{
    glUniform3fv(uLightPos[slot], 1, glm::value_ptr(pos));
    glUniform3fv(uLightCol[slot], 1, glm::value_ptr(col));
    glUniform3fv(uLightDir[slot], 1, glm::value_ptr(dir));
    glUniform1f (uLightCut[slot], cutCos);
    glUniform1f (uLightStr[slot], strength);
}

/* =====================================================================
 *  Sky / sun parameters per period
 * ===================================================================== */
struct PeriodSky {
    glm::vec3 skyCol;    /* clear-sky colour                 */
    glm::vec3 sunCol;    /* sun/ambient colour               */
    glm::vec3 sunDir;    /* normalised direction FROM sun    */
    float     sunStr;    /* diffuse sun strength (0=no sun)  */
    float     ambStr;    /* ambient fraction                 */
    glm::vec3 fogCol;    /* fog colour                       */
    float     fogDen;    /* fog density                      */
};

static const PeriodSky PERIODS[4] = {
    /* 0 — Early morning */
    { {0.82f,0.58f,0.42f}, {1.0f,0.82f,0.62f},
      glm::normalize(glm::vec3(-0.8f,-0.3f, 0.5f)),
      0.45f, 0.25f,
      {0.72f,0.52f,0.38f}, 0.010f },
    /* 1 — Noon */
    { {0.42f,0.72f,0.95f}, {1.0f,0.97f,0.90f},
      glm::normalize(glm::vec3( 0.1f,-1.0f, 0.1f)),
      1.00f, 0.18f,
      {0.55f,0.78f,0.92f}, 0.005f },
    /* 2 — Evening */
    { {0.80f,0.38f,0.15f}, {1.0f,0.62f,0.28f},
      glm::normalize(glm::vec3( 0.9f,-0.2f,-0.4f)),
      0.50f, 0.20f,
      {0.70f,0.36f,0.18f}, 0.012f },
    /* 3 — Night */
    { {0.04f,0.04f,0.12f}, {0.15f,0.18f,0.30f},
      glm::normalize(glm::vec3( 0.0f,-1.0f, 0.0f)),
      0.00f, 0.06f,
      {0.04f,0.04f,0.10f}, 0.008f },
};

static PeriodSky blendSky(int p, float frac) {
    /* Blend from period p toward period (p+1)%4 over the fade zone */
    float fadeFrac = std::min(frac * PERIOD_DUR / PERIOD_FADE, 1.0f);
    const PeriodSky& A = PERIODS[p];
    const PeriodSky& B = PERIODS[(p+1) % NUM_PERIODS];
    float t = fadeFrac;
    PeriodSky r;
    r.skyCol = glm::mix(A.skyCol, B.skyCol, t);
    r.sunCol = glm::mix(A.sunCol, B.sunCol, t);
    r.sunDir = glm::normalize(glm::mix(A.sunDir, B.sunDir, t));
    r.sunStr = glm::mix(A.sunStr, B.sunStr, t);
    r.ambStr = glm::mix(A.ambStr, B.ambStr, t);
    r.fogCol = glm::mix(A.fogCol, B.fogCol, t);
    r.fogDen = glm::mix(A.fogDen, B.fogDen, t);
    return r;
}

/* =====================================================================
 *  render()
 * ===================================================================== */
void render() {
    /* ── Sky / sun for this frame ── */
    PeriodSky sky = blendSky(todPeriod, todFrac);

    glClearColor(sky.skyCol.r, sky.skyCol.g, sky.skyCol.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3fv(uSunDir, 1, glm::value_ptr(sky.sunDir));
    glUniform3fv(uSunCol, 1, glm::value_ptr(sky.sunCol));
    glUniform1f (uSunStr, sky.sunStr);
    glUniform3fv(uFogCol, 1, glm::value_ptr(sky.fogCol));
    glUniform1f (uFogDen, sky.fogDen);

    /* ── Light slot 0-4: building gimbal spotlights ──
       Strength scales with lightsOn so they fade in at evening. */
    for (int i = 0; i < NUM_B; i++) {
        setLight(i, spotPos[i], bldg[i].lightCol, spotDir[i],
                 cosf(glm::radians(35.0f)), lightsOn * 2.5f);
    }

    /* ── Slots 5-6: car headlights ──
       Two tight spot cones, pointing forward-and-down from the front bumper.
       Only active when headlightsOn AND (lights should be on at all, or
       player explicitly enabled them). */
    {
        glm::vec3 fwd (sinf(carHeading), 0.0f, cosf(carHeading));
        glm::vec3 right(-cosf(carHeading), 0.0f, sinf(carHeading));
        /* Headlight mounts: front bumper corners, just above ground */
        float hy    = WHL_R + CAR_BH * 0.4f;
        float hz    = CAR_L * 0.52f;
        glm::vec3 hl[2] = {
            carPos + fwd*hz + right*CAR_W*0.35f + glm::vec3(0,hy,0),
            carPos + fwd*hz - right*CAR_W*0.35f + glm::vec3(0,hy,0),
        };
        /* Cone axis: forward and slightly downward so it hits the road */
        glm::vec3 hlDir = glm::normalize(fwd * 1.0f + glm::vec3(0,-0.18f,0));
        float hlStr = (headlightsOn) ? 6.0f : 0.0f;
        /* Wide enough to illuminate the road ahead visibly */
        float hlCut = cosf(glm::radians(22.0f));
        glm::vec3 hlCol(1.0f, 0.97f, 0.85f);   /* warm white */
        setLight(LIGHT_HEADLIGHT+0, hl[0], hlCol, hlDir, hlCut, hlStr);
        setLight(LIGHT_HEADLIGHT+1, hl[1], hlCol, hlDir, hlCut, hlStr);
    }

    /* ── Slots 7-9: 3 nearest torches to camera (night only) ──
       Point lights (cutoff >= 1.0 = omnidirectional in shader). */
    {
        /* Sort torches by distance to car — proxy for what's most visible */
        std::array<int,NUM_TORCH> idx;
        for (int i=0;i<NUM_TORCH;i++) idx[i]=i;
        std::sort(idx.begin(), idx.end(), [](int a, int b){
            float da = glm::length(glm::vec3(carPos.x,0,carPos.z)
                                 - glm::vec3(torchWorldPos[a].x,0,torchWorldPos[a].z));
            float db = glm::length(glm::vec3(carPos.x,0,carPos.z)
                                 - glm::vec3(torchWorldPos[b].x,0,torchWorldPos[b].z));
            return da < db;
        });
        glm::vec3 torchCol(1.0f, 0.45f, 0.08f);   /* orange fire */
        for (int s=0; s<NUM_TORCH_SLOTS; s++) {
            /* cutoff >= 1.0 → point light in the shader */
            setLight(LIGHT_TORCH_0+s,
                     torchWorldPos[idx[s]], torchCol,
                     glm::vec3(0,-1,0), 1.1f,
                     lightsOn * 3.5f);
        }
    }

    glUniform1i(uNumLights, MAX_LIGHTS);

    /* ── Camera ── */
    glm::mat4 viewMat; glm::vec3 eyePos;
    getCamera(viewMat, eyePos);

    /* ── Reset matrix stack ── */
    while (!matStack.empty()) matStack.pop();
    matStack.push(glm::mat4(1.0f));

    /* ══════════════════════════════════════════════
     *  COBBLESTONE GROUND DISK
     * ══════════════════════════════════════════════ */
    setTexMaterial(texCobble, glm::vec2(6.0f,6.0f), {0.03f,0.03f,0.03f}, 4.0f);
    /* Boost ambient at night so the floor isn't pitch black */
    glUniform1f(uAmbi, sky.ambStr + lightsOn * 0.12f);
    setModel(top());
    mGround.draw();
    glUniform1i(uUseTex, 0);

    /* ══════════════════════════════════════════════
     *  GRASS — central island (inside inner track)
     * ══════════════════════════════════════════════ */
    {
        setMaterial({0.22f,0.48f,0.14f},{0.02f,0.04f,0.01f},3.0f);
        glUniform1f(uAmbi, sky.ambStr);
        glm::mat4 gm = glm::scale(top(), glm::vec3(
            (TRK_A_IN - ROAD_W*0.5f - 0.5f)/ARENA_R, 1.0f,
            (TRK_B_IN - ROAD_W*0.5f - 0.5f)/ARENA_R));
        gm = glm::translate(gm, glm::vec3(0,0.015f,0));
        setModel(gm);
        mGround.draw();
    }

    /* ══════════════════════════════════════════════
     *  OUTER ROAD + KERBS
     * ══════════════════════════════════════════════ */
    glUniform1f(uAmbi, sky.ambStr);
    setMaterial({0.18f,0.17f,0.15f},{0.04f,0.04f,0.04f},6.0f);
    setModel(top()); mRoad.draw();
    auto drawKerb = [&](Mesh& m, float sX, float sZ) {
        glm::mat4 km = glm::scale(top(), glm::vec3(sX,1.0f,sZ));
        km = glm::translate(km, glm::vec3(0,0.005f,0));
        setMaterial({0.88f,0.84f,0.76f},{0.05f,0.05f,0.05f},8.0f);
        setModel(km); m.draw();
    };
    drawKerb(mRoad, (TRK_A_OUT+ROAD_W*0.5f+0.2f)/TRK_A_OUT, (TRK_B_OUT+ROAD_W*0.5f+0.2f)/TRK_B_OUT);
    drawKerb(mRoad, (TRK_A_OUT-ROAD_W*0.5f-0.2f)/TRK_A_OUT, (TRK_B_OUT-ROAD_W*0.5f-0.2f)/TRK_B_OUT);

    /* ══════════════════════════════════════════════
     *  INNER ROAD + KERBS
     * ══════════════════════════════════════════════ */
    setMaterial({0.18f,0.17f,0.15f},{0.04f,0.04f,0.04f},6.0f);
    setModel(top()); mRoadInner.draw();
    drawKerb(mRoadInner, (TRK_A_IN+ROAD_W*0.5f+0.2f)/TRK_A_IN, (TRK_B_IN+ROAD_W*0.5f+0.2f)/TRK_B_IN);
    drawKerb(mRoadInner, (TRK_A_IN-ROAD_W*0.5f-0.2f)/TRK_A_IN, (TRK_B_IN-ROAD_W*0.5f-0.2f)/TRK_B_IN);

    /* ══════════════════════════════════════════════
     *  SAND / SEATING RING
     * ══════════════════════════════════════════════ */
    {
        glm::mat4 sm = glm::scale(top(), glm::vec3(
            (COL_R_IN-0.8f)/TRK_A_OUT, 1.0f, (COL_R_IN-0.8f)/TRK_B_OUT));
        sm = glm::translate(sm, glm::vec3(0,-0.005f,0));
        setMaterial({0.76f,0.66f,0.48f},{0.03f,0.03f,0.03f},3.0f);
        setModel(sm); mRoad.draw();
    }

    /* ══════════════════════════════════════════════
     *  COLOSSEUM WALL
     * ══════════════════════════════════════════════ */
    {
        static Mesh colMesh = createColosseum();
        setTexMaterial(texStone, glm::vec2(1.0f,1.0f), {0.06f,0.05f,0.04f},12.0f);
        glUniform1f(uAmbi, sky.ambStr + 0.05f);
        setModel(top());
        colMesh.draw();
        glUniform1i(uUseTex, 0);
    }

    /* ══════════════════════════════════════════════
     *  STONE PILLARS on inner colosseum rim
     * ══════════════════════════════════════════════ */
    {
        const int NP = 24;
        glUniform1f(uAmbi, sky.ambStr);
        setMaterial({0.72f,0.65f,0.50f},{0.06f,0.06f,0.05f},16.0f);
        for (int i=0;i<NP;i++) {
            float t=2*(float)M_PI*i/NP;
            glm::mat4 pm=glm::translate(top(),glm::vec3(COL_R_IN*cosf(t),COL_H*0.5f,COL_R_IN*sinf(t)));
            pm=glm::scale(pm,glm::vec3(0.55f,COL_H,0.55f));
            setModel(pm); mCylinder.draw();
        }
    }

    /* ══════════════════════════════════════════════
     *  TREES (scattered in the median strip)
     * ══════════════════════════════════════════════ */
    // glUniform1f(uAmbi, sky.ambStr);
    // for (int ti=0;ti<NUM_TREE;ti++) {
    //     float ang   = 2*(float)M_PI * rnd(ti,0);
    //     float rMin  = TRK_A_IN  + ROAD_W*0.5f + 1.0f;
    //     float rMax  = TRK_A_OUT - ROAD_W*0.5f - 1.0f;
    //     float frac  = 0.2f + rnd(ti,1)*0.6f;
    //     float rad   = rMin + frac*(rMax-rMin);
    //     float aspect= TRK_B_OUT / TRK_A_OUT;
    //     float tx    = rad*cosf(ang);
    //     float tz    = rad*sinf(ang)*aspect;
    //     float treeH = 2.0f + rnd(ti,2)*2.5f;
    //     float folR  = 0.9f + rnd(ti,3)*0.6f;

    //     pushM(glm::translate(top(), glm::vec3(tx,0,tz)));

    //     setMaterial({0.32f,0.22f,0.12f},{0.02f,0.02f,0.02f},4.0f);
    //     {
    //         glm::mat4 trk=glm::translate(top(),glm::vec3(0,treeH*0.5f,0));
    //         trk=glm::scale(trk,glm::vec3(0.18f,treeH,0.18f));
    //         setModel(trk); mCylinder.draw();
    //     }
    //     float gv = 0.35f + rnd(ti,4)*0.2f;
    //     setMaterial({0.10f+rnd(ti,5)*0.05f,gv,0.08f},{0.02f,0.04f,0.02f},4.0f);
    //     {
    //         glm::mat4 fol=glm::translate(top(),glm::vec3(0,treeH+folR*0.6f,0));
    //         fol=glm::scale(fol,glm::vec3(folR*2));
    //         setModel(fol); mSphere.draw();
    //     }
    //     popM();
    // }

    /* ══════════════════════════════════════════════
     *  CENTRAL STATUE (replaces the "green bush")
     *  A warrior figure: cylinder body, sphere head,
     *  box arms, on a stone pedestal.
     * ══════════════════════════════════════════════ */
    {
        glUniform1f(uAmbi, sky.ambStr + 0.08f);
        pushM(glm::translate(top(), glm::vec3(0,0,0)));

        /* Pedestal */
        setMaterial({0.68f,0.62f,0.52f},{0.08f,0.08f,0.07f},20.0f);
        {
            glm::mat4 ped=glm::translate(top(),glm::vec3(0,0.4f,0));
            ped=glm::scale(ped,glm::vec3(0.9f,0.8f,0.9f));
            setModel(ped); mBox.draw();
        }
        /* Steps */
        {
            glm::mat4 st=glm::translate(top(),glm::vec3(0,0.05f,0));
            st=glm::scale(st,glm::vec3(1.3f,0.1f,1.3f));
            setModel(st); mBox.draw();
        }

        /* Bronze figure — warm metallic green patina */
        glm::vec3 bronzeCol(0.35f,0.60f,0.42f);
        glm::vec3 bronzeSpec(0.55f,0.80f,0.60f);
        setMaterial(bronzeCol, bronzeSpec, 80.0f);

        /* Torso */
        {
            glm::mat4 tor=glm::translate(top(),glm::vec3(0,1.6f,0));
            tor=glm::scale(tor,glm::vec3(0.42f,0.65f,0.30f));
            setModel(tor); mBox.draw();
        }
        /* Head */
        {
            glm::mat4 hd=glm::translate(top(),glm::vec3(0,2.18f,0));
            hd=glm::scale(hd,glm::vec3(0.32f));
            setModel(hd); mSphere.draw();
        }
        /* Helmet crest (thin box on top of head) */
        setMaterial({0.65f,0.20f,0.10f},{0.40f,0.15f,0.08f},40.0f);
        {
            glm::mat4 crest=glm::translate(top(),glm::vec3(0,2.40f,0));
            crest=glm::scale(crest,glm::vec3(0.06f,0.25f,0.28f));
            setModel(crest); mBox.draw();
        }
        setMaterial(bronzeCol, bronzeSpec, 80.0f);
        /* Left arm — raised, holding spear */
        {
            glm::mat4 arm=glm::translate(top(),glm::vec3(-0.32f,1.75f,0));
            arm=glm::rotate(arm,glm::radians(-55.0f),glm::vec3(0,0,1));
            arm=glm::scale(arm,glm::vec3(0.10f,0.55f,0.10f));
            setModel(arm); mCylinder.draw();
        }
        /* Right arm — down, holding shield (box) */
        {
            glm::mat4 arm=glm::translate(top(),glm::vec3(0.32f,1.60f,0));
            arm=glm::rotate(arm,glm::radians(20.0f),glm::vec3(0,0,1));
            arm=glm::scale(arm,glm::vec3(0.10f,0.50f,0.10f));
            setModel(arm); mCylinder.draw();
        }
        /* Shield */
        setMaterial({0.55f,0.42f,0.25f},{0.35f,0.28f,0.18f},32.0f);
        {
            glm::mat4 sh=glm::translate(top(),glm::vec3(0.50f,1.55f,0));
            sh=glm::scale(sh,glm::vec3(0.08f,0.48f,0.38f));
            setModel(sh); mBox.draw();
        }
        /* Spear shaft */
        setMaterial({0.42f,0.30f,0.15f},{0.20f,0.15f,0.08f},16.0f);
        {
            glm::mat4 sp=glm::translate(top(),glm::vec3(-0.70f,1.80f,0));
            sp=glm::rotate(sp,glm::radians(-15.0f),glm::vec3(0,0,1));
            sp=glm::scale(sp,glm::vec3(0.05f,1.60f,0.05f));
            setModel(sp); mCylinder.draw();
        }
        /* Spear tip */
        setMaterial(bronzeCol, bronzeSpec, 80.0f);
        {
            glm::mat4 tip=glm::translate(top(),glm::vec3(-1.06f,2.72f,0));
            tip=glm::scale(tip,glm::vec3(0.08f,0.22f,0.08f));
            setModel(tip); mSphere.draw();
        }
        /* Legs */
        setMaterial(bronzeCol, bronzeSpec, 80.0f);
        for (float s : {-0.13f, 0.13f}) {
            glm::mat4 leg=glm::translate(top(),glm::vec3(s,1.0f,0));
            leg=glm::scale(leg,glm::vec3(0.13f,0.60f,0.13f));
            setModel(leg); mCylinder.draw();
        }

        popM(); /* statue */
    }

    /* ══════════════════════════════════════════════
     *  FIRE TORCHES on inner colosseum rim
     * ══════════════════════════════════════════════ */
    {
        float flicker = 0.88f + 0.12f*sinf(globalTime*8.7f + 0.5f*cosf(globalTime*3.1f));
        glUniform1f(uAmbi, sky.ambStr);

        for (int ti=0;ti<NUM_TORCH;ti++) {
            float ang = 2*(float)M_PI*ti/NUM_TORCH;
            float px  = TORCH_R*cosf(ang);
            float pz  = TORCH_R*sinf(ang);

            pushM(glm::translate(top(),glm::vec3(px,0,pz)));

            /* Stone bracket pole */
            setMaterial({0.55f,0.50f,0.40f},{0.04f,0.04f,0.04f},8.0f);
            {
                glm::mat4 pm=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H*0.5f,0));
                pm=glm::scale(pm,glm::vec3(0.08f,TORCH_H,0.08f));
                setModel(pm); mCylinder.draw();
            }
            /* Bowl */
            {
                glm::mat4 bm=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H,0));
                bm=glm::scale(bm,glm::vec3(0.22f,0.08f,0.22f));
                setModel(bm); mCylinder.draw();
            }

            /* Flame — emissive, scales with lightsOn and flicker */
            float fr = FLAME_R * (0.4f + 0.6f*lightsOn) * flicker;
            glm::vec3 fireCol  = glm::mix(glm::vec3(0.28f,0.22f,0.18f),
                                          glm::vec3(1.00f,0.44f,0.04f), lightsOn);
            glm::vec3 fireEmit = glm::mix(glm::vec3(0.02f,0.01f,0.01f),
                                          glm::vec3(2.80f,0.85f,0.08f), lightsOn);
            setMaterial(fireCol, {0,0,0}, 1.0f, 1.0f, fireEmit);
            {
                glm::mat4 fm=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H+fr,0));
                fm=glm::scale(fm,glm::vec3(fr*2));
                setModel(fm); mSphere.draw();
            }
            /* Secondary wisp */
            {
                glm::mat4 fm2=glm::translate(top(),glm::vec3(0.02f*sinf(globalTime*4+ti),COL_H+TORCH_H+fr*2.1f,0));
                fm2=glm::scale(fm2,glm::vec3(fr*1.05f));
                setModel(fm2); mSphere.draw();
            }

            popM();
        }
    }

    /* ══════════════════════════════════════════════
     *  BUILDINGS
     * ══════════════════════════════════════════════ */
    glUniform1f(uAmbi, sky.ambStr);
    for (int bi=0;bi<NUM_B;bi++) {
        auto& B = bldg[bi];
        float bldgH = (float)B.stories * STORY_H;
        pushM(glm::translate(top(), B.pos));

        GLuint tex=(B.texType==0)?texBrick:(B.texType==1)?texWood:texConcrete;
        setTexMaterial(tex, glm::vec2(B_HALF,bldgH*0.5f), {0.06f,0.06f,0.06f},10.0f);
        {
            glm::mat4 body=glm::translate(top(),glm::vec3(0,bldgH*0.5f,0));
            body=glm::scale(body,glm::vec3(B_HALF*2,bldgH,B_HALF*2));
            setModel(body); mBox.draw();
        }
        glUniform1i(uUseTex,0);

        /* Pitched roof */
        setMaterial({0.52f,0.32f,0.22f},{0.04f,0.04f,0.04f},6.0f);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 roof=glm::translate(top(),glm::vec3(side*B_HALF*0.5f,bldgH+0.3f,0));
            roof=glm::rotate(roof,side*glm::radians(35.0f),glm::vec3(0,0,1));
            roof=glm::scale(roof,glm::vec3(B_HALF,0.12f,B_HALF*2.1f));
            setModel(roof); mBox.draw();
        }

        /* Fan */
        {
            glm::vec3 fanOff=glm::vec3(0,bldgH-0.1f,0)+B.toRoad*B_HALF;
            pushM(glm::translate(top(),fanOff));
            setMaterial({0.42f,0.42f,0.47f},{0.25f,0.25f,0.25f},32.0f);
            setModel(glm::scale(top(),glm::vec3(0.12f))); mSphere.draw();

            pushM(top());
            top()=glm::rotate(top(),B.roadYaw,glm::vec3(0,1,0));
            top()=glm::rotate(top(),fanAngle, glm::vec3(0,0,1));
            setMaterial({0.55f,0.52f,0.45f},{0.12f,0.12f,0.10f},16.0f);
            for (int b=0;b<4;b++) {
                glm::mat4 blade=top();
                blade=glm::rotate(blade,glm::radians(90.0f*b),glm::vec3(0,0,1));
                blade=glm::translate(blade,glm::vec3(0,FAN_R*0.5f,0));
                blade=glm::scale(blade,glm::vec3(FAN_BW,FAN_R,FAN_BT));
                setModel(blade); mBox.draw();
            }
            popM(); popM();
        }

        /* Gimbal arm + bulb
           Bulb emissive scales with lightsOn. */
        {
            glm::vec3 mountOff(0,bldgH+0.3f,0);
            glm::vec3 toRd=B.nearRoad-(B.pos+mountOff);
            float baseYaw=atan2f(toRd.x,toRd.z);
            float pitch=atan2f(-toRd.y,sqrtf(toRd.x*toRd.x+toRd.z*toRd.z));
            float swing=glm::radians(SW_MAX)*sinf(globalTime*SW_SPD+(float)bi);

            pushM(top());
            top()=glm::translate(top(),mountOff);
            top()=glm::rotate(top(),baseYaw+swing,glm::vec3(0,1,0));
            top()=glm::rotate(top(),pitch,        glm::vec3(1,0,0));

            setMaterial({0.30f,0.30f,0.32f},{0.08f,0.08f,0.08f},20.0f);
            setModel(glm::scale(top(),glm::vec3(0.10f))); mSphere.draw();

            setMaterial({0.33f,0.33f,0.36f},{0.08f,0.08f,0.08f},16.0f);
            {
                glm::mat4 arm=glm::translate(top(),glm::vec3(0,0,LT_ARM*0.5f));
                arm=glm::scale(arm,glm::vec3(0.06f,0.06f,LT_ARM));
                setModel(arm); mBox.draw();
            }

            /* Bulb: emissive only when lights are on */
            glm::vec3 bulbEmit = B.lightCol * 2.0f * lightsOn;
            setMaterial(B.lightCol,{0,0,0},1.0f,1.0f, bulbEmit);
            {
                glm::mat4 bulb=glm::translate(top(),glm::vec3(0,0,LT_ARM));
                bulb=glm::scale(bulb,glm::vec3(LT_BULB_R*2));
                setModel(bulb); mSphere.draw();
            }
            popM();
        }

        popM(); /* building */
    }

    /* ══════════════════════════════════════════════
     *  CAR — metallic red, with emissive tail-lights at night
     * ══════════════════════════════════════════════ */
    {
        glm::mat4 carBase=glm::translate(top(),carPos);
        carBase=glm::rotate(carBase,carHeading,glm::vec3(0,1,0));
        pushM(carBase);

        /* Body — metallic red */
        glm::vec3 carRed(0.72f, 0.06f, 0.06f);
        glm::vec3 carSpec(0.90f, 0.80f, 0.80f);
        setMaterial(carRed, carSpec, 160.0f);
        {
            glm::mat4 body=glm::translate(top(),glm::vec3(0,WHL_R+CAR_BH*0.5f,0));
            body=glm::scale(body,glm::vec3(CAR_W,CAR_BH,CAR_L));
            setModel(body); mBox.draw();
        }

        /* Cabin — darker red tint */
        setMaterial({0.55f,0.04f,0.04f}, carSpec, 160.0f);
        {
            glm::mat4 cab=glm::translate(top(),glm::vec3(0,WHL_R+CAR_BH+CAR_CH*0.5f,-0.15f));
            cab=glm::scale(cab,glm::vec3(CAR_W*0.88f,CAR_CH,CAR_L*0.5f));
            setModel(cab); mBox.draw();
        }

        /* Headlights — emissive warm white, glow when on */
        glm::vec3 hlEmit = headlightsOn ? glm::vec3(1.8f,1.7f,1.2f) : glm::vec3(0.05f,0.05f,0.04f);
        setMaterial({1.0f,1.0f,0.88f},{0,0,0},1.0f,1.0f, hlEmit);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 hl=glm::translate(top(),glm::vec3(side*CAR_W*0.35f,WHL_R+CAR_BH*0.5f,CAR_L*0.52f));
            hl=glm::scale(hl,glm::vec3(0.13f,0.09f,0.04f));
            setModel(hl); mBox.draw();
        }
        /* Headlight housing chrome ring */
        setMaterial({0.75f,0.75f,0.80f},{0.9f,0.9f,0.9f},200.0f);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 hr=glm::translate(top(),glm::vec3(side*CAR_W*0.35f,WHL_R+CAR_BH*0.5f,CAR_L*0.53f));
            hr=glm::scale(hr,glm::vec3(0.15f,0.11f,0.02f));
            setModel(hr); mBox.draw();
        }

        /* Tail-lights — red emissive, brighter at night */
        glm::vec3 tlEmit = glm::mix(glm::vec3(0.4f,0.02f,0.02f),
                                    glm::vec3(2.0f,0.10f,0.08f), lightsOn);
        setMaterial({1.0f,0.08f,0.08f},{0,0,0},1.0f,1.0f, tlEmit);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 tl=glm::translate(top(),glm::vec3(side*CAR_W*0.38f,WHL_R+CAR_BH*0.5f,-CAR_L*0.52f));
            tl=glm::scale(tl,glm::vec3(0.11f,0.07f,0.04f));
            setModel(tl); mBox.draw();
        }
        /* Thin chrome bumper strip */
        setMaterial({0.65f,0.65f,0.70f},{0.85f,0.85f,0.90f},180.0f);
        for (float sign:{-1.0f,1.0f}) {
            glm::mat4 bump=glm::translate(top(),glm::vec3(0,WHL_R+0.05f,sign*CAR_L*0.52f));
            bump=glm::scale(bump,glm::vec3(CAR_W*1.05f,0.06f,0.04f));
            setModel(bump); mBox.draw();
        }

        /* Wheels — black rubber */
        setMaterial({0.08f,0.08f,0.08f},{0.12f,0.12f,0.12f},8.0f);
        float wx[]={-CAR_W*0.5f,CAR_W*0.5f};
        float wz[]={ CAR_L*0.3f,-CAR_L*0.3f};
        for (int i=0;i<2;i++) for (int j=0;j<2;j++) {
            glm::mat4 wm=glm::translate(top(),glm::vec3(wx[i],WHL_R,wz[j]));
            wm=glm::rotate(wm,(float)M_PI*0.5f,glm::vec3(0,0,1));
            wm=glm::rotate(wm,wheelRot,glm::vec3(0,1,0));
            wm=glm::scale(wm,glm::vec3(WHL_R*2,WHL_W,WHL_R*2));
            setModel(wm); mCylinder.draw();
        }
        /* Alloy wheel centres — silver */
        setMaterial({0.70f,0.72f,0.76f},{0.85f,0.85f,0.90f},120.0f);
        for (int i=0;i<2;i++) for (int j=0;j<2;j++) {
            glm::mat4 hb=glm::translate(top(),glm::vec3(wx[i],WHL_R,wz[j]));
            hb=glm::scale(hb,glm::vec3(WHL_R*1.2f,WHL_W*1.05f,WHL_R*1.2f));
            setModel(hb); mSphere.draw();
        }

        popM(); /* car */
    }
}