/* =====================================================================
 *  input.cpp — GLFW keyboard and character callbacks
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
 *    /                  Enter cheat mode (clears buffer); type cheat, Enter submits
 *    Escape             Cancel cheat / quit
 *
 *  Cheat codes (type after '/', then press Enter)
 *  ────────────────────────────────────────────────────────────────────
 *    headlight   Toggle headlights
 *    changecar   Switch between sports car and chariot
 *    time        Toggle between day (noon) and night
 *    super       Toggle building collision off/on
 *    box         Toggle car bounding box display
 * ===================================================================== */
#include "gl_common.h"
#include "input.h"
#include "globals.h"
#include "constants.h"
#include "world.h"

#include <algorithm>
#include <cmath>

/* ── Update window title to reflect cheat buffer ── */
static void updateTitle(GLFWwindow* w) {
    if (cheatMode) {
        std::string title = std::string("CHEAT> ") + cheatBuffer;
        glfwSetWindowTitle(w, title.c_str());
    } else {
        glfwSetWindowTitle(w, WIN_TITLE);
    }
}

/* ── Execute a submitted cheat string ── */
static void processCheat(const std::string& code) {
    if (code == "headlight") {
        headlightsOn = !headlightsOn;
    } else if (code == "changecar") {
        useChariot = !useChariot;
    } else if (code == "time") {
        /* Jump globalTime to middle of night or noon period */
        float cycleDur   = PERIOD_DUR * NUM_PERIODS;
        float curCycleBase = globalTime - fmodf(globalTime, cycleDur);
        if (todPeriod >= 2) {
            /* currently evening/night → jump to noon (period 1, 50%) */
            globalTime = curCycleBase + PERIOD_DUR * 1.5f;
        } else {
            /* currently day → jump to night (period 3, 10% in) */
            globalTime = curCycleBase + PERIOD_DUR * 3.1f;
        }
    } else if (code == "super") {
        superMode = !superMode;
    } else if (code == "box") {
        showBoundingBox = !showBoundingBox;
    }
}

/* =====================================================================
 *  keyCallback — handles key presses for movement, camera, and cheats
 * ===================================================================== */
void keyCallback(GLFWwindow* w, int key, int /*sc*/, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    /* ── Cheat mode: most letter keys feed into the buffer via charCallback.
       Only special/navigation keys are handled here. ── */
    if (cheatMode) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                cheatMode = false;
                cheatBuffer.clear();
                updateTitle(w);
                break;
            case GLFW_KEY_ENTER:
            case GLFW_KEY_KP_ENTER:
                processCheat(cheatBuffer);
                cheatMode = false;
                cheatBuffer.clear();
                updateTitle(w);
                break;
            case GLFW_KEY_BACKSPACE:
                if (!cheatBuffer.empty()) cheatBuffer.pop_back();
                updateTitle(w);
                break;
            case GLFW_KEY_SLASH:
                /* '/' while in cheat mode: clear buffer, stay in cheat mode */
                cheatBuffer.clear();
                updateTitle(w);
                break;
            /* Arrow keys still drive the car during cheat entry */
            case GLFW_KEY_UP:
                if (!carFrozen) carSpeed = std::min(carSpeed + SPD_INC, MAX_SPD);
                break;
            case GLFW_KEY_DOWN:
                if (!carFrozen) carSpeed = std::max(carSpeed - SPD_INC, -MAX_SPD);
                break;
            case GLFW_KEY_LEFT:
                if (!carFrozen) carHeading += glm::radians(STR_DEG);
                break;
            case GLFW_KEY_RIGHT:
                if (!carFrozen) carHeading -= glm::radians(STR_DEG);
                break;
            /* Camera switches still work */
            case GLFW_KEY_1: camMode = 0; break;
            case GLFW_KEY_2: camMode = 1; break;
            case GLFW_KEY_3: camMode = 2; break;
            case GLFW_KEY_4: camMode = 3; break;
            case GLFW_KEY_5: camMode = 4; break;
            default: break;
        }
        return;
    }

    /* ── Normal mode ── */
    switch (key) {

        /* ── Activate cheat mode ── */
        case GLFW_KEY_SLASH:
            cheatMode = true;
            cheatBuffer.clear();
            updateTitle(w);
            break;

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
        case GLFW_KEY_1: camMode = 0; break;
        case GLFW_KEY_2: camMode = 1; break;
        case GLFW_KEY_3: camMode = 2; break;
        case GLFW_KEY_4: camMode = 3; break;
        case GLFW_KEY_5: camMode = 4; break;

        /* ── Bullet time ── */
        case GLFW_KEY_B:
            bulletTime = !bulletTime;
            break;

        /* ── Reset ── */
        case GLFW_KEY_X:
            resetWorld();
            updateTitle(w);
            break;

        /* ── Quit ── */
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(w, GLFW_TRUE);
            break;

        default: break;
    }
}

/* =====================================================================
 *  charCallback — feeds typed characters into cheat buffer
 * ===================================================================== */
void charCallback(GLFWwindow* w, unsigned int codepoint) {
    if (!cheatMode) return;
    /* Only accept a-z / A-Z (cheat codes are pure alpha).
       This also naturally blocks '/' (the activator) from leaking into
       the buffer when GLFW fires charCallback right after keyCallback
       sets cheatMode=true on the same keypress. */
    char c = (char)codepoint;
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if (c < 'a' || c > 'z') return;
    cheatBuffer += c;
    updateTitle(w);
}

void framebufferCB(GLFWwindow*, int w, int h) {
    WIN_W = w;
    WIN_H = h;
    glViewport(0, 0, w, h);
}
