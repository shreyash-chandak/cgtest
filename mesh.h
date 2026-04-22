#pragma once
/* =====================================================================
 *  mesh.h — CPU-side mesh generation for all primitives
 * ===================================================================== */
#include "types.h"

/* Unit box (−0.5…+0.5 on each axis) */
Mesh createBox();

/* Open cylinder shell (no caps): radius 0.5, height 1, Y-axis */
Mesh createCylinder(int slices = 32);

/* Closed cylinder (with caps): radius 0.5, height 1 */
Mesh createCylinderCapped(int slices = 32);

/* UV sphere: radius 0.5, centred at origin */
Mesh createSphere(int slices = 16, int stacks = 12);

/* Circular disk at y=0, radius = ARENA_R, textured as cobblestone */
Mesh createGround();

/* Colosseum wall ring: hollow thick cylinder with arched-window detail */
Mesh createColosseum();

/* Elliptical road ring — outer track */
Mesh createRoad();

/* Elliptical road ring — inner track */
Mesh createRoadInner();

/* Helper: nearest point on the OUTER track centre-line to world point p */
#include <glm/glm.hpp>
glm::vec3 nearestTrack(glm::vec3 p);

/* Helper: nearest point on the INNER track centre-line to world point p */
glm::vec3 nearestTrackInner(glm::vec3 p);