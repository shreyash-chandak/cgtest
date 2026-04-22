#pragma once
/* =====================================================================
 *  update.h — Per-frame physics and animation update
 * ===================================================================== */

/* Advance all simulation state by dt seconds.
   Respects the bullet-time flag (slow-motion factor). */
void update(float dt);
