/* =====================================================================
 *  camera.cpp — Camera system: five switchable views
 *
 *  Key fix (view 3 — Light-source cam):
 *  The original code used glm::lookAt(spotPos, spotPos+spotDir, {0,1,0}).
 *  When the spotlight points nearly straight down the world-up vector
 *  ({0,1,0}) is nearly parallel to spotDir, causing glm::lookAt to
 *  produce a degenerate matrix (the camera "rolls" wildly or flips).
 *
 *  Fix: we store the full gimbal rotation matrix (spotGimbalMat) in
 *  update.cpp for each building.  The matrix's local Y column (column 1)
 *  gives the correct up-vector that rotates with the gimbal — it is
 *  always perpendicular to spotDir and never collapses to zero.
 * ===================================================================== */
#include "gl_common.h"
#include "camera.h"
#include "globals.h"
#include "constants.h"

#include <cmath>
#include <algorithm>

void getCamera(glm::mat4& viewOut, glm::vec3& eyeOut) {
    float aspect = (float)WIN_W / (float)std::max(WIN_H, 1);
    glm::mat4 proj = glm::perspective(glm::radians(CAM_FOV), aspect,
                                       CAM_NEAR, CAM_FAR);
    glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));

    /* Car forward direction in world space */
    glm::vec3 fwd(sinf(carHeading), 0.0f, cosf(carHeading));

    glm::mat4 view(1.0f);
    glm::vec3 eye(0.0f);

    switch (camMode) {

        /* ── 0: Sky view — wide top-down showing full arena with padding ── */
        default:
        case 0:
            eye  = glm::vec3(0.0f, 55.0f, 0.0f);
            view = glm::lookAt(eye,
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, -1.0f));
            break;

        /* ── 1: Car view — roof-mounted, looks forward ── */
        case 1: {
            float roofY = useChariot ? 1.45f : (WHL_R + CAR_BH + CAR_CH + 0.12f);
            eye  = carPos + glm::vec3(0.0f, roofY, 0.0f) - fwd * (useChariot ? 0.62f : 0.46f);
            glm::vec3 target = carPos + fwd * 6.2f + glm::vec3(0.0f, useChariot ? 0.55f : 0.45f, 0.0f);
            view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        /* ── 2: Ground view — stationary near building 0, A/D swivels ── */
        case 2: {
            /* Camera sits 3 m above the building's road-facing wall */
            glm::vec3 base = bldg[0].pos
                           + bldg[0].toRoad * (B_HALF + 0.5f)
                           + glm::vec3(0.0f, 3.0f, 0.0f);

            float yaw = bldg[0].roadYaw + glm::radians(gndSwivel);
            glm::vec3 lookDir(sinf(yaw), -0.2f, cosf(yaw));

            eye  = base;
            view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        /* ── 3: Light-source view — camera sits at the bulb, looks toward the wall ── */
        case 3: {
            /* Pick the building whose spotlight normal best faces the wall.
               'toWall' is the radial outward direction from centre through the bulb. */
            int best = 0;
            float bestScore = -1e9f;
            for (int i = 0; i < NUM_B; i++) {
                glm::vec3 radial(spotPos[i].x, 0.0f, spotPos[i].z);
                float rLen = glm::length(radial);
                if (rLen < 0.001f) continue;
                radial /= rLen;
                /* Prefer gimbals whose beam is NOT aimed at the wall
                   (so the perpendicular view toward the wall is clear). */
                float score = rLen - fabsf(glm::dot(radial, spotDir[i])) * 5.0f;
                if (score > bestScore) {
                    bestScore = score;
                    best = i;
                }
            }
            /* Look radially outward from bulb toward the wall */
            glm::vec3 outward(spotPos[best].x, 0.0f, spotPos[best].z);
            float oLen = glm::length(outward);
            if (oLen > 0.001f) outward /= oLen;
            else outward = glm::vec3(1.0f, 0.0f, 0.0f);

            eye = spotPos[best];
            glm::vec3 target = eye + outward * 10.0f;
            /* Use world-up since outward is horizontal */
            view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        /* ── 4: Helicopter cam — fixed offset behind and above car ── */
        case 4:
            eye  = carPos - fwd * 8.0f + glm::vec3(0.0f, 5.0f, 0.0f);
            view = glm::lookAt(eye,
                               carPos + glm::vec3(0.0f, 1.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
            break;
    }

    viewOut = view;
    eyeOut  = eye;

    glUniformMatrix4fv(uView,    1, GL_FALSE, glm::value_ptr(view));
    glUniform3fv      (uViewPos, 1, glm::value_ptr(eye));
}
