#pragma once
/* =====================================================================
 *  texture.h — Procedural texture generation and upload
 * ===================================================================== */
#include "gl_common.h"

/* Upload an RGB pixel buffer to a new GL texture */
GLuint uploadTex(unsigned char* data, int w, int h);

/* Load an image texture from disk (PNG/JPEG/etc). Returns 0 on failure. */
GLuint loadImageTex(const char* path, bool flipY = true);

/* Fill a W×H RGB buffer with a brick pattern */
void genBrickTex(unsigned char* d, int W, int H);

/* Fill a W×H RGB buffer with a wood-grain pattern */
void genWoodTex(unsigned char* d, int W, int H);

/* Fill a W×H RGB buffer with a concrete pattern */
void genConcreteTex(unsigned char* d, int W, int H);

/* Fill a W×H RGB buffer with a cut-stone (sandstone) pattern */
void genStoneTex(unsigned char* d, int W, int H);

/* Fill a W×H RGB buffer with a cobblestone pattern */
void genCobbleTex(unsigned char* d, int W, int H);
