// Командный 3D-редактор: SDL2 + OpenGL ES 3 + Dear ImGui, PBR-материалы
#include <SDL.h>
#include <GLES3/gl3.h>
#include <jni.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "model.hpp"

static const char* PISTOL = R"PS(// Пистолет по частям (метры). Правь числа и жми «Запуск»
box frame size=0.028,0.03,0.18 pos=0,0,0 color=#2b2b2b
box grip size=0.03,0.09,0.05 pos=0,-0.058,-0.05 rot=15,0,0 color=#262626
box mag size=0.024,0.1,0.04 pos=0,-0.075,-0.052 rot=15,0,0 color=#3a3a3a
box slide size=0.03,0.035,0.19 pos=0,0.032,0.005 color=#1c1c1c
cylinder barrel r=0.006 h=0.1 pos=0,0.034,0.07 rot=90,0,0 color=#666666
box trigger size=0.006,0.022,0.01 pos=0,-0.025,0.005 color=#444444
box guard size=0.024,0.004,0.05 pos=0,-0.038,0.03 color=#2b2b2b
box sight size=0.006,0.008,0.008 pos=0,0.052,0.09 color=#dddddd
)PS";

static const char* AK = R"PS(// AK from reference: traced side silhouettes. Metres, Z forward, Y up, bore axis y=0
extrude receiver pts=0.1706,0.0513;0.1706,0.0324;0.1611,0.0308;0.1611,0.0197;0.1280,0.0182;0.1296,-0.0434;0.1532,-0.0466;0.1564,-0.0419;0.1706,-0.0387;0.1485,-0.0482;0.1327,-0.0482;0.1217,-0.0403;0.0806,-0.0419;0.0790,-0.0498;-0.1548,-0.0498;-0.1580,-0.0340;-0.1564,-0.0008;-0.1706,0.0008;-0.1564,0.0071;-0.1501,0.0276;-0.1422,0.0292;-0.1248,0.0498 w=0.038 color=#646941
extrude rearsight pts=0.0901,0.0687;0.0932,0.0703;0.0948,0.0671;0.1264,0.0656;0.1406,0.0687;0.1469,0.0656;0.1580,0.0671;0.1659,0.0577;0.1706,0.0577;0.1706,0.0529;0.0948,0.0529;0.0901,0.0624 w=0.020 color=#4a5038
extrude handguard_up pts=0.2796,0.0545;0.2812,0.0229;0.1627,0.0229;0.1627,0.0308;0.1754,0.0324;0.1738,0.0577;0.2654,0.0577 w=0.040 color=#7a3f28
extrude handguard_low pts=0.2954,-0.0055;0.2891,-0.0071;0.2891,-0.0245;0.2796,-0.0245;0.2781,-0.0292;0.2591,-0.0324;0.1848,-0.0340;0.1564,-0.0403;0.1532,-0.0450;0.1311,-0.0434;0.1280,-0.0371;0.1201,-0.0371;0.1232,-0.0324;0.1296,-0.0308;0.1296,-0.0071;0.1169,-0.0039;0.1296,-0.0024;0.1296,0.0150;0.1217,0.0166;0.1217,0.0197;0.1611,0.0182;0.1627,0.0213;0.2796,0.0213;0.2796,-0.0024;0.2828,-0.0055 w=0.044 color=#7a3f28
extrude stock pts=-0.1532,0.0087;-0.1532,-0.0561;-0.1817,-0.0640;-0.2986,-0.1193;-0.2986,-0.1288;-0.3112,-0.1351;-0.3176,-0.1303;-0.3270,-0.1303;-0.3507,-0.1414;-0.3618,-0.1382;-0.3681,-0.0150;-0.3634,-0.0055;-0.2623,0.0024;-0.2259,0.0024;-0.2149,-0.0055;-0.2006,-0.0055 w=0.036 color=#7a3f28
extrude grip pts=-0.0806,-0.0498;-0.0806,-0.0608;-0.0743,-0.0640;-0.0743,-0.0861;-0.0822,-0.0861;-0.0869,-0.0908;-0.1027,-0.1604;-0.1090,-0.1730;-0.1074,-0.1935;-0.1185,-0.1999;-0.1517,-0.1904;-0.1580,-0.1777;-0.1343,-0.0972;-0.1343,-0.0719;-0.1375,-0.0640;-0.1327,-0.0608;-0.1311,-0.0640;-0.0901,-0.0624;-0.0885,-0.0498 w=0.032 color=#6b3524
extrude mag pts=-0.0174,-0.0466;0.0774,-0.0466;0.0822,-0.0845;0.0964,-0.1272;0.1201,-0.1714;0.1485,-0.2046;0.1485,-0.2157;0.1106,-0.2662;0.1011,-0.2662;0.0553,-0.2078;0.0237,-0.1414;0.0095,-0.0845;-0.0158,-0.0829 w=0.040 color=#4c5240
extrude guard pts=-0.0269,-0.0498;-0.0442,-0.0498;-0.0537,-0.0608;-0.0537,-0.0766;-0.0427,-0.0877;-0.0442,-0.0893;-0.0506,-0.0877;-0.0569,-0.0814;-0.0600,-0.0735;-0.0600,-0.0577;-0.0679,-0.0545;-0.0711,-0.0845;-0.0695,-0.0893;-0.0600,-0.0972;-0.0205,-0.0956;-0.0158,-0.0908;-0.0158,-0.0861;-0.0111,-0.0861;-0.0142,-0.0956;-0.0205,-0.0987;-0.0616,-0.1003;-0.0679,-0.0972;-0.0727,-0.0908;-0.0774,-0.0908;-0.0853,-0.1003;-0.0916,-0.1256;-0.0916,-0.1161;-0.0853,-0.0987;-0.0853,-0.0908;-0.0822,-0.0877;-0.0727,-0.0861;-0.0727,-0.0640;-0.0790,-0.0608;-0.0790,-0.0498 w=0.006 color=#4a5038
extrude frontsight pts=0.4724,0.0750;0.4771,0.0719;0.4771,-0.0245;0.4455,-0.0245;0.4455,-0.0182;0.4487,-0.0166;0.4487,-0.0134;0.4455,-0.0118;0.4455,0.0118;0.4503,0.0213;0.4582,0.0513;0.4582,0.0656;0.4629,0.0735 w=0.016 color=#4a5038
extrude gasblock pts=0.3507,0.0482;0.3634,0.0482;0.3713,0.0245;0.3792,0.0166;0.3902,0.0118;0.3902,-0.0245;0.3855,-0.0308;0.3744,-0.0308;0.3713,-0.0245;0.3507,-0.0245;0.3507,-0.0182;0.3571,-0.0166;0.3555,-0.0118;0.3507,-0.0118;0.3507,0.0118;0.3571,0.0134;0.3555,0.0197;0.3507,0.0229 w=0.026 color=#4a5038
lathe barrel prof=0,-0.1730;0.0118,-0.1730;0.0118,0.0387;0.0118,0.0545;0.0118,0.1098;0.0118,0.1414;0.0142,0.1461;0.0142,0.1572;0.0111,0.1619;0.0111,0.1730;0,0.1730 seg=20 rot=90,0,0 pos=0,0.0000,0.3326 color=#4a5038
lathe gastube prof=0,-0.0980;0.0126,-0.0980;0.0126,0.0980;0,0.0980 seg=16 rot=90,0,0 pos=0,0.0355,0.2575 color=#5a6140
lathe rod prof=0,-0.0711;0.0032,-0.0711;0.0032,0.0711;0,0.0711 seg=8 rot=90,0,0 pos=0,-0.0213,0.3713 color=#3c4030
)PS";
static const char* DEMO = R"PS(// Материалы и новые формы. Метры, Y вверх.
materials auto
box floor size=0.7,0.01,0.5 pos=0,-0.005,0 material=бетон
box plank size=0.22,0.025,0.09 pos=-0.17,0.0125,0.06 material=walnut bevel=0.005 bevel_segments=3
cylinder rod r=0.015 h=0.15 pos=0,0.075,0.06 seg=40 smooth=1 material=steel
torus ring r=0.04 tube=0.012 seg=40 rings=16 pos=0.17,0.052,0.06 rot=90,0,0 smooth=1 material=copper
sphere ball r=0.035 seg=32 smooth=1 pos=-0.14,0.04,-0.1 material=glass
capsule pill r=0.02 h=0.05 seg=24 rings=8 pos=0,0.045,-0.1 smooth=1 material=резина
ellipsoid egg size=0.07,0.1,0.07 seg=32 pos=0.14,0.05,-0.1 smooth=1 material=керамика
box wall size=0.24,0.12,0.02 pos=-0.2,0.06,-0.2 material=кирпич
box stock size=0.12,0.03,0.04 pos=0.17,0.015,-0.2
)PS";

