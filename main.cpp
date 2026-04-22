/* =====================================================================
 *  main.cpp — Entry point: window, GLFW/GLEW init, main loop
 *
 *  Controls
 *  ────────────────────────────────────────────────────────────────────
 *    ↑ / ↓              Accelerate / decelerate car
 *    ← / →              Steer car left / right
 *    f / s              Faster / slower (alternative)
 *    l / r              Steer left / right (alternative, per spec)
 *    a / d              Swivel ground camera (view 3 only)
 *    w / Shift+W        Increase / decrease fan spin speed
 *    1 – 5              Switch camera view
 *    b                  Toggle bullet-time (slow-motion)
 *    x                  Reset world
 *    Escape             Quit
 * ===================================================================== */
#include "gl_common.h"

#include <iostream>

#include "constants.h"
#include "globals.h"
#include "world.h"
#include "input.h"
#include "update.h"
#include "render.h"

int main() {
    /* ── GLFW init ── */
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);   /* 4× MSAA */

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, WIN_TITLE,
                                       nullptr, nullptr);
    if (!win) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, keyCallback);
    glfwSetFramebufferSizeCallback(win, framebufferCB);
    glfwSwapInterval(1);    /* vsync */

    /* ── GLEW init ── */
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        return 1;
    }

    std::cout << "OpenGL  : " << glGetString(GL_VERSION)  << "\n";
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";

    /* ── Scene init ── */
    if (!initGL()) {
        std::cerr << "Scene init failed\n";
        return 1;
    }
    resetWorld();

    /* ── Main loop ── */
    float lastT = (float)glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        float dt  = now - lastT;
        lastT     = now;
        if (dt > 0.1f) dt = 0.1f;      /* clamp large spikes */

        glfwPollEvents();
        update(dt);
        render();
        glfwSwapBuffers(win);
    }

    /* ── Cleanup ── */
    glDeleteProgram(shaderProg);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
