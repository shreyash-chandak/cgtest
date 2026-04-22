#pragma once
/* =====================================================================
 *  shader.h — Shader compilation and uniform helpers
 * ===================================================================== */
#include "gl_common.h"
#include <glm/glm.hpp>
#include <string>

/* Read a file into a string (returns "" on failure) */
std::string readFile(const char* path);

/* Compile a single shader stage */
GLuint compileShader(GLenum type, const char* src);

/* Link a vertex + fragment shader into a programme */
GLuint buildProgram(const char* vSrc, const char* fSrc);

/* Cache all uniform locations after linking */
void cacheUniforms();

/* Set model matrix and auto-compute the normal matrix */
void setModel(const glm::mat4& m);

/* Set a flat-colour material (no texture) */
void setMaterial(glm::vec3 col, glm::vec3 spec, float shine,
                 float ambi = 0.15f, glm::vec3 emit = glm::vec3(0));

/* Set a textured material */
void setTexMaterial(GLuint tex, glm::vec2 scale, glm::vec3 spec, float shine,
                    float ambi = 0.15f);

/* Embedded GLSL fallbacks (used when external files are missing) */
extern const char* VS_FALLBACK;
extern const char* FS_FALLBACK;
