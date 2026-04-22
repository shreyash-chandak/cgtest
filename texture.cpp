/* =====================================================================
 *  texture.cpp — Procedural texture generation and GL upload
 * ===================================================================== */
#include "gl_common.h"
#include "texture.h"
#include <algorithm>
#include <cmath>

/* Simple integer hash → [0,1) */
static float hashNoise(int x, int y) {
    int n = x * 374761393 + y * 668265263;
    n = (n << 13) ^ n;
    return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff)
           / 2147483647.0f;
}

GLuint uploadTex(unsigned char* data, int w, int h) {
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,       GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,       GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,   GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,   GL_LINEAR);
    return t;
}

void genBrickTex(unsigned char* d, int W, int H) {
    const int BW = 32, BH = 16, MRT = 2;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int row = y / BH;
        int ox  = (row & 1) * (BW / 2);
        int bx  = (x + ox) % BW, by = y % BH;
        bool mortar = (bx < MRT || by < MRT);
        int i = (y * W + x) * 3;
        if (mortar) { d[i]=190; d[i+1]=185; d[i+2]=175; }
        else {
            float n = hashNoise(x, y) * 25.0f;
            d[i]   = (unsigned char)std::min(255.0f, 155 + n);
            d[i+1] = (unsigned char)std::min(255.0f,  55 + n * 0.4f);
            d[i+2] = (unsigned char)std::min(255.0f,  35 + n * 0.3f);
        }
    }
}

void genWoodTex(unsigned char* d, int W, int H) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float grain = sinf(y * 0.45f + sinf(x * 0.08f) * 4.0f) * 0.5f + 0.5f;
        float n = hashNoise(x, y) * 12.0f;
        int i = (y * W + x) * 3;
        d[i]   = (unsigned char)std::min(255.0f, 100 + grain * 80 + n);
        d[i+1] = (unsigned char)std::min(255.0f,  60 + grain * 50 + n * 0.7f);
        d[i+2] = (unsigned char)std::min(255.0f,  30 + grain * 30 + n * 0.4f);
    }
}

void genConcreteTex(unsigned char* d, int W, int H) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float n = hashNoise(x, y) * 20.0f;
        int i = (y * W + x) * 3;
        unsigned char v = (unsigned char)std::min(255.0f, 160 + n);
        d[i] = v; d[i+1] = (unsigned char)(v - 5); d[i+2] = (unsigned char)(v - 10);
    }
}

void genStoneTex(unsigned char* d, int W, int H) {
    /* Large sandstone blocks with mortar lines */
    const int BW=48, BH=24, MRT=2;
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
        int row=y/BH, ox=(row&1)*(BW/2);
        int bx=(x+ox)%BW, by=y%BH;
        bool mortar=(bx<MRT||by<MRT);
        int i=(y*W+x)*3;
        float n=hashNoise(x/3,y/3)*18.0f;
        if (mortar) { d[i]=175; d[i+1]=168; d[i+2]=155; }
        else {
            d[i]  =(unsigned char)std::min(255.0f,190+n);
            d[i+1]=(unsigned char)std::min(255.0f,170+n*0.9f);
            d[i+2]=(unsigned char)std::min(255.0f,130+n*0.8f);
        }
    }
}

void genCobbleTex(unsigned char* d, int W, int H) {
    /* Round cobblestones with dark grout */
    const int CS=20;
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
        int row=y/CS, col=x/CS, ox=(row&1)*(CS/2);
        int lx=(x+ox)%CS, ly=y%CS;
        float fx=lx/(float)CS-0.5f, fy=ly/(float)CS-0.5f;
        float dist=sqrtf(fx*fx+fy*fy);
        bool grout=(dist>0.38f);
        float n=hashNoise(col+(row&1)*100,row)*20.0f;
        int i=(y*W+x)*3;
        if (grout) { d[i]=60; d[i+1]=58; d[i+2]=55; }
        else {
            d[i]  =(unsigned char)std::min(255.0f,130+n);
            d[i+1]=(unsigned char)std::min(255.0f,120+n*0.95f);
            d[i+2]=(unsigned char)std::min(255.0f,105+n*0.85f);
        }
    }
}