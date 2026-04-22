#pragma once
/* =====================================================================
 *  camera.h — Five switchable camera views
 *
 *  View 0 (key 1)  Sky view        — top-down, centred on arena
 *  View 1 (key 2)  Car view        — roof-mounted, looks forward
 *  View 2 (key 3)  Ground view     — fixed near building 0, A/D to swivel
 *  View 3 (key 4)  Light-source    — rides the swinging gimbal of building 0
 *  View 4 (key 5)  Helicopter      — trails behind car at fixed offset
 * ===================================================================== */
#include "gl_common.h"

/* Compute and upload view + projection matrices for the current camMode.
   Also writes eyePos for the lighting viewPos uniform. */
void getCamera(glm::mat4& viewOut, glm::vec3& eyeOut);
