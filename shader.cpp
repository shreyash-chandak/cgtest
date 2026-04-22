/* =====================================================================
 *  shader.cpp — Shader compilation, linking, and material helpers
 * ===================================================================== */
#include "gl_common.h"
#include "shader.h"
#include "globals.h"

#include <fstream>
#include <sstream>
#include <iostream>

/* ── Embedded GLSL fallbacks ── */
const char* VS_FALLBACK = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
uniform mat4 model, view, projection;
uniform mat3 normalMatrix;
out vec3 FragPos, Normal;
out vec2 TexCoord;
void main(){
    vec4 wp = model*vec4(aPos,1.0);
    FragPos  = wp.xyz;
    Normal   = normalize(normalMatrix*aNormal);
    TexCoord = aTexCoord;
    gl_Position = projection*view*wp;
})";

const char* FS_FALLBACK = R"(
#version 330 core
in vec3 FragPos, Normal;
in vec2 TexCoord;
out vec4 FragColor;
uniform vec3 objectColor, specularColor, emissiveColor, viewPos;
uniform bool useTexture;
uniform sampler2D diffuseTexture;
uniform vec2 texScale;
uniform float shininess, ambientStrength;
uniform vec3 sunDir, sunColor, fogColor;
uniform float sunStrength, fogDensity;
#define MAX_LIGHTS 5
uniform int numLights;
uniform vec3 lightPos[MAX_LIGHTS], lightColor[MAX_LIGHTS], lightDirection[MAX_LIGHTS];
uniform float lightCutoff[MAX_LIGHTS];
uniform float lightStrength[MAX_LIGHTS];
void main(){
    vec3 N=normalize(Normal), V=normalize(viewPos-FragPos);
    vec3 base=useTexture?texture(diffuseTexture,TexCoord*texScale).rgb:objectColor;
    vec3 L=normalize(-sunDir);
    float diff=max(dot(N,L),0.0);
    vec3 H=normalize(L+V);
    float spec=pow(max(dot(N,H),0.0),shininess);
    float amb=ambientStrength*sunStrength;
    vec3 res=amb*base*sunColor + sunStrength*diff*base*sunColor + sunStrength*spec*specularColor*sunColor;
    for(int i=0;i<numLights&&i<MAX_LIGHTS;i++){
        vec3 fl=lightPos[i]-FragPos; float d=length(fl); vec3 ld=fl/d;
        float at=1.0/(1.0+0.09*d+0.032*d*d);
        float th=dot(ld,normalize(-lightDirection[i]));
        float outer=lightCutoff[i]-0.18;
        float sp=clamp((th-outer)/(lightCutoff[i]-outer),0.0,1.0);
        float df=max(dot(N,ld),0.0);
        vec3 hh=normalize(ld+V);
        float ss=pow(max(dot(N,hh),0.0),shininess);
        res+=at*sp*df*base*lightColor[i]+at*sp*ss*specularColor*lightColor[i];
    }
    res+=emissiveColor;
    float cd=length(viewPos-FragPos);
    float ff=clamp(exp(-fogDensity*cd),0.0,1.0);
    res=mix(fogColor,res,ff);
    res=res/(res+vec3(1.0));
    FragColor=vec4(res,1.0);
})";

/* ────────────────────────────────────────────────── */

std::string readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return s;
}

GLuint buildProgram(const char* vSrc, const char* fSrc) {
    GLuint v = compileShader(GL_VERTEX_SHADER,   vSrc);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(p, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void cacheUniforms() {
    auto L = [](const char* n){ return glGetUniformLocation(shaderProg, n); };
    uModel = L("model"); uView = L("view"); uProj = L("projection");
    uNormMat = L("normalMatrix");
    uObjCol = L("objectColor"); uUseTex = L("useTexture");
    uTexScale = L("texScale");  uSpecCol = L("specularColor");
    uShine = L("shininess");    uAmbi = L("ambientStrength");
    uEmissive = L("emissiveColor"); uViewPos = L("viewPos");
    uSunDir = L("sunDir"); uSunCol = L("sunColor"); uSunStr = L("sunStrength");
    uNumLights = L("numLights");
    uFogCol = L("fogColor"); uFogDen = L("fogDensity");
    for (int i = 0; i < MAX_LIGHTS; i++) {
        std::string b;
        b = "lightPos["      + std::to_string(i) + "]"; uLightPos[i] = L(b.c_str());
        b = "lightColor["    + std::to_string(i) + "]"; uLightCol[i] = L(b.c_str());
        b = "lightDirection["+ std::to_string(i) + "]"; uLightDir[i] = L(b.c_str());
        b = "lightCutoff["   + std::to_string(i) + "]"; uLightCut[i] = L(b.c_str());
        b = "lightStrength[" + std::to_string(i) + "]"; uLightStr[i] = L(b.c_str());
    }
}

void setModel(const glm::mat4& m) {
    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m));
    glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
    glUniformMatrix3fv(uNormMat, 1, GL_FALSE, glm::value_ptr(nm));
}

void setMaterial(glm::vec3 col, glm::vec3 spec, float shine,
                 float ambi, glm::vec3 emit) {
    glUniform3fv(uObjCol,   1, glm::value_ptr(col));
    glUniform1i (uUseTex,   0);
    glUniform3fv(uSpecCol,  1, glm::value_ptr(spec));
    glUniform1f (uShine,    shine);
    glUniform1f (uAmbi,     ambi);
    glUniform3fv(uEmissive, 1, glm::value_ptr(emit));
    glUniform2f (uTexScale, 1.0f, 1.0f);
}

void setTexMaterial(GLuint tex, glm::vec2 scale, glm::vec3 spec, float shine) {
    glUniform1i (uUseTex,  1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform2fv(uTexScale, 1, glm::value_ptr(scale));
    glUniform3fv(uSpecCol,  1, glm::value_ptr(spec));
    glUniform1f (uShine,    shine);
    glUniform1f (uAmbi,     0.15f);
    glUniform3fv(uEmissive, 1, glm::value_ptr(glm::vec3(0)));
}
