#pragma once
/* =====================================================================
 *  world.h — World initialisation, reset, and collision detection
 * ===================================================================== */
#include "gl_common.h"

/* Initialise OpenGL resources (textures, meshes, shaders, buildings) */
bool initGL();

/* Reset all mutable world state to starting conditions */
void resetWorld();

/* Return true if the car at (pos, heading) intersects walls or buildings */
bool checkCollision(glm::vec3 pos, float heading);
