/* =====================================================================
 *  input.cpp — GLFW keyboard and framebuffer-resize callbacks
 *
 *  Controls
 *  ────────────────────────────────────────────────────────────────────
 *    ↑ / ↓              Accelerate / decelerate car
 *    ← / →              Steer car left / right
 *    f / s              Also faster / slower (alternative)
 *    a / d              Swivel ground camera (view 3) left / right
 *    w / Shift+W        Increase / decrease fan spin speed
 *    1 – 5              Switch camera view
 *    b                  Toggle bullet-time
 *    x                  Reset world
 *    Escape             Quit
 * ===================================================================== */
#include "gl_common.h"
#include "input.h"
#include "globals.h"
#include "constants.h"
#include "world.h"

#include <algorithm>
#include <cmath>

void keyCallback(GLFWwindow* w, int key, int /*sc*/, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {

        /* ── Car: accelerate / decelerate ── */
        case GLFW_KEY_UP:
        case GLFW_KEY_F:
            if (!carFrozen)
                carSpeed = std::min(carSpeed + SPD_INC, MAX_SPD);
            break;

        case GLFW_KEY_DOWN:
        case GLFW_KEY_S:
            if (!carFrozen)
                carSpeed = std::max(carSpeed - SPD_INC, -MAX_SPD);
            break;

        /* ── Car: steer ── */
        case GLFW_KEY_LEFT:
            if (!carFrozen)
                carHeading += glm::radians(STR_DEG);
            break;

        case GLFW_KEY_RIGHT:
            if (!carFrozen)
                carHeading -= glm::radians(STR_DEG);
            break;

        /* ── Legacy text keys for steering (assignment spec: l / r) ── */
        case GLFW_KEY_L:
            if (!carFrozen)
                carHeading += glm::radians(STR_DEG);
            break;

        case GLFW_KEY_R:
            if (!carFrozen)
                carHeading -= glm::radians(STR_DEG);
            break;

        /* ── Ground camera swivel (view 3 only) — A / D ── */
        case GLFW_KEY_A:
            gndSwivel = std::min(gndSwivel + 3.0f,  GND_SWIV);
            break;

        case GLFW_KEY_D:
            gndSwivel = std::max(gndSwivel - 3.0f, -GND_SWIV);
            break;

        /* ── Fan / windmill speed — W / Shift+W ── */
        case GLFW_KEY_W:
            if (mods & GLFW_MOD_SHIFT)
                fanSpeed = std::max(0.0f, fanSpeed - FAN_SINC);
            else
                fanSpeed += FAN_SINC;
            break;

        /* ── Camera views ── */
        case GLFW_KEY_1: camMode = 0; break;   /* Sky         */
        case GLFW_KEY_2: camMode = 1; break;   /* Car roof    */
        case GLFW_KEY_3: camMode = 2; break;   /* Ground      */
        case GLFW_KEY_4: camMode = 3; break;   /* Light-source*/
        case GLFW_KEY_5: camMode = 4; break;   /* Helicopter  */

        /* ── Headlights toggle ── */
        case GLFW_KEY_H:
            headlightsOn = !headlightsOn;
            break;

        /* ── Bullet time ── */
        case GLFW_KEY_B:
            bulletTime = !bulletTime;
            break;

        /* ── Reset ── */
        case GLFW_KEY_X:
            resetWorld();
            break;

        /* ── Quit ── */
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(w, GLFW_TRUE);
            break;

        default: break;
    }
}

void framebufferCB(GLFWwindow*, int w, int h) {
    WIN_W = w;
    WIN_H = h;
    glViewport(0, 0, w, h);
}