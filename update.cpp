/* =====================================================================
 *  update.cpp — Per-frame physics, animation, and time-of-day
 *
 *  Time-of-day model
 *  ─────────────────
 *  Four discrete periods, each PERIOD_DUR seconds long.
 *  A PERIOD_FADE-second cross-fade happens at each boundary.
 *
 *   Period 0: Early morning — orange-pink sky, low warm sun, lights OFF
 *   Period 1: Noon          — bright blue sky, high sun, lights OFF
 *   Period 2: Evening       — amber/purple sky, setting sun, lights ON
 *   Period 3: Night         — dark sky, no sun, lights ON (full)
 *
 *  lightsOn: 0.0 (day) → 1.0 (night) — drives torch emissive and
 *            ambient boost; interpolated smoothly across the fade zone.
 * ===================================================================== */
#include "gl_common.h"
#include "update.h"
#include "globals.h"
#include "constants.h"
#include "world.h"

#include <cmath>
#include <algorithm>

void update(float dt) {
    float eff = bulletTime ? dt * BULLET_F : dt;
    globalTime += eff;

    /* ── Time-of-day ── */
    float cycle = fmodf(globalTime, PERIOD_DUR * NUM_PERIODS);
    todPeriod   = (int)(cycle / PERIOD_DUR) % NUM_PERIODS;
    todFrac     = fmodf(cycle, PERIOD_DUR) / PERIOD_DUR;  /* 0..1 within period */

    /* Compute lightsOn: smoothly transition at evening (period 2) start
       and back to off at early morning (period 0) start. */
    /* Periods 2 and 3 = lights on */
    float targetLights = (todPeriod >= 2) ? 1.0f : 0.0f;
    /* During the fade zone at the start of a period, interpolate */
    float fadeFrac = std::min(todFrac * PERIOD_DUR / PERIOD_FADE, 1.0f);
    if (todPeriod == 2 && fadeFrac < 1.0f)
        lightsOn = fadeFrac;             /* fading ON  as evening begins */
    else if (todPeriod == 0 && fadeFrac < 1.0f)
        lightsOn = 1.0f - fadeFrac;     /* fading OFF as morning begins */
    else
        lightsOn = targetLights;

    /* ── Fan rotation ── */
    fanAngle += glm::radians(fanSpeed) * eff;

    /* ── Car physics ── */
    if (!carFrozen && fabsf(carSpeed) > 0.001f) {
        glm::vec3 fwd(sinf(carHeading), 0.0f, cosf(carHeading));
        glm::vec3 next = carPos + fwd * carSpeed * eff;
        if (checkCollision(next, carHeading)) {
            carFrozen = true;
            carSpeed  = 0.0f;
        } else {
            carPos = next;
        }
        wheelRot += carSpeed * eff / WHL_R;
    }

    /* ── Building gimbal lights ── */
    for (int i = 0; i < NUM_B; i++) {
        float bldgH = bldg[i].stories * STORY_H;
        glm::vec3 mount = bldg[i].pos + glm::vec3(0.0f, bldgH + 0.3f, 0.0f);
        glm::vec3 toRd  = bldg[i].nearRoad - mount;
        float baseYaw   = atan2f(toRd.x, toRd.z);
        float pitch     = atan2f(-toRd.y, sqrtf(toRd.x*toRd.x + toRd.z*toRd.z));
        float swing     = glm::radians(SW_MAX) * sinf(globalTime * SW_SPD + (float)i);

        glm::mat4 M(1.0f);
        M = glm::translate(M, mount);
        M = glm::rotate(M, baseYaw + swing, glm::vec3(0,1,0));
        M = glm::rotate(M, pitch,           glm::vec3(1,0,0));
        spotGimbalMat[i] = M;
        spotPos[i] = glm::vec3(M * glm::vec4(0,0,LT_ARM,1));
        spotDir[i] = glm::normalize(glm::vec3(M * glm::vec4(0,0,1,0)));
    }
}