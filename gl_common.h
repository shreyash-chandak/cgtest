#pragma once
/* =====================================================================
 *  gl_common.h — Must be included FIRST in every translation unit
 *                that touches OpenGL or GLFW.
 *
 *  GLEW must be included before gl.h / glfw3.h or the compiler will
 *  error with "gl.h included before glew.h".  This header enforces
 *  that ordering in one place.
 * ===================================================================== */
#define GLM_FORCE_RADIANS
#include <GL/glew.h>       /* MUST come before any other GL header */
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