// ---------- матрицы (column-major) ----------
typedef float M4[16];
static void mul(const float* a, const float* b, float* r) {
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a[k * 4 + row] * b[c * 4 + k];
            r[c * 4 + row] = s;
        }
}
static void persp(float fovy, float asp, float n, float f, float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    float t = 1.f / std::tan(fovy * 0.5f);
    m[0] = t / asp; m[5] = t; m[10] = (f + n) / (n - f); m[11] = -1; m[14] = 2 * f * n / (n - f);
}
static void lookAt(V3 e, V3 c, V3 up, float* m) {
    V3 f = norm3(c - e), s = norm3(cross3(f, up)), u = cross3(s, f);
    m[0] = s.x; m[4] = s.y; m[8] = s.z; m[12] = -dot3(s, e);
    m[1] = u.x; m[5] = u.y; m[9] = u.z; m[13] = -dot3(u, e);
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z; m[14] = dot3(f, e);
    m[3] = m[7] = m[11] = 0; m[15] = 1;
}

// ---------- шейдеры ----------
// Простой: сетка и каркас
static const char* VS_LINE =
    "#version 300 es\n"
    "layout(location=0) in vec3 p; layout(location=1) in vec3 n; layout(location=2) in vec3 c;\n"
    "uniform mat4 mvp; out vec3 vc;\n"
    "void main(){ vc=c; gl_Position=mvp*vec4(p,1.0); }\n";
