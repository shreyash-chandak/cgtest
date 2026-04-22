#pragma once
/* =====================================================================
 *  render.h — Scene rendering (all draw calls)
 * ===================================================================== */

/* Render one complete frame: sky, ground, road, walls, buildings,
   fans, gimbal lights, and car.  Call after update(). */
void render();
