/* =====================================================================
 *  render.cpp — Colosseum scene rendering
 *
 *  Light slot layout (MAX_LIGHTS = 25):
 *    0-4   : building gimbal spotlights (NUM_B=5)
 *    5-6   : car headlights (two cones, toggleable)
 *    7-8   : dipper lights (ground-focused, near front bumper)
 *    9-24  : ALL 16 torches (uniform high-intensity point lights)
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
    { {0.66f,0.72f,0.78f}, {1.00f,0.95f,0.90f},
      glm::normalize(glm::vec3(-0.75f,-0.45f, 0.45f)),
      0.70f, 0.14f,
      {0.62f,0.66f,0.70f}, 0.006f },
    /* 1 — Noon */
    { {0.58f,0.70f,0.80f}, {1.00f,1.00f,0.97f},
      glm::normalize(glm::vec3( 0.12f,-1.0f, 0.10f)),
      1.00f, 0.12f,
      {0.60f,0.68f,0.75f}, 0.004f },
    /* 2 — Evening */
    { {0.30f,0.33f,0.37f}, {1.00f,0.88f,0.76f},
      glm::normalize(glm::vec3( 0.90f,-0.20f,-0.40f)),
      0.28f, 0.055f,
      {0.20f,0.22f,0.24f}, 0.0028f },
    /* 3 — Night */
    { {0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f},
      glm::normalize(glm::vec3(0.0f,-1.0f,0.0f)),
      0.00f, 0.0045f,
      {0.0f,0.0f,0.0f}, 0.0007f },
};

