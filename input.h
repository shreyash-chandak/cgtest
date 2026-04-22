#pragma once
/* =====================================================================
 *  input.h — GLFW keyboard callback and framebuffer resize callback
 *
 *  Controls
 *  ────────────────────────────────────────────────────────────────────
 *    ↑ / ↓              Accelerate / decelerate car
 *    ← / →              Steer car left / right
 *    f / s              Also faster / slower (alternative)
 *    a / d              Swivel ground camera (view 3 only) ± 30°
 *    w / Shift+W        Increase / decrease fan spin speed
 *    1 – 5              Switch camera view
 *    h                  Toggle car headlights
 *    b                  Toggle bullet-time (slow-motion)
 *    x                  Reset world
 *    Escape             Quit
 * ===================================================================== */
#include "gl_common.h"

void keyCallback(GLFWwindow* w, int key, int sc, int action, int mods);
void charCallback(GLFWwindow* w, unsigned int codepoint);
void framebufferCB(GLFWwindow* w, int width, int height);