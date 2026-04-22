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

    /* Fade lights near the *end* of evening/night, not at period start. */
    float periodTime = todFrac * PERIOD_DUR;
    float fadeStart  = PERIOD_DUR - PERIOD_FADE;
    if (todPeriod == 2) {
        lightsOn = (periodTime >= fadeStart)
                 ? std::min((periodTime - fadeStart) / PERIOD_FADE, 1.0f)
                 : 0.0f;
    } else if (todPeriod == 3) {
        lightsOn = (periodTime >= fadeStart)
                 ? std::max(1.0f - (periodTime - fadeStart) / PERIOD_FADE, 0.0f)
                 : 1.0f;
    } else {
        lightsOn = 0.0f;
    }

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
        float baseYaw   = bldg[i].roadYaw;
        float swing     = glm::radians(SW_MAX) * sinf(globalTime * SW_SPD + (float)i);
        float pitch     = glm::radians(78.0f); /* steep down-tilt toward floor */

        glm::mat4 M(1.0f);
        M = glm::translate(M, mount);
        M = glm::rotate(M, baseYaw + swing, glm::vec3(0,1,0));
        M = glm::rotate(M, pitch,           glm::vec3(1,0,0));
        spotGimbalMat[i] = M;
        spotPos[i] = glm::vec3(M * glm::vec4(0,0,LT_ARM,1));
        glm::vec3 nearestGround(spotPos[i].x, 0.03f, spotPos[i].z);
        spotDir[i] = glm::normalize(nearestGround - spotPos[i]);
    }
}
