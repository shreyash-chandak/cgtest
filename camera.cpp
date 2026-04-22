/* =====================================================================
 *  camera.cpp - Camera system: six switchable views
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

    glm::vec3 fwd(sinf(carHeading), 0.0f, cosf(carHeading));

    glm::mat4 view(1.0f);
    glm::vec3 eye(0.0f);

    switch (camMode) {
        default:
        case 0:
            eye = glm::vec3(0.0f, 55.0f, 0.0f);
            view = glm::lookAt(eye,
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, -1.0f));
            break;

        case 1: {
            float roofY = useChariot ? 1.45f : (WHL_R + CAR_BH + CAR_CH + 0.12f);
            eye = carPos + glm::vec3(0.0f, roofY, 0.0f) - fwd * (useChariot ? 0.62f : 0.46f);
            glm::vec3 target = carPos + fwd * 6.2f + glm::vec3(0.0f, useChariot ? 0.55f : 0.45f, 0.0f);
            view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        case 2: {
            glm::vec3 base = bldg[0].pos
                           + bldg[0].toRoad * (B_HALF + 0.5f)
                           + glm::vec3(0.0f, 3.0f, 0.0f);

            float yaw = bldg[0].roadYaw + glm::radians(gndSwivel);
            glm::vec3 lookDir(sinf(yaw), -0.2f, cosf(yaw));

            eye = base;
            view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        case 3: {
            const int idx = 0;
            eye = spotPos[idx];
            glm::vec3 dir = glm::length(spotDir[idx]) > 0.0001f
                          ? glm::normalize(spotDir[idx])
                          : glm::vec3(0.0f, -1.0f, 0.0f);
            float yDir = std::min(dir.y, -0.02f);
            float tHit = (0.03f - eye.y) / yDir;
            tHit = std::clamp(tHit, 0.8f, COL_H * 3.5f);
            glm::vec3 roadHit = eye + dir * tHit;
            view = glm::lookAt(eye, roadHit, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }

        case 4:
            eye = carPos - fwd * 8.0f + glm::vec3(0.0f, 5.0f, 0.0f);
            view = glm::lookAt(eye,
                               carPos + glm::vec3(0.0f, 1.0f, 0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
            break;

        case 5: {
            float cLen = glm::length(freeCamCenterXZ);
            if (cLen > FREE_CAM_MAX_RAD && cLen > 0.0001f) {
                freeCamCenterXZ = (freeCamCenterXZ / cLen) * FREE_CAM_MAX_RAD;
            }

            float zoom = std::clamp(freeCamZoom, FREE_CAM_MIN_ZOOM, FREE_CAM_MAX_ZOOM);
            float yaw = glm::radians(freeCamYawDeg);
            float pitch = glm::radians(std::clamp(freeCamPitchDeg, 18.0f, 85.0f));

            glm::vec3 target(freeCamCenterXZ.x, 0.6f, freeCamCenterXZ.y);
            glm::vec3 offset(cosf(pitch) * sinf(yaw),
                             sinf(pitch),
                             cosf(pitch) * cosf(yaw));

            eye = target + offset * zoom;
            view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }
    }

    viewOut = view;
    eyeOut = eye;

    glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3fv(uViewPos, 1, glm::value_ptr(eye));
}