static const char* FS_LINE =
    "#version 300 es\nprecision mediump float; in vec3 vc; out vec4 o;\n"
    "void main(){ o=vec4(vc,1.0); }\n";

// PBR: albedo/normal/ORM, GGX, два направленных источника, окружение-градиент
static const char* VS_PBR =
    "#version 300 es\n"
    "layout(location=0) in vec3 p; layout(location=1) in vec3 n; layout(location=2) in vec3 c; layout(location=3) in vec2 t;\n"
    "uniform mat4 mvp; out vec3 vP; out vec3 vN; out vec3 vC; out vec2 vUV;\n"
    "void main(){ vP=p; vN=n; vC=c; vUV=t; gl_Position=mvp*vec4(p,1.0); }\n";
static const char* FS_PBR =
    "#version 300 es\nprecision highp float;\n"
    "in vec3 vP; in vec3 vN; in vec3 vC; in vec2 vUV;\n"
    "uniform vec3 uCam; uniform float uUseTex; uniform vec3 uTint; uniform float uMetal; uniform float uRough;\n"
    "uniform float uAlpha; uniform float uNS; uniform sampler2D tA; uniform sampler2D tN; uniform sampler2D tO;\n"
    "out vec4 o;\n"
    "const float PI=3.14159265;\n"
    "vec3 perturb(vec3 N, vec3 p, vec2 uv, vec3 tn){\n"
    " vec3 dp1=dFdx(p), dp2=dFdy(p); vec2 duv1=dFdx(uv), duv2=dFdy(uv);\n"
    " vec3 dp2perp=cross(dp2,N), dp1perp=cross(N,dp1);\n"
    " vec3 T=dp2perp*duv1.x+dp1perp*duv2.x; vec3 B=dp2perp*duv1.y+dp1perp*duv2.y;\n"
    " float im=inversesqrt(max(max(dot(T,T),dot(B,B)),1e-20)); return normalize(mat3(T*im,B*im,N)*tn); }\n"
    "float D_GGX(float NH,float a){ float a2=a*a; float d=NH*NH*(a2-1.0)+1.0; return a2/(PI*d*d+1e-6); }\n"
    "float G_S(float NV,float NL,float r){ float k=(r+1.0)*(r+1.0)/8.0; return (NV/(NV*(1.0-k)+k))*(NL/(NL*(1.0-k)+k)); }\n"
    "vec3 F_S(float c,vec3 F0){ return F0+(vec3(1.0)-F0)*pow(1.0-c,5.0); }\n"
    "vec3 env(vec3 d,float rough){ float t=clamp(d.y*0.5+0.5,0.0,1.0);\n"
    " vec3 sky=mix(vec3(0.62,0.72,0.88),vec3(0.28,0.45,0.78),t); vec3 gnd=vec3(0.2,0.19,0.18);\n"
    " vec3 e=mix(gnd,sky,smoothstep(0.38,0.62,t)); return mix(e,vec3(0.45,0.5,0.55),rough*rough*0.6); }\n"
    "vec3 light(vec3 N,vec3 V,vec3 L,vec3 Li,vec3 alb,float metal,float rough,vec3 F0){\n"
    " float NL=max(dot(N,L),0.0); if(NL<=0.0) return vec3(0.0); vec3 H=normalize(V+L);\n"
    " float NV=max(dot(N,V),1e-3), NH=max(dot(N,H),0.0), VH=max(dot(V,H),0.0); float a=rough*rough;\n"
    " vec3 F=F_S(VH,F0); vec3 spec=D_GGX(NH,a)*G_S(NV,NL,rough)*F/(4.0*NV*NL+1e-4);\n"
    " vec3 dif=(vec3(1.0)-F)*(1.0-metal)*alb/PI; return (dif+spec)*Li*NL; }\n"
    "void main(){\n"
    " vec3 N=normalize(vN); vec3 V=normalize(uCam-vP);\n"
    " vec3 alb; float metal=uMetal, rough=uRough, ao=1.0;\n"
    " if(uUseTex>0.5){ alb=texture(tA,vUV).rgb*uTint;\n"
    "  vec3 tn=texture(tN,vUV).xyz*2.0-1.0; tn.xy*=uNS; N=perturb(N,vP,vUV,normalize(tn));\n"
    "  vec3 orm=texture(tO,vUV).rgb; ao=orm.r; rough=clamp(uRough+(orm.g-0.5)*0.6,0.04,1.0); metal=clamp(uMetal*orm.b,0.0,1.0);\n"
    " } else { alb=pow(vC,vec3(2.2)); }\n"
    " vec3 F0=mix(vec3(0.04),alb,metal);\n"
    " vec3 col=light(N,V,normalize(vec3(0.5,0.8,0.6)),vec3(3.2,3.0,2.8),alb,metal,rough,F0)\n"
    "        +light(N,V,normalize(vec3(-0.6,0.3,-0.5)),vec3(0.9,1.0,1.2),alb,metal,rough,F0);\n"
    " col+=env(N,1.0)*alb*(1.0-metal)*ao*0.9;\n"
    " vec3 R=reflect(-V,N); vec3 Fe=F0+(max(vec3(1.0-rough),F0)-F0)*pow(1.0-max(dot(N,V),0.0),5.0);\n"
    " col+=env(R,rough)*Fe*ao*1.1;\n"
    " col=col/(col+vec3(1.0)); col=pow(col,vec3(1.0/2.2));\n"
    " o=vec4(col,uAlpha); }\n";

