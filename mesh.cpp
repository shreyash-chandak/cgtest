/* =====================================================================
 *  mesh.cpp — CPU-side mesh generation
 *  Each vertex layout: pos(3) | normal(3) | uv(2) = 8 floats
 * ===================================================================== */
#include "gl_common.h"
#include "mesh.h"
#include "constants.h"

#include <vector>
#include <cmath>

/* ── Low-level helpers ── */
static Mesh makeMesh(const std::vector<float>& verts) {
    Mesh m;
    m.count = (int)verts.size() / 8;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    int s = 8*(int)sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, s, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, s, (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, s, (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return m;
}

static void pv(std::vector<float>& v, glm::vec3 p, glm::vec3 n, glm::vec2 t) {
    v.push_back(p.x); v.push_back(p.y); v.push_back(p.z);
    v.push_back(n.x); v.push_back(n.y); v.push_back(n.z);
    v.push_back(t.x); v.push_back(t.y);
}

/* ── Box ── */
Mesh createBox() {
    std::vector<float> v;
    auto face = [&](glm::vec3 n, glm::vec3 u, glm::vec3 r) {
        glm::vec3 c=n*0.5f, a=c-u*0.5f-r*0.5f, b=c+u*0.5f-r*0.5f;
        glm::vec3 cc=c+u*0.5f+r*0.5f, d=c-u*0.5f+r*0.5f;
        pv(v,a,n,{0,0}); pv(v,b,n,{1,0}); pv(v,cc,n,{1,1});
        pv(v,a,n,{0,0}); pv(v,cc,n,{1,1}); pv(v,d,n,{0,1});
    };
    face({0,0,1},{0,1,0},{1,0,0}); face({0,0,-1},{0,1,0},{-1,0,0});
    face({0,1,0},{0,0,-1},{1,0,0}); face({0,-1,0},{0,0,1},{1,0,0});
    face({1,0,0},{0,1,0},{0,0,-1}); face({-1,0,0},{0,1,0},{0,0,1});
    return makeMesh(v);
}

/* ── Open cylinder shell ── */
Mesh createCylinder(int sl) {
    std::vector<float> v;
    for (int i=0;i<sl;i++) {
        float t1=2.0f*(float)M_PI*i/sl, t2=2.0f*(float)M_PI*(i+1)/sl;
        float c1=cosf(t1),s1=sinf(t1),c2=cosf(t2),s2=sinf(t2);
        float u1=(float)i/sl, u2=(float)(i+1)/sl;
        glm::vec3 n1(c1,0,s1),n2(c2,0,s2);
        glm::vec3 lo1(0.5f*c1,-0.5f,0.5f*s1),lo2(0.5f*c2,-0.5f,0.5f*s2);
        glm::vec3 hi1(0.5f*c1,0.5f,0.5f*s1),hi2(0.5f*c2,0.5f,0.5f*s2);
        pv(v,lo1,n1,{u1,0}); pv(v,hi1,n1,{u1,1}); pv(v,lo2,n2,{u2,0});
        pv(v,lo2,n2,{u2,0}); pv(v,hi1,n1,{u1,1}); pv(v,hi2,n2,{u2,1});
    }
    return makeMesh(v);
}

/* ── Capped cylinder ── */
Mesh createCylinderCapped(int sl) {
    std::vector<float> v;
    for (int i=0;i<sl;i++) {
        float t1=2.0f*(float)M_PI*i/sl, t2=2.0f*(float)M_PI*(i+1)/sl;
        float c1=cosf(t1),s1=sinf(t1),c2=cosf(t2),s2=sinf(t2);
        float u1=(float)i/sl, u2=(float)(i+1)/sl;
        glm::vec3 n1(c1,0,s1),n2(c2,0,s2);
        glm::vec3 lo1(0.5f*c1,-0.5f,0.5f*s1),lo2(0.5f*c2,-0.5f,0.5f*s2);
        glm::vec3 hi1(0.5f*c1,0.5f,0.5f*s1),hi2(0.5f*c2,0.5f,0.5f*s2);
        pv(v,lo1,n1,{u1,0}); pv(v,hi1,n1,{u1,1}); pv(v,lo2,n2,{u2,0});
        pv(v,lo2,n2,{u2,0}); pv(v,hi1,n1,{u1,1}); pv(v,hi2,n2,{u2,1});
        glm::vec3 tc(0,0.5f,0),tn(0,1,0);
        pv(v,tc,tn,{0.5f,0.5f}); pv(v,hi1,tn,{c1*.5f+.5f,s1*.5f+.5f}); pv(v,hi2,tn,{c2*.5f+.5f,s2*.5f+.5f});
        glm::vec3 bc(0,-0.5f,0),bn(0,-1,0);
        pv(v,bc,bn,{0.5f,0.5f}); pv(v,lo2,bn,{c2*.5f+.5f,s2*.5f+.5f}); pv(v,lo1,bn,{c1*.5f+.5f,s1*.5f+.5f});
    }
    return makeMesh(v);
}

/* ── Sphere ── */
Mesh createSphere(int sl, int st) {
    std::vector<float> v;
    for (int i=0;i<st;i++) {
        float p1=(float)M_PI*i/st-(float)M_PI/2, p2=(float)M_PI*(i+1)/st-(float)M_PI/2;
        for (int j=0;j<sl;j++) {
            float t1=2*(float)M_PI*j/sl, t2=2*(float)M_PI*(j+1)/sl;
            auto P=[](float p,float t){ return glm::vec3(cosf(p)*cosf(t),sinf(p),cosf(p)*sinf(t))*0.5f; };
            glm::vec3 a=P(p1,t1),b=P(p1,t2),c=P(p2,t1),d=P(p2,t2);
            glm::vec2 ua{(float)j/sl,(float)i/st},ub{(float)(j+1)/sl,(float)i/st};
            glm::vec2 uc{(float)j/sl,(float)(i+1)/st},ud{(float)(j+1)/sl,(float)(i+1)/st};
            pv(v,a,glm::normalize(a),ua); pv(v,c,glm::normalize(c),uc); pv(v,b,glm::normalize(b),ub);
            pv(v,b,glm::normalize(b),ub); pv(v,c,glm::normalize(c),uc); pv(v,d,glm::normalize(d),ud);
        }
    }
    return makeMesh(v);
}

/* ── Circular ground disk ──
   WINDING FIX: vertices emitted as c → p2 → p1 so the front face
   (CCW in OpenGL) points UP (+Y) and is not back-face culled.
   Radius uses COL_R_IN so the floor extends right up to the wall. */
Mesh createGround() {
    std::vector<float> v;
    glm::vec3 up(0,1,0);
    const float y=-0.01f;
    const float R = COL_R_IN;          /* extend to the wall — no gap */
    const int N=80;
    for (int i=0;i<N;i++) {
        float t1=2*(float)M_PI*i/N, t2=2*(float)M_PI*(i+1)/N;
        glm::vec3 c(0,y,0);
        glm::vec3 p1(R*cosf(t1),y,R*sinf(t1));
        glm::vec3 p2(R*cosf(t2),y,R*sinf(t2));
        pv(v,c,up,{0.5f,0.5f});
        pv(v,p2,up,{cosf(t2)*R/8.0f+0.5f, sinf(t2)*R/8.0f+0.5f});
        pv(v,p1,up,{cosf(t1)*R/8.0f+0.5f, sinf(t1)*R/8.0f+0.5f});
    }
    return makeMesh(v);
}

/* ── Colosseum: 3-tier hollow cylinder with inner+outer faces ── */
Mesh createColosseum() {
    std::vector<float> v;
    const int SEG = COL_SEG;
    const float Ht = COL_H / COL_TIERS;

    for (int tier=0; tier<(int)COL_TIERS; tier++) {
        float yBot = tier * Ht;
        float yTop = yBot + Ht;
        float taper = tier * 0.55f;
        float ro = COL_R_OUT - taper;
        float ri = COL_R_IN  - taper;

        for (int i=0;i<SEG;i++) {
            float t1=2*(float)M_PI*i/SEG, t2=2*(float)M_PI*(i+1)/SEG;
            float u1=(float)i/SEG*12.0f, u2=(float)(i+1)/SEG*12.0f;
            float v1=(float)tier/COL_TIERS, v2=(float)(tier+1)/COL_TIERS;

            /* outer face */
            glm::vec3 no1(cosf(t1),0,sinf(t1)), no2(cosf(t2),0,sinf(t2));
            glm::vec3 obl(ro*cosf(t1),yBot,ro*sinf(t1)), obr(ro*cosf(t2),yBot,ro*sinf(t2));
            glm::vec3 otl(ro*cosf(t1),yTop,ro*sinf(t1)), otr(ro*cosf(t2),yTop,ro*sinf(t2));
            pv(v,obl,no1,{u1,v1}); pv(v,otl,no1,{u1,v2}); pv(v,obr,no2,{u2,v1});
            pv(v,obr,no2,{u2,v1}); pv(v,otl,no1,{u1,v2}); pv(v,otr,no2,{u2,v2});

            /* inner face */
            glm::vec3 ni1(-cosf(t1),0,-sinf(t1)), ni2(-cosf(t2),0,-sinf(t2));
            glm::vec3 ibl(ri*cosf(t1),yBot,ri*sinf(t1)), ibr(ri*cosf(t2),yBot,ri*sinf(t2));
            glm::vec3 itl(ri*cosf(t1),yTop,ri*sinf(t1)), itr(ri*cosf(t2),yTop,ri*sinf(t2));
            pv(v,ibr,ni2,{u2,v1}); pv(v,itr,ni2,{u2,v2}); pv(v,ibl,ni1,{u1,v1});
            pv(v,ibl,ni1,{u1,v1}); pv(v,itr,ni2,{u2,v2}); pv(v,itl,ni1,{u1,v2});

            /* tier top walkway */
            glm::vec3 nt(0,1,0);
            pv(v,otl,nt,{u1,0}); pv(v,itl,nt,{u1,1}); pv(v,otr,nt,{u2,0});
            pv(v,otr,nt,{u2,0}); pv(v,itl,nt,{u1,1}); pv(v,itr,nt,{u2,1});
        }
    }
    return makeMesh(v);
}

/* ── Road rings ── */
static Mesh makeEllipseRoad(float A, float B) {
    std::vector<float> v;
    glm::vec3 up(0,1,0);
    const float y=0.02f;
    for (int i=0;i<TRK_SEG;i++) {
        float t1=2*(float)M_PI*i/TRK_SEG, t2=2*(float)M_PI*(i+1)/TRK_SEG;
        auto eNorm=[&](float t)->glm::vec3{
            float nx=B*cosf(t),nz=A*sinf(t),len=sqrtf(nx*nx+nz*nz);
            return {nx/len,0.0f,nz/len};
        };
        glm::vec3 c1(A*cosf(t1),y,B*sinf(t1)), c2(A*cosf(t2),y,B*sinf(t2));
        glm::vec3 n1=eNorm(t1), n2=eNorm(t2);
        float hw=ROAD_W*0.5f;
        glm::vec3 i1=c1-hw*n1, o1=c1+hw*n1, i2=c2-hw*n2, o2=c2+hw*n2;
        float u1=(float)i*6.0f/TRK_SEG, u2=(float)(i+1)*6.0f/TRK_SEG;
        pv(v,i1,up,{u1,0}); pv(v,o1,up,{u1,1}); pv(v,i2,up,{u2,0});
        pv(v,o1,up,{u1,1}); pv(v,o2,up,{u2,1}); pv(v,i2,up,{u2,0});
    }
    return makeMesh(v);
}

Mesh createRoad()      { return makeEllipseRoad(TRK_A_OUT, TRK_B_OUT); }
Mesh createRoadInner() { return makeEllipseRoad(TRK_A_IN,  TRK_B_IN);  }

/* ── Nearest track point helpers ── */
static glm::vec3 nearestEllipse(glm::vec3 p, float A, float B) {
    float best=1e18f; glm::vec3 ret(0);
    for (int i=0;i<360;i++) {
        float t=glm::radians((float)i);
        glm::vec3 q(A*cosf(t),0,B*sinf(t));
        float d=glm::length(glm::vec3(p.x,0,p.z)-q);
        if (d<best){best=d;ret=q;}
    }
    return ret;
}

glm::vec3 nearestTrack(glm::vec3 p)      { return nearestEllipse(p, TRK_A_OUT, TRK_B_OUT); }
glm::vec3 nearestTrackInner(glm::vec3 p) { return nearestEllipse(p, TRK_A_IN,  TRK_B_IN);  }