/* ===================================================================
 *  fragment.glsl — Fragment Shader (GLSL 330 core)
 *  Blinn-Phong lighting with:
 *    • 1 directional sun (colour + strength driven by time-of-day)
 *    • Up to MAX_LIGHTS coloured spot/point lights
 *        slots 0-3   : building gimbal spotlights
 *        slots 4-5   : car headlights (spot cones)
 *        slots 6-7   : dipper lights (ground-facing)
 *        slots 8-10  : nearest torches (wide-cone point lights, night only)
 *    • Emissive term
 *    • Exponential fog  +  Reinhard tone-map
 * =================================================================== */
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

/* ---- Material ---- */
uniform vec3      objectColor;
uniform bool      useTexture;
uniform sampler2D diffuseTexture;
uniform vec2      texScale;
uniform vec3      specularColor;
uniform float     shininess;
uniform float     ambientStrength;
uniform vec3      emissiveColor;

/* ---- Camera ---- */
uniform vec3 viewPos;

/* ---- Sun ---- */
uniform vec3  sunDir;
uniform vec3  sunColor;
uniform float sunStrength;

/* ---- Point / spot lights ---- */
#define MAX_LIGHTS 25
uniform int   numLights;
uniform vec3  lightPos      [MAX_LIGHTS];
uniform vec3  lightColor    [MAX_LIGHTS];
uniform vec3  lightDirection[MAX_LIGHTS]; /* spot axis; ignored when cutoff>=1 */
uniform float lightCutoff   [MAX_LIGHTS]; /* cos(half-angle); >=1.0 = point    */
uniform float lightStrength [MAX_LIGHTS]; /* per-light intensity multiplier     */

/* ---- Atmosphere ---- */
uniform vec3  fogColor;
uniform float fogDensity;

/* ================================================================== */
vec3 calcSun(vec3 N, vec3 V, vec3 base)
{
    vec3  L    = normalize(-sunDir);
    float diff = max(dot(N, L), 0.0);
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    float amb = ambientStrength * sunStrength;
    return amb * base * sunColor
         + sunStrength * diff * base * sunColor
         + sunStrength * spec * specularColor * sunColor;
}

/* ================================================================== */
vec3 calcLight(int i, vec3 N, vec3 V, vec3 base)
{
    vec3  toFrag = lightPos[i] - FragPos;
    float dist   = length(toFrag);
    vec3  L      = toFrag / dist;

    /* Physically-based quadratic attenuation */
    float atten  = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);

    /* Spot cone — if cutoff >= 1.0 treat as omnidirectional point light */
    float spot = 1.0;
    if (lightCutoff[i] < 1.0) {
        float theta      = dot(L, normalize(-lightDirection[i]));
        float outerCos   = lightCutoff[i] - 0.15;
        spot = clamp((theta - outerCos) / (lightCutoff[i] - outerCos), 0.0, 1.0);
    }

    float diff = max(dot(N, L), 0.0);
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    float I = lightStrength[i];
    return atten * spot * I * (diff * base * lightColor[i]
                             + spec * specularColor * lightColor[i]);
}

/* ================================================================== */
void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 base = useTexture
              ? texture(diffuseTexture, TexCoord * texScale).rgb
              : objectColor;

    vec3 result = calcSun(N, V, base);

    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
        result += calcLight(i, N, V, base);

    result += emissiveColor;

    float camDist  = length(viewPos - FragPos);
    float fogBlend = clamp(exp(-fogDensity * camDist), 0.0, 1.0);
    result = mix(fogColor, result, fogBlend);

    result = result / (result + vec3(1.0));
    FragColor = vec4(result, 1.0);
}