static GLuint mkShader(GLenum t, const char* s) {
    GLuint h = glCreateShader(t);
    glShaderSource(h, 1, &s, nullptr);
    glCompileShader(h);
    GLint ok = 0; glGetShaderiv(h, GL_COMPILE_STATUS, &ok);
    if (!ok) { char b[1024]; glGetShaderInfoLog(h, 1024, nullptr, b); SDL_Log("shader error: %s", b); }
    return h;
}
static GLuint mkProgram(const char* vs, const char* fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, mkShader(GL_VERTEX_SHADER, vs));
    glAttachShader(p, mkShader(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    return p;
}
static void setupLineVAO(GLuint vao, GLuint vbo) {
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 0; i < 3; i++) { glEnableVertexAttribArray(i); glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 36, (void*)(uintptr_t)(i * 12)); }
    glBindVertexArray(0);
}
static void setupModelVAO(GLuint vao, GLuint vbo) {
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 0; i < 3; i++) { glEnableVertexAttribArray(i); glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 44, (void*)(uintptr_t)(i * 12)); }
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 44, (void*)(uintptr_t)36);
    glBindVertexArray(0);
}

// ---------- текстуры материалов ----------
struct GpuMat { GLuint t[3] = {0, 0, 0}; };
static std::map<std::string, GpuMat> gGpu;
static void upload(GLuint tex, const std::vector<uint8_t>& px, int n, GLenum internalFmt) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
static GpuMat& gpuMat(const mat::Def& d) {
    auto it = gGpu.find(d.name);
    if (it != gGpu.end()) return it->second;
    mat::TexSet ts = mat::gen(d, 256);
    GpuMat g; glGenTextures(3, g.t);
    upload(g.t[0], ts.a, ts.w, GL_SRGB8_ALPHA8);   // albedo: sRGB
    upload(g.t[1], ts.n, ts.w, GL_RGBA8);          // normal: линейная
    upload(g.t[2], ts.o, ts.w, GL_RGBA8);          // ORM: линейная
    return gGpu[d.name] = g;
}

