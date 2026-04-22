#pragma once
/* =====================================================================
 *  types.h — Shared data structures
 * ===================================================================== */
#include "gl_common.h"

/* ── Mesh: VAO + draw helper ── */
struct Mesh {
    GLuint vao   = 0;
    GLuint vbo   = 0;
    int    count = 0;

    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
    }
};

/* ── Per-building data ── */
struct BuildingInfo {
    glm::vec3 pos;          /* base centre world position              */
    int       stories;      /* building storeys (4+ in current setup)  */
    int       texType;      /* 0 = closed facade, 1 = gate facade      */
    glm::vec3 lightCol;     /* spotlight colour                        */
    /* Pre-computed at init: */
    glm::vec3 nearRoad;     /* closest point on track centre-line      */
    glm::vec3 toRoad;       /* horizontal unit vector toward road      */
    float     roadYaw;      /* atan2 of toRoad (for fan orientation)   */
};
