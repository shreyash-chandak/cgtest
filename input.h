#pragma once
/* =====================================================================
 *  input.h - GLFW input callbacks
 *
 *  Key controls include camera switches on keys 1..6.
 *  Mouse controls are used by free camera view (key 6):
 *  - Left drag to pan
 *  - Scroll to zoom
 * ===================================================================== */
#include "gl_common.h"

void keyCallback(GLFWwindow* w, int key, int sc, int action, int mods);
void charCallback(GLFWwindow* w, unsigned int codepoint);
void framebufferCB(GLFWwindow* w, int width, int height);
void cursorPosCB(GLFWwindow* w, double x, double y);
void mouseButtonCB(GLFWwindow* w, int button, int action, int mods);
void scrollCB(GLFWwindow* w, double xoff, double yoff);