struct Draw { int first, count; bool tex; GLuint t[3]; float base[3], tint[3], metal, rough, alpha, ns; };

// ---------- сохранение в «Загрузки» через Java ----------
static std::string saveDownload(const std::string& name, const std::string& data) {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject act = (jobject)SDL_AndroidGetActivity();
    if (!env || !act) return "err:jni";
    jclass cls = env->FindClass("com/modeler/app/Saver");
    if (!cls) { env->ExceptionClear(); return "err:class"; }
    jmethodID m = env->GetStaticMethodID(cls, "save", "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (!m) { env->ExceptionClear(); return "err:method"; }
    jstring jn = env->NewStringUTF(name.c_str());
    jstring jd = env->NewStringUTF(data.c_str());
    jstring r = (jstring)env->CallStaticObjectMethod(cls, m, act, jn, jd);
    std::string res = "err:null";
    if (r) { const char* c = env->GetStringUTFChars(r, nullptr); res = c; env->ReleaseStringUTFChars(r, c); }
    env->DeleteLocalRef(jn); env->DeleteLocalRef(jd);
    return res;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* win = SDL_CreateWindow("3D", 0, 0, 0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!win) { SDL_Log("window: %s", SDL_GetError()); return 1; }
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    if (!gl) { SDL_Log("gl: %s", SDL_GetError()); return 1; }
    SDL_GL_SetSwapInterval(1);

    // --- ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    float ddpi = 160.f;
    SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr);
    float scale = std::min(4.f, std::max(1.f, ddpi / 160.f));
    size_t fsz = 0;
    void* fdata = SDL_LoadFile("DroidSans.ttf", &fsz);
    const char* sysFonts[] = {"/system/fonts/Roboto-Regular.ttf", "/system/fonts/NotoSans-Regular.ttf", "/system/fonts/DroidSans.ttf"};
    for (int i = 0; !fdata && i < 3; i++) fdata = SDL_LoadFile(sysFonts[i], &fsz);
    if (fdata) {
        ImFontConfig cfg; cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(fdata, (int)fsz, 14.f * scale, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    } else {
        ImFontConfig cfg; cfg.SizePixels = 14.f * scale;
        io.Fonts->AddFontDefault(&cfg);
    }
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // --- шейдеры, буферы ---
    GLuint progLine = mkProgram(VS_LINE, FS_LINE), progPbr = mkProgram(VS_PBR, FS_PBR);
    GLint uLineMvp = glGetUniformLocation(progLine, "mvp");
    GLint uMvp = glGetUniformLocation(progPbr, "mvp"), uCam = glGetUniformLocation(progPbr, "uCam"), uUseTex = glGetUniformLocation(progPbr, "uUseTex"),
          uTint = glGetUniformLocation(progPbr, "uTint"), uMetal = glGetUniformLocation(progPbr, "uMetal"), uRough = glGetUniformLocation(progPbr, "uRough"),
          uAlpha = glGetUniformLocation(progPbr, "uAlpha"), uNS = glGetUniformLocation(progPbr, "uNS"),
          sA = glGetUniformLocation(progPbr, "tA"), sN = glGetUniformLocation(progPbr, "tN"), sO = glGetUniformLocation(progPbr, "tO");

    GLuint vaoModel, vaoGrid, vaoWire, vboModel, vboGrid, vboWire;
    glGenVertexArrays(1, &vaoModel); glGenVertexArrays(1, &vaoGrid); glGenVertexArrays(1, &vaoWire);
    glGenBuffers(1, &vboModel); glGenBuffers(1, &vboGrid); glGenBuffers(1, &vboWire);
    setupModelVAO(vaoModel, vboModel); setupLineVAO(vaoGrid, vboGrid); setupLineVAO(vaoWire, vboWire);
    std::vector<float> grid;
    auto gv = [&](float x, float z, float c) { float d[9] = {x, 0, z, c, c, c, c, c, c}; grid.insert(grid.end(), d, d + 9); };
    for (int i = -10; i <= 10; i++) { float c = (i == 0) ? 0.55f : 0.28f, p = i * 0.1f; gv(p, -1, c); gv(p, 1, c); gv(-1, p, c); gv(1, p, c); }
    glBindBuffer(GL_ARRAY_BUFFER, vboGrid);
    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(float), grid.data(), GL_STATIC_DRAW);
    int gridVerts = (int)grid.size() / 9, wireVerts = 0;

    // --- сцена и текст скрипта ---
    Scene scene;
    static char buf[16384];
    std::string lastPath = std::string(SDL_AndroidGetInternalStoragePath() ? SDL_AndroidGetInternalStoragePath() : ".") + "/last.txt";
    {
        FILE* f = fopen(lastPath.c_str(), "rb");
        size_t n = 0;
        if (f) { n = fread(buf, 1, sizeof(buf) - 1, f); fclose(f); }
        buf[n] = 0;
        if (n == 0) std::snprintf(buf, sizeof buf, "%s", PISTOL);
    }
    std::vector<std::string> msgs;
    std::vector<Draw> draws;
    std::vector<float> lastData;
    bool scrollLog = false, wire = false;
    V3 tgt = {0, 0, 0};

    auto rebuildWire = [&]() {
        std::vector<float> w;
        size_t nv = lastData.size() / 11;
        if (nv / 3 <= 60000) {
            for (size_t t = 0; t + 2 < nv; t += 3)
                for (int k = 0; k < 3; k++)
                    for (int e = 0; e < 2; e++) {
                        const float* v = &lastData[(t + (k + e) % 3) * 11];
                        float d[9] = {v[0], v[1], v[2], 0.85f, 0.9f, 0.95f, 0.85f, 0.9f, 0.95f};
                        w.insert(w.end(), d, d + 9);
                    }
        }
        glBindBuffer(GL_ARRAY_BUFFER, vboWire);
        glBufferData(GL_ARRAY_BUFFER, w.size() * sizeof(float), w.data(), GL_DYNAMIC_DRAW);
        wireVerts = (int)w.size() / 9;
    };
    auto rebuild = [&]() {
        scene.run(buf);
        msgs = scene.log;
        std::vector<Range> rg; scene.build(lastData, rg);
        glBindBuffer(GL_ARRAY_BUFFER, vboModel);
        glBufferData(GL_ARRAY_BUFFER, lastData.size() * sizeof(float), lastData.data(), GL_DYNAMIC_DRAW);
        draws.clear();
        for (auto& r : rg) {
            const Part& p = scene.parts[r.part]; Draw d{}; d.first = r.first; d.count = r.count;
            if (!p.matName.empty()) {
                const mat::Def& m = scene.mats.at(p.matName); GpuMat& g = gpuMat(m);
                d.tex = true; for (int i = 0; i < 3; i++) d.t[i] = g.t[i];
                for (int i = 0; i < 3; i++) d.tint[i] = std::pow(m.tint[i], 2.2f);
                d.metal = m.metallic; d.rough = m.roughness; d.alpha = m.alpha; d.ns = m.normalStr;
            } else {
                d.tex = false; d.base[0] = p.col.x; d.base[1] = p.col.y; d.base[2] = p.col.z;
                d.metal = 0.f; d.rough = 0.6f; d.alpha = 1.f; d.ns = 1.f;
            }
            draws.push_back(d);
        }
        // камера центрируется на модели
        if (!lastData.empty()) {
            float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
            for (size_t i = 0; i + 10 < lastData.size(); i += 11)
                for (int k = 0; k < 3; k++) { lo[k] = std::min(lo[k], lastData[i + k]); hi[k] = std::max(hi[k], lastData[i + k]); }
            tgt = {(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2};
        } else tgt = {0, 0, 0};
        if (wire) rebuildWire();
        FILE* f = fopen(lastPath.c_str(), "wb");
        if (f) { fwrite(buf, 1, strlen(buf), f); fclose(f); }
        scrollLog = true;
    };
    rebuild();

    float th = 0.7f, ph = 1.1f, dist = 0.5f;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_MULTIGESTURE && e.mgesture.numFingers == 2) {
                dist *= (1.f - e.mgesture.dDist * 5.f);
                dist = std::min(30.f, std::max(0.05f, dist));
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        int W, H; SDL_GL_GetDrawableSize(win, &W, &H);
        bool kb = SDL_IsTextInputActive();

        int fingers = 0;
        if (SDL_GetNumTouchDevices() > 0) fingers = SDL_GetNumTouchFingers(SDL_GetTouchDevice(0));
        if (!io.WantCaptureMouse && ImGui::IsMouseDown(0) && fingers < 2) {
            th -= io.MouseDelta.x * 0.008f;
            ph = std::min(3.0f, std::max(0.1f, ph - io.MouseDelta.y * 0.008f));
        }

        float panelY = kb ? 28.f * scale : H * 0.52f;
        float panelH = kb ? H * 0.46f : H * 0.48f;
        ImGui::SetNextWindowPos(ImVec2(0, panelY));
        ImGui::SetNextWindowSize(ImVec2((float)W, panelH));
        ImGui::Begin("panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
        ImGui::InputTextMultiline("##src", buf, sizeof(buf), ImVec2(-1, panelH * 0.34f));
        float bh = 40.f * scale;
        auto btn = [&](const char* label) { bool r = ImGui::Button(label, ImVec2(0, bh)); ImGui::SameLine(); return r; };
        if (btn("Запуск")) rebuild();
        if (btn("Пример")) {
            static int ex = 0; int k = ex++ % 3;
            std::snprintf(buf, sizeof buf, "%s", k == 0 ? PISTOL : (k == 1 ? AK : DEMO));
            rebuild(); dist = k == 0 ? 0.45f : (k == 1 ? 1.3f : 1.1f); th = k == 1 ? 1.2f : 0.7f;
        }
        if (btn("Очистить")) { buf[0] = 0; rebuild(); }
        ImGui::NewLine();
        if (btn("OBJ")) {
            if (scene.parts.empty()) msgs.push_back("Сцена пуста");
            else {
                std::string r = saveDownload("model.obj", scene.exportObj("model.mtl"));
                std::string r2 = saveDownload("model.mtl", scene.exportMtl());
                msgs.push_back(r == "ok" && r2 == "ok" ? "Сохранено в Загрузки: model.obj и model.mtl" : "Ошибка сохранения: " + r + " " + r2);
            }
            scrollLog = true;
        }
        if (btn("Скрипт")) {
            std::string r = saveDownload("script.txt", buf);
            msgs.push_back(r == "ok" ? "Сохранено в Загрузки: script.txt" : "Ошибка сохранения: " + r);
            scrollLog = true;
        }
        if (btn(wire ? "Каркас: вкл" : "Каркас")) {
            wire = !wire;
            if (wire) { rebuildWire(); if (wireVerts == 0 && !lastData.empty()) msgs.push_back("Каркас отключён: больше 60000 полигонов"); }
        }
        ImGui::NewLine();
        ImGui::Text("%d дет. | %d вершин | %d рёбер | %d полиг.", (int)scene.parts.size(), scene.verts(), scene.edges(), scene.tris());
        ImGui::BeginChild("log", ImVec2(0, 0), true);
        for (auto& m : msgs) ImGui::TextWrapped("%s", m.c_str());
        if (scrollLog) { ImGui::SetScrollHereY(1.0f); scrollLog = false; }
        ImGui::EndChild();
        ImGui::End();

        if (io.WantTextInput) { if (!SDL_IsTextInputActive()) SDL_StartTextInput(); }
        else if (SDL_IsTextInputActive()) SDL_StopTextInput();

        ImGui::Render();

        // --- 3D в верхней части экрана ---
        glViewport(0, (int)(H * 0.48f), W, (int)(H * 0.52f));
        glClearColor(0.14f, 0.16f, 0.19f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        M4 P, V, MVP;
        persp(0.87f, (float)W / (H * 0.52f), 0.005f, 100.f, P);
        V3 eye = {tgt.x + dist * std::sin(ph) * std::sin(th), tgt.y + dist * std::cos(ph), tgt.z + dist * std::sin(ph) * std::cos(th)};
        lookAt(eye, tgt, {0, 1, 0}, V);
        mul(P, V, MVP);

        glUseProgram(progLine);
        glUniformMatrix4fv(uLineMvp, 1, GL_FALSE, MVP);
        glBindVertexArray(vaoGrid);
        glDrawArrays(GL_LINES, 0, gridVerts);
        if (wire) {
            glBindVertexArray(vaoWire);
            glDrawArrays(GL_LINES, 0, wireVerts);
        } else if (!draws.empty()) {
            glUseProgram(progPbr);
            glUniformMatrix4fv(uMvp, 1, GL_FALSE, MVP);
            glUniform3f(uCam, eye.x, eye.y, eye.z);
            glUniform1i(sA, 0); glUniform1i(sN, 1); glUniform1i(sO, 2);
            glBindVertexArray(vaoModel);
            for (int pass = 0; pass < 2; pass++) {
                if (pass == 1) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); }
                for (auto& d : draws) {
                    if ((d.alpha < 0.99f) != (pass == 1)) continue;
                    glUniform1f(uUseTex, d.tex ? 1.f : 0.f);
                    glUniform3f(uTint, d.tint[0], d.tint[1], d.tint[2]);
                    glUniform1f(uMetal, d.metal); glUniform1f(uRough, d.rough); glUniform1f(uAlpha, d.alpha); glUniform1f(uNS, d.ns);
                    if (d.tex) for (int k = 0; k < 3; k++) { glActiveTexture(GL_TEXTURE0 + k); glBindTexture(GL_TEXTURE_2D, d.t[k]); }
                    glDrawArrays(GL_TRIANGLES, d.first, d.count);
                }
            }
            glDepthMask(GL_TRUE); glDisable(GL_BLEND);
            glActiveTexture(GL_TEXTURE0);
        }
        glBindVertexArray(0);

        glViewport(0, 0, W, H);
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
