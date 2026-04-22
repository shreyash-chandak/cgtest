/* ===================================================================
 *  vertex.glsl — Vertex Shader (GLSL 330 core)
 *  Transforms geometry to clip space and prepares interpolants for
 *  fragment-stage Blinn-Phong lighting.
 * =================================================================== */
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

/* Transformation matrices supplied by the CPU each draw call */
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;          /* transpose(inverse(mat3(model))) */

/* Outputs to fragment shader */
out vec3 FragPos;                   /* world-space position            */
out vec3 Normal;                    /* world-space normal              */
out vec2 TexCoord;                  /* texture coordinate              */

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;
    Normal        = normalize(normalMatrix * aNormal);
    TexCoord      = aTexCoord;
    gl_Position   = projection * view * worldPos;
}
