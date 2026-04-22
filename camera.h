#pragma once
/* =====================================================================
 *  camera.h - Six switchable camera views
 *
 *  View 0 (key 1)  Sky view
 *  View 1 (key 2)  Car view
 *  View 2 (key 3)  Ground view
 *  View 3 (key 4)  Swinging light source view
 *  View 4 (key 5)  Helicopter view
 *  View 5 (key 6)  Free camera (mouse drag + scroll zoom)
 * ===================================================================== */
#include "gl_common.h"

void getCamera(glm::mat4& viewOut, glm::vec3& eyeOut);