static PeriodSky blendSky(int p, float frac) {
    /* Blend in the final PERIOD_FADE seconds of each period */
    float fadeStart = 1.0f - (PERIOD_FADE / PERIOD_DUR);
    float fadeFrac = 0.0f;
    if (frac > fadeStart)
        fadeFrac = std::min((frac - fadeStart) / (1.0f - fadeStart), 1.0f);
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
    const float gimbalStrengthMax = 6.8f;
    float gimbalStrength = lightsOn * gimbalStrengthMax;
    for (int i = 0; i < NUM_B; i++) {
        setLight(LIGHT_GIMBAL_0 + i, spotPos[i], bldg[i].lightCol, spotDir[i],
                 cosf(glm::radians(35.0f)), gimbalStrength);
    }

    /* ── Slots 5-6: car headlights ──
       Two tight spot cones, pointing forward-and-down from the front bumper.
       Only active when headlightsOn AND (lights should be on at all, or
       player explicitly enabled them). */
    {
        glm::vec3 fwd (sinf(carHeading), 0.0f, cosf(carHeading));
        glm::vec3 right(-cosf(carHeading), 0.0f, sinf(carHeading));
        /* Headlight mounts: front bumper corners, just above ground */
        float hy    = useChariot ? 0.82f : (WHL_R + CAR_BH * 0.95f);
        float hz    = useChariot ? (CAR_L * 0.50f) : (CAR_L * 0.52f);
        float hx    = useChariot ? (CAR_W * 0.44f) : (CAR_W * 0.35f);
        glm::vec3 hl[2] = {
            carPos + fwd*hz + right*hx + glm::vec3(0,hy,0),
            carPos + fwd*hz - right*hx + glm::vec3(0,hy,0),
        };
        /* Cone axis: forward and steeply enough downward to pool light on the ground */
        glm::vec3 hlDir = glm::normalize(fwd + glm::vec3(0, useChariot ? -0.55f : -0.72f, 0));
        float hlStr = (headlightsOn) ? (useChariot ? 10.5f : 14.0f) : 0.0f;
        /* Wider cone so light reaches the ground surface visibly */
        float hlCut = cosf(glm::radians(useChariot ? 34.0f : 38.0f));
        glm::vec3 hlCol = useChariot
                        ? glm::vec3(1.0f, 0.82f, 0.52f)
                        : glm::vec3(1.0f, 0.97f, 0.85f);
        setLight(LIGHT_HEADLIGHT+0, hl[0], hlCol, hlDir, hlCut, hlStr);
        setLight(LIGHT_HEADLIGHT+1, hl[1], hlCol, hlDir, hlCut, hlStr);

        /* Dipper lights: mounted lower and aimed sharply down. */
        float dpy = useChariot ? (hy - 0.20f) : (WHL_R + CAR_BH * 0.42f);
        float dpz = useChariot ? (CAR_L * 0.50f) : (CAR_L * 0.50f);
        float dpx = useChariot ? (CAR_W * 0.34f) : (CAR_W * 0.30f);
        glm::vec3 dp[2] = {
            carPos + fwd*dpz + right*dpx + glm::vec3(0,dpy,0),
            carPos + fwd*dpz - right*dpx + glm::vec3(0,dpy,0),
        };
        glm::vec3 dpDir = glm::normalize(fwd + glm::vec3(0, useChariot ? -1.25f : -1.55f, 0));
        float dpStr = headlightsOn ? (useChariot ? 7.2f : 9.5f) : 0.0f;
        float dpCut = cosf(glm::radians(useChariot ? 54.0f : 58.0f));
        glm::vec3 dpCol = useChariot
                        ? glm::vec3(1.00f, 0.74f, 0.42f)
                        : glm::vec3(1.00f, 0.94f, 0.78f);
        setLight(LIGHT_DIPPER+0, dp[0], dpCol, dpDir, dpCut, dpStr);
        setLight(LIGHT_DIPPER+1, dp[1], dpCol, dpDir, dpCut, dpStr);
    }

    /* ── Slots 9-24: ALL 16 torches — uniform high-intensity point lights ──
       Every torch gets its own light slot for equal brightness.
       Point lights (cutoff >= 1.0 = omnidirectional in shader). */
    {
        glm::vec3 torchCol(1.0f, 0.45f, 0.08f);   /* orange fire */
        for (int s=0; s<NUM_TORCH_SLOTS && s<NUM_TORCH; s++) {
            setLight(LIGHT_TORCH_0+s,
                     torchWorldPos[s], torchCol,
                     glm::vec3(0,-1,0), 1.1f,
                     lightsOn * 5.0f);
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
     *  MUD GROUND DISK — confined to colosseum interior only.
     *  The ground mesh radius is COL_R_IN so it ends at the wall.
     *  Outside the wall is bare sky (no floor drawn there).
     * ══════════════════════════════════════════════ */
    setTexMaterial(texMud, glm::vec2(6.0f,6.0f), {0.02f,0.01f,0.01f}, 4.0f, std::max(0.06f, sky.ambStr));
    setModel(top());
    mGround.draw();
    glUniform1i(uUseTex, 0);

    /* ══════════════════════════════════════════════
     *  MUD — central island (inside inner track)
     * ══════════════════════════════════════════════ */
    {
        setTexMaterial(texMud, glm::vec2(1.8f,1.8f), {0.02f,0.01f,0.01f}, 4.0f, sky.ambStr);
        // glUniform1f(uAmbi, sky.ambStr);
        glm::mat4 gm = glm::scale(top(), glm::vec3(
            (TRK_A_IN - ROAD_W*0.5f - 0.5f)/ARENA_R, 1.0f,
            (TRK_B_IN - ROAD_W*0.5f - 0.5f)/ARENA_R));
        gm = glm::translate(gm, glm::vec3(0,0.015f,0));
        setModel(gm); mGround.draw();
        glUniform1i(uUseTex, 0);
    }

    /* ══════════════════════════════════════════════
     *  OUTER ROAD + KERBS
     * ══════════════════════════════════════════════ */
    glUniform1f(uAmbi, sky.ambStr);
    setTexMaterial(texConcrete, glm::vec2(2.5f,1.0f), {0.04f,0.04f,0.04f},8.0f);
    setModel(top()); mRoad.draw();
    glUniform1i(uUseTex, 0);
    auto drawKerb = [&](Mesh& m, float sX, float sZ) {
        glm::mat4 km = glm::scale(top(), glm::vec3(sX,1.0f,sZ));
        km = glm::translate(km, glm::vec3(0,0.005f,0));
        setTexMaterial(texStone, glm::vec2(0.6f,0.4f), {0.05f,0.05f,0.05f},8.0f, sky.ambStr);
        // glUniform1f(uAmbi, sky.ambStr);
        setModel(km); m.draw();
        glUniform1i(uUseTex, 0);
    };
    drawKerb(mRoad, (TRK_A_OUT+ROAD_W*0.5f+0.2f)/TRK_A_OUT, (TRK_B_OUT+ROAD_W*0.5f+0.2f)/TRK_B_OUT);
    drawKerb(mRoad, (TRK_A_OUT-ROAD_W*0.5f-0.2f)/TRK_A_OUT, (TRK_B_OUT-ROAD_W*0.5f-0.2f)/TRK_B_OUT);

    /* ══════════════════════════════════════════════
     *  INNER ROAD + KERBS
     * ══════════════════════════════════════════════ */
    setTexMaterial(texConcrete, glm::vec2(1.9f,0.8f), {0.04f,0.04f,0.04f},8.0f, sky.ambStr);
    // glUniform1f(uAmbi, sky.ambStr);
    setModel(top()); mRoadInner.draw();
    glUniform1i(uUseTex, 0);
    drawKerb(mRoadInner, (TRK_A_IN+ROAD_W*0.5f+0.2f)/TRK_A_IN, (TRK_B_IN+ROAD_W*0.5f+0.2f)/TRK_B_IN);
    drawKerb(mRoadInner, (TRK_A_IN-ROAD_W*0.5f-0.2f)/TRK_A_IN, (TRK_B_IN-ROAD_W*0.5f-0.2f)/TRK_B_IN);

    /* ══════════════════════════════════════════════
     *  BUILDING-ZONE CONCENTRIC BORDER RINGS
     *  Two stone circles that visually border the building ring
     *  from inside (r≈11.5) and outside (r≈15).
     * ══════════════════════════════════════════════ */
    {
        const float bRingInner = 11.5f;   /* inner border radius */
        const float bRingOuter = 15.0f;   /* outer border radius */
        const float ringW      = 0.18f;   /* ring width */
        const float ringY      = 0.025f;  /* just above ground */
        setMaterial({0.52f,0.46f,0.38f}, {0.08f,0.08f,0.07f}, 10.0f, sky.ambStr);
        for (float r : {bRingInner, bRingOuter}) {
            glm::mat4 rm = glm::translate(top(), glm::vec3(0, ringY, 0));
            rm = glm::scale(rm, glm::vec3(r * 2.0f, 0.04f, r * 2.0f));
            setModel(rm); mCylinder.draw();
            /* Slightly smaller cylinder inside to leave only the ring edge */
            setMaterial({0.55f,0.38f,0.22f}, {0.02f,0.01f,0.01f}, 4.0f, sky.ambStr);
            glm::mat4 inner = glm::translate(top(), glm::vec3(0, ringY - 0.001f, 0));
            inner = glm::scale(inner, glm::vec3((r - ringW) * 2.0f, 0.04f, (r - ringW) * 2.0f));
            setModel(inner); mCylinder.draw();
            setMaterial({0.52f,0.46f,0.38f}, {0.08f,0.08f,0.07f}, 10.0f, sky.ambStr);
        }
    }

    /* ══════════════════════════════════════════════
     *  SAND / SEATING RING
     * ══════════════════════════════════════════════ */
    {
        glm::mat4 sm = glm::scale(top(), glm::vec3(
            (COL_R_IN-0.8f)/TRK_A_OUT, 1.0f, (COL_R_IN-0.8f)/TRK_B_OUT));
        sm = glm::translate(sm, glm::vec3(0,-0.005f,0));
        setTexMaterial(texWood, glm::vec2(2.2f,1.2f), {0.03f,0.03f,0.03f},4.0f, sky.ambStr);
        // glUniform1f(uAmbi, sky.ambStr);
        setModel(sm); mRoad.draw();
        glUniform1i(uUseTex, 0);
    }

    /* ══════════════════════════════════════════════
     *  COLOSSEUM WALL
     * ══════════════════════════════════════════════ */
    {
        static Mesh colMesh = createColosseum();
        setTexMaterial(texStone, glm::vec2(2.6f,1.4f), {0.06f,0.05f,0.04f},16.0f,sky.ambStr + 0.035f);
        // glUniform1f(uAmbi, sky.ambStr + 0.035f);
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
        setTexMaterial(texStone, glm::vec2(0.55f,2.4f), {0.06f,0.06f,0.05f},12.0f);
        for (int i=0;i<NP;i++) {
            float t=2*(float)M_PI*i/NP;
            glm::mat4 pm=glm::translate(top(),glm::vec3(COL_R_IN*cosf(t),COL_H*0.5f,COL_R_IN*sinf(t)));
            pm=glm::scale(pm,glm::vec3(0.55f,COL_H,0.55f));
            setModel(pm); mCylinder.draw();
        }
        glUniform1i(uUseTex, 0);
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

    /* ================================================
     *  CENTER SPAWN MARKER (replaces statue)
     * ================================================ */
    {
        glUniform1f(uAmbi, sky.ambStr + 0.02f);
        setMaterial({0.22f,0.46f,0.86f}, {0.10f,0.18f,0.35f}, 30.0f, sky.ambStr);
        glm::mat4 outer = glm::translate(top(), glm::vec3(0.0f, 0.03f, 0.0f));
        outer = glm::scale(outer, glm::vec3(2.3f, 0.05f, 2.3f));
        setModel(outer); mCylinder.draw();

        setMaterial({0.75f,0.85f,1.00f}, {0.08f,0.08f,0.08f}, 8.0f, sky.ambStr);
        glm::mat4 inner = glm::translate(top(), glm::vec3(0.0f, 0.031f, 0.0f));
        inner = glm::scale(inner, glm::vec3(1.4f, 0.03f, 1.4f));
        setModel(inner); mCylinder.draw();
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

            /* Textured wood pole */
            setTexMaterial(texWood, glm::vec2(0.35f,1.8f), {0.04f,0.04f,0.04f},8.0f, sky.ambStr);
            // glUniform1f(uAmbi, sky.ambStr);
            {
                glm::mat4 pm=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H*0.5f,0));
                pm=glm::scale(pm,glm::vec3(0.08f,TORCH_H,0.08f));
                setModel(pm); mCylinder.draw();
            }
            glUniform1i(uUseTex, 0);
            /* Bowl */
            setTexMaterial(texStone, glm::vec2(0.55f,0.30f), {0.05f,0.05f,0.05f},8.0f,sky.ambStr);
            // glUniform1f(uAmbi, sky.ambStr);
            {
                glm::mat4 bm=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H,0));
                bm=glm::scale(bm,glm::vec3(0.22f,0.08f,0.22f));
                setModel(bm); mCylinder.draw();
            }
            glUniform1i(uUseTex, 0);

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

            /* Textured flame billboards (crossed) */
            setTexMaterial(texTorch, glm::vec2(0.22f,0.22f), {0.0f,0.0f,0.0f}, 2.0f, 0.01f);
            // glUniform1f(uAmbi, 0.01f);
            glm::vec3 billEmit = glm::vec3(1.4f,0.75f,0.25f) * lightsOn * flicker;
            glUniform3fv(uEmissive, 1, glm::value_ptr(billEmit));
            for (int q = 0; q < 2; q++) {
                glm::mat4 fb=glm::translate(top(),glm::vec3(0,COL_H+TORCH_H+fr*1.5f,0));
                fb=glm::rotate(fb,glm::radians(90.0f*(float)q + (float)((ti*17)%45)),glm::vec3(0,1,0));
                fb=glm::scale(fb,glm::vec3(fr*0.18f, fr*3.3f, fr*1.9f));
                setModel(fb); mBox.draw();
            }
            glUniform1i(uUseTex, 0);
            glUniform3fv(uEmissive, 1, glm::value_ptr(glm::vec3(0.0f)));

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

        /* texType 0 → building_view_closed.png (brick/stone face)
           texType 1 → building_with_gate.jpeg (gate-house wall)
           texType 2 → gatehouse.jpg (full castle wall) */
        bool hasGate = (B.texType == 1);
        bool gateAlongX = fabsf(B.toRoad.x) > fabsf(B.toRoad.z);
        GLuint tex = hasGate ? texGate : texBrick;
        glm::vec2 facadeScale = hasGate
                              ? glm::vec2(0.88f, 0.46f * (float)B.stories)
                              : glm::vec2(0.62f, 0.36f * (float)B.stories);
        setTexMaterial(tex, facadeScale, {0.06f,0.06f,0.06f},14.0f, sky.ambStr + 0.02f);
        if (!hasGate) {
            glm::mat4 body=glm::translate(top(),glm::vec3(0,bldgH*0.5f,0));
            body=glm::scale(body,glm::vec3(B_HALF*2,bldgH,B_HALF*2));
            setModel(body); mBox.draw();
        } else {
            float wallT = std::max(0.12f, B_HALF - B_GATE_HALF);
            float gateH = STORY_H * 1.55f;
            float topH  = std::max(0.35f, bldgH - gateH);

            if (gateAlongX) {
                glm::mat4 left = glm::translate(top(), glm::vec3(0, bldgH*0.5f,  B_GATE_HALF + wallT*0.5f));
                left = glm::scale(left, glm::vec3(B_HALF*2, bldgH, wallT));
                setModel(left); mBox.draw();

                glm::mat4 right = glm::translate(top(), glm::vec3(0, bldgH*0.5f, -B_GATE_HALF - wallT*0.5f));
                right = glm::scale(right, glm::vec3(B_HALF*2, bldgH, wallT));
                setModel(right); mBox.draw();

                glm::mat4 topBridge = glm::translate(top(), glm::vec3(0, gateH + topH*0.5f, 0));
                topBridge = glm::scale(topBridge, glm::vec3(B_HALF*2, topH, B_GATE_HALF*2));
                setModel(topBridge); mBox.draw();
            } else {
                glm::mat4 left = glm::translate(top(), glm::vec3( B_GATE_HALF + wallT*0.5f, bldgH*0.5f, 0));
                left = glm::scale(left, glm::vec3(wallT, bldgH, B_HALF*2));
                setModel(left); mBox.draw();

                glm::mat4 right = glm::translate(top(), glm::vec3(-B_GATE_HALF - wallT*0.5f, bldgH*0.5f, 0));
                right = glm::scale(right, glm::vec3(wallT, bldgH, B_HALF*2));
                setModel(right); mBox.draw();

                glm::mat4 topBridge = glm::translate(top(), glm::vec3(0, gateH + topH*0.5f, 0));
                topBridge = glm::scale(topBridge, glm::vec3(B_GATE_HALF*2, topH, B_HALF*2));
                setModel(topBridge); mBox.draw();
            }
        }
        glUniform1i(uUseTex,0);

        /* Pitched roof */
        setTexMaterial(texGate, glm::vec2(0.65f,0.35f), {0.04f,0.04f,0.04f},8.0f, sky.ambStr);
        // glUniform1f(uAmbi, sky.ambStr);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 roof=glm::translate(top(),glm::vec3(side*B_HALF*0.5f,bldgH+0.3f,0));
            roof=glm::rotate(roof,side*glm::radians(35.0f),glm::vec3(0,0,1));
            roof=glm::scale(roof,glm::vec3(B_HALF,0.12f,B_HALF*2.1f));
            setModel(roof); mBox.draw();
        }
        glUniform1i(uUseTex, 0);

        /* Window panes: outward-facing, deterministic random night glow */
        {
            const glm::vec3 sideN[4] = {
                glm::vec3( 1,0,0), glm::vec3(-1,0,0),
                glm::vec3( 0,0,1), glm::vec3( 0,0,-1)
            };
            const glm::vec3 sideT[4] = {
                glm::vec3(0,0, 1), glm::vec3(0,0,-1),
                glm::vec3(-1,0,0), glm::vec3(1,0, 0)
            };

            for (int s=0; s<4; s++) {
                for (int floor=0; floor<B.stories; floor++) {
                    float wy = 1.05f + floor * STORY_H;
                    for (int col=0; col<2; col++) {
                        if (hasGate && floor == 0) {
                            bool gateFace = gateAlongX ? (fabsf(sideN[s].x) > 0.5f)
                                                       : (fabsf(sideN[s].z) > 0.5f);
                            if (gateFace) continue;
                        }
                        float sideShift = (col==0 ? -0.46f : 0.46f) * (B_HALF*1.35f);
                        bool baseLit = ((s + floor + col + bi) % 2) == 0;
                        bool extraLit = rnd(bi*31 + s*7 + floor*13 + col*19, 11) > 0.80f;
                        float litMask = (baseLit || extraLit) ? 1.0f : 0.0f;

                        glm::vec3 center = sideN[s] * (B_HALF + 0.03f)
                                         + sideT[s] * sideShift
                                         + glm::vec3(0,wy,0);
                        glm::vec3 paneEmit = glm::vec3(8.0f,6.5f,4.2f) * lightsOn * litMask;
                        glm::vec3 paneCol  = glm::mix(glm::vec3(0.07f,0.10f,0.14f),
                                                      glm::vec3(1.0f,0.92f,0.68f),
                                                      lightsOn * litMask);
                        setMaterial(paneCol, {0.02f,0.02f,0.02f}, 18.0f, sky.ambStr, paneEmit);
                        glm::mat4 wp = glm::translate(top(), center);
                        if (fabsf(sideN[s].x) > 0.5f)
                            wp = glm::scale(wp, glm::vec3(0.032f, 0.62f, 0.38f));
                        else
                            wp = glm::scale(wp, glm::vec3(0.38f, 0.62f, 0.032f));
                        setModel(wp); mBox.draw();

                        setMaterial({0.33f,0.30f,0.22f}, {0.03f,0.03f,0.03f}, 10.0f, sky.ambStr);
                        glm::mat4 frame = glm::translate(top(), center - sideN[s]*0.012f);
                        if (fabsf(sideN[s].x) > 0.5f)
                            frame = glm::scale(frame, glm::vec3(0.026f, 0.72f, 0.48f));
                        else
                            frame = glm::scale(frame, glm::vec3(0.48f, 0.72f, 0.026f));
                        setModel(frame); mBox.draw();
                    }
                }
            }
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
            glm::vec3 sideDir = glm::cross(glm::vec3(0,1,0), B.toRoad);
            if (glm::length(sideDir) < 0.001f) sideDir = glm::vec3(1,0,0);
            sideDir = glm::normalize(sideDir);
            glm::vec3 mountOff = sideDir * (B_HALF + 0.85f) + glm::vec3(0,bldgH+0.85f,0);
            float baseYaw = B.roadYaw;
            float pitch   = glm::radians(40.0f);
            float swing   = glm::radians(SW_MAX) * sinf(globalTime * SW_SPD + (float)bi);

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
            float bulbOn = std::clamp(gimbalStrength / gimbalStrengthMax, 0.0f, 1.0f);
            glm::vec3 bulbEmit = B.lightCol * 2.0f * bulbOn;
            setMaterial(B.lightCol,{0,0,0},1.0f,1.0f, bulbEmit);
            {
                glm::mat4 bulb=glm::translate(top(),glm::vec3(0,0,LT_ARM));
                bulb=glm::scale(bulb,glm::vec3(LT_BULB_R*2));
                setModel(bulb); mSphere.draw();
            }

            if (lightsOn > 0.001f) {
                glm::vec3 beamStart = spotPos[bi];
                float yDir = std::min(spotDir[bi].y, -0.02f);
                float tHit = (0.03f - beamStart.y) / yDir;
                tHit = std::clamp(tHit, 0.5f, COL_H * 3.0f);
                glm::vec3 beamEnd = beamStart + spotDir[bi] * tHit;
                glm::vec3 beamVec = beamEnd - beamStart;
                float beamLen = glm::length(beamVec);
                if (beamLen > 0.001f) {
                    glm::vec3 beamDir = beamVec / beamLen;
                    glm::vec3 zAxis(0,0,1);
                    float c = std::clamp(glm::dot(zAxis, beamDir), -1.0f, 1.0f);
                    glm::vec3 axis = glm::cross(zAxis, beamDir);
                    float axisLen = glm::length(axis);

                    glm::mat4 beamM(1.0f);
                    beamM = glm::translate(beamM, (beamStart + beamEnd) * 0.5f);
                    if (axisLen > 0.0001f) {
                        beamM = glm::rotate(beamM, acosf(c), axis / axisLen);
                    } else if (c < 0.0f) {
                        beamM = glm::rotate(beamM, (float)M_PI, glm::vec3(0,1,0));
                    }
                    beamM = glm::scale(beamM, glm::vec3(0.07f, 0.07f, beamLen));
                    setMaterial(glm::vec3(0.04f,0.04f,0.04f), {0,0,0}, 1.0f, 0.01f,
                                B.lightCol * (1.45f * lightsOn));
                    setModel(beamM); mCylinder.draw();

                    glm::mat4 hit = glm::translate(top(), glm::vec3(beamEnd.x, 0.035f, beamEnd.z));
                    hit = glm::scale(hit, glm::vec3(0.52f, 0.02f, 0.52f));
                    setMaterial(B.lightCol * 0.30f, {0,0,0}, 1.0f, 0.02f,
                                B.lightCol * (0.70f * lightsOn));
                    setModel(hit); mCylinder.draw();
                }
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
        if (!useChariot) {

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
        glm::vec3 dpEmit = headlightsOn ? glm::vec3(1.35f,1.20f,0.95f) : glm::vec3(0.03f,0.03f,0.03f);
        setMaterial({0.95f,0.95f,0.86f},{0,0,0},1.0f,1.0f, dpEmit);
        for (float side:{-1.0f,1.0f}) {
            glm::mat4 dp=glm::translate(top(),glm::vec3(side*CAR_W*0.30f,WHL_R+CAR_BH*0.20f,CAR_L*0.50f));
            dp=glm::scale(dp,glm::vec3(0.11f,0.07f,0.035f));
            setModel(dp); mBox.draw();
        }

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
        } else {
            const float CH_WID = CAR_W * 1.35f;
            const float CH_LEN = CAR_L * 1.20f;
            const float CH_H   = 0.34f;
            const float CH_WR  = WHL_R * 1.65f;
            const float CH_WW  = WHL_W * 0.78f;

            setTexMaterial(texWood, glm::vec2(0.95f,0.60f), {0.10f,0.10f,0.10f}, 12.0f, sky.ambStr + 0.01f);
            // glUniform1f(uAmbi, sky.ambStr + 0.01f);
            {
                glm::mat4 base=glm::translate(top(),glm::vec3(0,CH_WR + CH_H*0.5f,0));
                base=glm::scale(base,glm::vec3(CH_WID,CH_H,CH_LEN));
                setModel(base); mBox.draw();
            }

            setTexMaterial(texGate, glm::vec2(0.60f,0.30f), {0.08f,0.08f,0.08f}, 10.0f, sky.ambStr);
            // glUniform1f(uAmbi, sky.ambStr);
            for (float sx : {-1.0f,1.0f}) {
                glm::mat4 rail=glm::translate(top(),glm::vec3(sx*CH_WID*0.44f,CH_WR+0.38f,-0.10f));
                rail=glm::scale(rail,glm::vec3(0.08f,0.28f,CH_LEN*0.92f));
                setModel(rail); mBox.draw();
            }
            {
                glm::mat4 front=glm::translate(top(),glm::vec3(0,CH_WR+0.34f,CH_LEN*0.40f));
                front=glm::scale(front,glm::vec3(CH_WID*0.72f,0.22f,0.07f));
                setModel(front); mBox.draw();
            }
            {
                glm::mat4 back=glm::translate(top(),glm::vec3(0,CH_WR+0.42f,-CH_LEN*0.38f));
                back=glm::scale(back,glm::vec3(CH_WID*0.72f,0.32f,0.07f));
                setModel(back); mBox.draw();
            }
            glUniform1i(uUseTex, 0);

            setMaterial({0.40f,0.27f,0.14f}, {0.12f,0.10f,0.08f}, 14.0f, sky.ambStr);
            {
                glm::mat4 yoke=glm::translate(top(),glm::vec3(0,CH_WR+0.14f,CH_LEN*0.82f));
                yoke=glm::scale(yoke,glm::vec3(0.16f,0.07f,CH_LEN*1.15f));
                setModel(yoke); mBox.draw();
            }

            setTexMaterial(texWood, glm::vec2(0.55f,0.55f), {0.10f,0.10f,0.10f}, 12.0f, sky.ambStr);
            // glUniform1f(uAmbi, sky.ambStr);
            for (float sx : {-1.0f,1.0f}) {
                glm::mat4 wm=glm::translate(top(),glm::vec3(sx*CH_WID*0.58f,CH_WR,-CH_LEN*0.10f));
                wm=glm::rotate(wm,(float)M_PI*0.5f,glm::vec3(0,0,1));
                wm=glm::rotate(wm,wheelRot,glm::vec3(0,1,0));
                wm=glm::scale(wm,glm::vec3(CH_WR*2,CH_WW,CH_WR*2));
                setModel(wm); mCylinder.draw();

                glm::mat4 hub=glm::translate(top(),glm::vec3(sx*CH_WID*0.58f,CH_WR,-CH_LEN*0.10f));
                hub=glm::scale(hub,glm::vec3(CH_WR*0.55f,CH_WW*1.2f,CH_WR*0.55f));
                setModel(hub); mSphere.draw();
            }
            glUniform1i(uUseTex, 0);

            setMaterial({0.62f,0.58f,0.52f},{0.70f,0.70f,0.70f},72.0f, sky.ambStr);
            for (float sx : {-1.0f,1.0f}) {
                for (int sp=0; sp<6; sp++) {
                    glm::mat4 spoke=glm::translate(top(),glm::vec3(sx*CH_WID*0.58f,CH_WR,-CH_LEN*0.10f));
                    spoke=glm::rotate(spoke,glm::radians(60.0f*sp) + wheelRot,glm::vec3(1,0,0));
                    spoke=glm::rotate(spoke,glm::radians(90.0f),glm::vec3(0,0,1));
                    spoke=glm::translate(spoke,glm::vec3(0,CH_WR*0.34f,0));
                    spoke=glm::scale(spoke,glm::vec3(0.045f,CH_WR*0.68f,0.030f));
                    setModel(spoke); mBox.draw();
                }
            }

            glm::vec3 lanternEmit = headlightsOn
                                  ? glm::vec3(1.25f,0.95f,0.60f)
                                  : glm::vec3(0.03f,0.02f,0.01f);
            setMaterial({0.98f,0.80f,0.50f},{0,0,0},1.0f,1.0f,lanternEmit);
            for (float sx : {-1.0f,1.0f}) {
                glm::mat4 lan=glm::translate(top(),glm::vec3(sx*CH_WID*0.33f,CH_WR+0.46f,CH_LEN*0.52f));
                lan=glm::scale(lan,glm::vec3(0.12f,0.18f,0.12f));
                setModel(lan); mBox.draw();
            }
        }

        popM(); /* car */
    }

    /* ══════════════════════════════════════════════
     *  CAR BOUNDING BOX (blue wireframe, cheat: box)
     * ══════════════════════════════════════════════ */
    if (showBoundingBox) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        /* Draw the car footprint as a flat thin box at body height */
        float boxY = WHL_R + CAR_BH * 0.5f;
        setMaterial({0.0f, 0.2f, 1.0f}, {0,0,0}, 1.0f, 1.0f,
                    {0.0f, 0.3f, 1.8f});
        {
            glm::mat4 bm = glm::translate(top(), carPos + glm::vec3(0, boxY, 0));
            bm = glm::rotate(bm, carHeading, glm::vec3(0,1,0));
            bm = glm::scale(bm, glm::vec3(CAR_W, CAR_BH + CAR_CH, CAR_L));
            setModel(bm); mBox.draw();
        }
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}
