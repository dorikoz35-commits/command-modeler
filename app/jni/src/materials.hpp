// Материалы: база, синонимы (EN/RU), автоопределение по имени детали, процедурные PBR-карты.
// Без зависимостей от SDL/GL: проверяется обычными тестами.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <algorithm>

namespace mat {

enum Pat { P_BRUSHED, P_PAINT, P_RUST, P_WOOD, P_RUBBER, P_PLASTIC, P_CONCRETE, P_STONE, P_BRICK, P_GLASS,
           P_CERAMIC, P_FABRIC, P_LEATHER, P_CARBON, P_SAND, P_SOIL, P_GRASS, P_SNOW, P_ICE, P_WATER };

struct Def {
    std::string name;
    float base[3];    // sRGB, запекается в процедурный albedo
    float dark[3];    // второй цвет узора (тёмные кольца дерева, ржавчина, раствор кирпича...)
    float metallic, roughness, alpha;
    float tile;       // размер одного повторения текстуры в метрах
    float normalStr;  // интенсивность normal map
    int pat;
    float tint[3];    // пользовательский оттенок (sRGB), умножается на albedo
    bool online;      // запрошен source=online (скачивание появится позже)
};

inline std::vector<Def> builtin() {
    auto D = [](const char* n, float r, float g, float b, float dr, float dg, float db, float m, float ro, float a, float tile, float ns, int p) {
        Def d; d.name = n; d.base[0] = r; d.base[1] = g; d.base[2] = b; d.dark[0] = dr; d.dark[1] = dg; d.dark[2] = db;
        d.metallic = m; d.roughness = ro; d.alpha = a; d.tile = tile; d.normalStr = ns; d.pat = p;
        d.tint[0] = d.tint[1] = d.tint[2] = 1.f; d.online = false; return d; };
    return {
        D("steel", .62f, .63f, .65f, 0, 0, 0, 1.f, .35f, 1.f, .25f, 1.f, P_BRUSHED),
        D("iron", .45f, .45f, .47f, 0, 0, 0, 1.f, .5f, 1.f, .3f, 1.f, P_BRUSHED),
        D("aluminum", .82f, .83f, .84f, 0, 0, 0, 1.f, .3f, 1.f, .25f, 1.f, P_BRUSHED),
        D("copper", .91f, .52f, .38f, 0, 0, 0, 1.f, .25f, 1.f, .25f, 1.f, P_BRUSHED),
        D("gold", 1.f, .77f, .33f, 0, 0, 0, 1.f, .2f, 1.f, .25f, .8f, P_BRUSHED),
        D("painted_metal", .32f, .38f, .27f, 0, 0, 0, .55f, .42f, 1.f, .35f, 1.f, P_PAINT),
        D("rusted_metal", .55f, .55f, .56f, .45f, .2f, .08f, .6f, .7f, 1.f, .8f, 1.5f, P_RUST),
        D("wood", .52f, .35f, .2f, .33f, .2f, .1f, 0, .55f, 1.f, .18f, 1.f, P_WOOD),
        D("walnut", .36f, .22f, .13f, .17f, .09f, .05f, 0, .5f, 1.f, .18f, 1.f, P_WOOD),
        D("oak", .66f, .48f, .28f, .46f, .3f, .16f, 0, .55f, 1.f, .18f, 1.f, P_WOOD),
        D("rubber", .07f, .07f, .075f, 0, 0, 0, 0, .85f, 1.f, .15f, 1.f, P_RUBBER),
        D("plastic", .35f, .36f, .38f, 0, 0, 0, 0, .42f, 1.f, .3f, .6f, P_PLASTIC),
        D("concrete", .55f, .55f, .53f, 0, 0, 0, 0, .9f, 1.f, 1.f, 1.2f, P_CONCRETE),
        D("stone", .5f, .48f, .45f, .22f, .2f, .18f, 0, .8f, 1.f, 1.f, 1.3f, P_STONE),
        D("brick", .55f, .25f, .18f, .62f, .6f, .56f, 0, .9f, 1.f, .5f, 1.4f, P_BRICK),
        D("glass", .85f, .93f, .95f, 0, 0, 0, 0, .05f, .3f, 1.f, 0.f, P_GLASS),
        D("ceramic", .92f, .92f, .9f, 0, 0, 0, 0, .15f, 1.f, 1.f, .4f, P_CERAMIC),
        D("fabric", .35f, .4f, .5f, 0, 0, 0, 0, .92f, 1.f, .2f, 1.4f, P_FABRIC),
        D("leather", .36f, .2f, .12f, 0, 0, 0, 0, .55f, 1.f, .3f, 1.2f, P_LEATHER),
        D("carbon_fiber", .2f, .2f, .22f, 0, 0, 0, 0, .25f, 1.f, .08f, .8f, P_CARBON),
        D("sand", .76f, .65f, .45f, 0, 0, 0, 0, 1.f, 1.f, .6f, .8f, P_SAND),
        D("soil", .25f, .17f, .1f, 0, 0, 0, 0, 1.f, 1.f, .8f, 1.f, P_SOIL),
        D("grass", .3f, .55f, .15f, .1f, .25f, .05f, 0, .9f, 1.f, .6f, 1.f, P_GRASS),
        D("snow", .93f, .95f, .98f, 0, 0, 0, 0, .75f, 1.f, .6f, .6f, P_SNOW),
        D("ice", .7f, .85f, .95f, .45f, .6f, .75f, 0, .12f, .55f, .8f, .8f, P_ICE),
        D("water", .08f, .3f, .45f, 0, 0, 0, 0, .03f, .6f, 1.5f, 1.5f, P_WATER),
    };
}

// ---------- имена ----------
inline std::string lower(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) r += (char)std::tolower(c);
        else if (c == 0xD0 && i + 1 < s.size()) {
            unsigned char d = (unsigned char)s[i + 1];
            if (d >= 0x90 && d <= 0x9F) { r += (char)0xD0; r += (char)(d + 0x20); }
            else if (d >= 0xA0 && d <= 0xAF) { r += (char)0xD1; r += (char)(d - 0x20); }
            else if (d == 0x81) { r += (char)0xD1; r += (char)0x91; }
            else { r += (char)c; r += (char)d; }
            i++;
        } else r += (char)c;
    }
    return r;
}

// Название или синоним (рус/англ) -> каноническое имя материала, "" если не найдено
inline std::string canonical(const std::string& in) {
    static const std::vector<std::pair<std::string, std::string>> syn = {
        {"steel", "steel"}, {"сталь", "steel"}, {"стальной", "steel"}, {"нержавейка", "steel"}, {"металл", "steel"}, {"metal", "steel"},
        {"iron", "iron"}, {"железо", "iron"}, {"чугун", "iron"},
        {"aluminum", "aluminum"}, {"aluminium", "aluminum"}, {"алюминий", "aluminum"}, {"дюраль", "aluminum"},
        {"copper", "copper"}, {"медь", "copper"}, {"gold", "gold"}, {"золото", "gold"},
        {"wood", "wood"}, {"wooden", "wood"}, {"дерево", "wood"}, {"древесина", "wood"}, {"деревянный", "wood"},
        {"walnut", "walnut"}, {"орех", "walnut"}, {"грецкий_орех", "walnut"}, {"oak", "oak"}, {"дуб", "oak"},
        {"rubber", "rubber"}, {"резина", "rubber"}, {"резиновый", "rubber"},
        {"plastic", "plastic"}, {"пластик", "plastic"}, {"пластмасса", "plastic"}, {"polymer", "plastic"},
        {"concrete", "concrete"}, {"бетон", "concrete"}, {"stone", "stone"}, {"камень", "stone"}, {"rock", "stone"}, {"гранит", "stone"},
        {"brick", "brick"}, {"кирпич", "brick"}, {"glass", "glass"}, {"стекло", "glass"},
        {"ceramic", "ceramic"}, {"керамика", "ceramic"}, {"фарфор", "ceramic"}, {"porcelain", "ceramic"},
        {"fabric", "fabric"}, {"ткань", "fabric"}, {"cloth", "fabric"}, {"textile", "fabric"},
        {"leather", "leather"}, {"кожа", "leather"},
        {"carbon_fiber", "carbon_fiber"}, {"carbon", "carbon_fiber"}, {"карбон", "carbon_fiber"}, {"углеволокно", "carbon_fiber"}, {"углепластик", "carbon_fiber"},
        {"painted_metal", "painted_metal"}, {"крашеный_металл", "painted_metal"}, {"окрашенный_металл", "painted_metal"}, {"paint", "painted_metal"},
        {"rusted_metal", "rusted_metal"}, {"rust", "rusted_metal"}, {"ржавый_металл", "rusted_metal"}, {"ржавчина", "rusted_metal"}, {"ржавое_железо", "rusted_metal"},
        {"sand", "sand"}, {"песок", "sand"}, {"soil", "soil"}, {"dirt", "soil"}, {"земля", "soil"}, {"почва", "soil"}, {"грунт", "soil"},
        {"grass", "grass"}, {"трава", "grass"}, {"snow", "snow"}, {"снег", "snow"},
        {"ice", "ice"}, {"лёд", "ice"}, {"лед", "ice"}, {"water", "water"}, {"вода", "water"}};
    std::string s = lower(in);
    for (auto& ch : s) if (ch == ' ' || ch == '-') ch = '_';
    for (auto& p : syn) if (p.first == s) return p.second;
    return "";
}

struct Rule { const char* key; const char* mat; bool exact; bool cyr; };

// Автоопределение по имени детали. Порядок правил важен: первое совпадение побеждает.
inline std::string autoDetect(const std::string& partName) {
    static const std::vector<Rule> rules = {
        {"rubber", "rubber", false, false}, {"buttpad", "rubber", false, false}, {"butt", "rubber", false, false}, {"tire", "rubber", false, false},
        {"резин", "rubber", false, true}, {"шин", "rubber", false, true},
        {"walnut", "walnut", false, false}, {"орех", "walnut", false, true}, {"oak", "oak", true, false}, {"дуб", "oak", false, true},
        {"wood", "wood", false, false}, {"stock", "wood", true, false}, {"handguard", "wood", false, false}, {"forend", "wood", false, false},
        {"plank", "wood", false, false}, {"дерев", "wood", false, true}, {"древес", "wood", false, true}, {"приклад", "wood", false, true}, {"цевь", "wood", false, true},
        {"plastic", "plastic", false, false}, {"polymer", "plastic", false, false}, {"grip", "plastic", true, false}, {"пластик", "plastic", false, true}, {"рукоят", "plastic", false, true},
        {"glass", "glass", false, false}, {"lens", "glass", false, false}, {"window", "glass", false, false}, {"стекл", "glass", false, true}, {"линз", "glass", false, true},
        {"receiver", "painted_metal", false, false}, {"mag", "painted_metal", false, false}, {"magazine", "painted_metal", false, false},
        {"коробк", "painted_metal", false, true}, {"магазин", "painted_metal", false, true},
        {"steel", "steel", false, false}, {"barrel", "steel", false, false}, {"slide", "steel", false, false}, {"bolt", "steel", false, false},
        {"trigger", "steel", false, false}, {"hammer", "steel", false, false}, {"muzzle", "steel", false, false}, {"gastube", "steel", false, false},
        {"gasblock", "steel", false, false}, {"sight", "steel", false, false}, {"frontsight", "steel", false, false}, {"rearsight", "steel", false, false},
        {"selector", "steel", false, false}, {"spring", "steel", false, false}, {"pin", "steel", true, false}, {"screw", "steel", false, false},
        {"rod", "steel", true, false}, {"guard", "steel", true, false}, {"ствол", "steel", false, true}, {"затвор", "steel", false, true},
        {"спуск", "steel", false, true}, {"мушк", "steel", false, true}, {"прицел", "steel", false, true}, {"курок", "steel", false, true},
        {"concrete", "concrete", false, false}, {"бетон", "concrete", false, true}, {"brick", "brick", false, false}, {"кирп", "brick", false, true},
        {"stone", "stone", false, false}, {"rock", "stone", true, false}, {"камен", "stone", false, true},
        {"sand", "sand", false, false}, {"песок", "sand", false, true}, {"soil", "soil", false, false}, {"dirt", "soil", true, false},
        {"земл", "soil", false, true}, {"почв", "soil", false, true}, {"grass", "grass", false, false}, {"трав", "grass", false, true},
        {"snow", "snow", false, false}, {"снег", "snow", false, true}, {"ice", "ice", true, false}, {"лёд", "ice", false, true}, {"лед", "ice", false, true},
        {"water", "water", false, false}, {"вода", "water", false, true}, {"ceramic", "ceramic", false, false}, {"porcelain", "ceramic", false, false},
        {"керам", "ceramic", false, true}, {"фарфор", "ceramic", false, true}, {"fabric", "fabric", false, false}, {"cloth", "fabric", false, false},
        {"ткан", "fabric", false, true}, {"leather", "leather", false, false}, {"кож", "leather", false, true},
        {"carbon", "carbon_fiber", false, false}, {"карбон", "carbon_fiber", false, true}, {"углевол", "carbon_fiber", false, true},
        {"rust", "rusted_metal", false, false}, {"ржав", "rusted_metal", false, true}, {"aluminum", "aluminum", false, false}, {"aluminium", "aluminum", false, false},
        {"алюмин", "aluminum", false, true}, {"copper", "copper", false, false}, {"медь", "copper", false, true}, {"gold", "gold", false, false},
        {"золот", "gold", false, true}, {"iron", "iron", false, false}, {"желез", "iron", false, true}, {"чугун", "iron", false, true},
        {"metal", "steel", false, false}, {"металл", "steel", false, true}};
    std::string s = lower(partName);
    std::vector<std::string> tok; std::string cur;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c >= 0x80) cur += (char)c;
        else { if (!cur.empty()) tok.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) tok.push_back(cur);
    for (auto& r : rules) {
        std::string k = r.key;
        if (r.cyr) { if (s.find(k) != std::string::npos) return r.mat; continue; }
        for (auto& t : tok) {
            std::string tt = t; while (!tt.empty() && std::isdigit((unsigned char)tt.back())) tt.pop_back(); // mag1 -> mag
            if (r.exact ? tt == k : tt.compare(0, k.size(), k) == 0) return r.mat;
        }
    }
    return "";
}

// ---------- процедурные текстуры (бесшовные) ----------
struct TexSet { int w = 0, h = 0; std::vector<uint8_t> a, n, o; }; // albedo(sRGB), normal, ORM (R=AO, G=шероховатость 0.5+дельта, B=маска металла)

inline uint32_t hash2(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u; return h ^ (h >> 16);
}
inline float rnd(int x, int y, int seed) { return (hash2(x, y, seed) & 0xFFFF) / 65535.f; }
inline int wrapi(int a, int p) { return ((a % p) + p) % p; }
inline float vnoise(float x, float y, int px, int py, int seed) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y); float fx = x - xi, fy = y - yi;
    float a = rnd(wrapi(xi, px), wrapi(yi, py), seed), b = rnd(wrapi(xi + 1, px), wrapi(yi, py), seed);
    float c = rnd(wrapi(xi, px), wrapi(yi + 1, py), seed), d = rnd(wrapi(xi + 1, px), wrapi(yi + 1, py), seed);
    float u = fx * fx * (3 - 2 * fx), v = fy * fy * (3 - 2 * fy);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}
inline float fbm(float u, float v, int base, int oct, int seed) {
    float s = 0, amp = .5f, tot = 0; int per = base;
    for (int o = 0; o < oct; o++) { s += amp * vnoise(u * per, v * per, per, per, seed + o * 17); tot += amp; amp *= .5f; per *= 2; }
    return s / tot;
}
inline float cl(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
inline float sst(float a, float b, float x) { float t = cl((x - a) / (b - a)); return t * t * (3 - 2 * t); }
inline float mx(float a, float b, float t) { return a + (b - a) * t; }

struct Px { float r, g, b, h, rough, ao, metal; };

inline Px pix(const Def& d, float u, float v) {
    const float PI = 3.14159265f;
    float R = d.base[0], G = d.base[1], B = d.base[2];
    Px p{R, G, B, 0.5f, 0.5f, 1.f, 1.f};
    auto col = [&](float k) { p.r = R * k; p.g = G * k; p.b = B * k; };
    switch (d.pat) {
    case P_BRUSHED: { float n = vnoise(u * 2, v * 128, 2, 128, 3), m = vnoise(u * 4, v * 40, 4, 40, 5);
        col(.9f + .2f * n); p.rough = .5f + (n - .5f) * .8f; p.h = n * .4f + m * .2f; break; }
    case P_PAINT: { float n = fbm(u, v, 24, 3, 7); col(.97f + .06f * n); p.rough = .5f + (n - .5f) * .2f; p.h = n * .15f; break; }
    case P_RUST: { float m = fbm(u, v, 5, 5, 11), f = fbm(u, v, 32, 3, 13), r = sst(.48f, .62f, m);
        p.r = mx(R * (.9f + .2f * f), d.dark[0] * (.6f + .8f * f), r); p.g = mx(G * (.9f + .2f * f), d.dark[1] * (.6f + .8f * f), r);
        p.b = mx(B * (.9f + .2f * f), d.dark[2] * (.6f + .8f * f), r);
        p.rough = .5f + (r - .3f) * .9f; p.metal = 1.f - r; p.h = r * .6f + f * .3f; break; }
    case P_WOOD: { float warp = fbm(u, v, 3, 4, 17), t = v * 10 + warp * 3.f, s = .5f + .5f * std::sin(2 * PI * t);
        float fib = vnoise(u * 4, v * 256, 4, 256, 19), k = cl((.35f + .65f * std::pow(s, 1.3f)) * .8f + (fib - .5f) * .5f + .1f);
        p.r = mx(d.dark[0], R, k); p.g = mx(d.dark[1], G, k); p.b = mx(d.dark[2], B, k);
        p.h = fib * .5f + s * .3f; p.rough = .5f + (fib - .5f) * .3f; p.metal = 0; p.ao = .85f + .15f * k; break; }
    case P_RUBBER: { float n = fbm(u, v, 64, 2, 23); col(.8f + .4f * n); p.h = n * .4f; p.metal = 0; break; }
    case P_PLASTIC: { float n = fbm(u, v, 48, 2, 29); col(.96f + .08f * n); p.h = n * .1f; p.rough = .5f + (n - .5f) * .1f; p.metal = 0; break; }
    case P_CONCRETE: { float n = fbm(u, v, 4, 5, 31), pit = sst(.9f, .97f, vnoise(u * 64, v * 64, 64, 64, 37));
        col((.7f + .6f * n) * (1 - .4f * pit)); p.h = n * .5f - pit * .5f; p.ao = 1 - .5f * pit; p.metal = 0; break; }
    case P_STONE: { float n = fbm(u, v, 4, 5, 41), rd = 1 - std::fabs(2 * fbm(u, v, 3, 5, 43) - 1), vein = std::pow(rd, 8.f);
        float k = .7f + .5f * n; p.r = mx(R * k, d.dark[0], vein * .8f); p.g = mx(G * k, d.dark[1], vein * .8f); p.b = mx(B * k, d.dark[2], vein * .8f);
        p.h = n * .6f + (1 - vein) * .2f; p.metal = 0; break; }
    case P_BRICK: { const int cols = 2, rows = 8; int row = (int)std::floor(v * rows); float off = (row & 1) * .5f, uu = u * cols + off;
        int c = (int)std::floor(uu); float fx = uu - c, fy = v * rows - row;
        float b = sst(.02f, .045f, std::min(fx, 1 - fx)) * sst(.06f, .11f, std::min(fy, 1 - fy));
        float var = rnd(wrapi(c, cols), wrapi(row, rows), 47), n = fbm(u, v, 16, 3, 49), k = (.75f + .5f * var) * (.9f + .2f * n);
        p.r = mx(d.dark[0], R * k, b); p.g = mx(d.dark[1], G * k, b); p.b = mx(d.dark[2], B * k, b);
        p.h = b * .8f + n * .1f; p.ao = .6f + .4f * b; p.rough = .5f + (1 - b) * .3f; p.metal = 0; break; }
    case P_GLASS: p.h = .5f; p.metal = 0; break;
    case P_CERAMIC: { float n = fbm(u, v, 32, 2, 53); col(.98f + .04f * n); p.h = n * .05f; p.metal = 0; break; }
    case P_FABRIC: { float fu = u * 48, fv = v * 48; int ci = (int)std::floor(fu), cj = (int)std::floor(fv); bool over = ((ci + cj) & 1) != 0;
        float s = over ? std::sin(PI * (fv - cj)) : std::sin(PI * (fu - ci)), h = .5f + .5f * s * (over ? 1.f : .8f), n = fbm(u, v, 128, 2, 59);
        col((.7f + .4f * h) * (.9f + .2f * n)); p.h = h; p.metal = 0; break; }
    case P_LEATHER: { float q = vnoise(u * 32, v * 32, 32, 32, 61), q2 = vnoise(u * 64, v * 64, 64, 64, 67);
        float h = std::pow(1 - std::fabs(2 * q - 1), 2.f) * .6f + q2 * .2f; col(.75f + .35f * h); p.h = h; p.metal = 0; break; }
    case P_CARBON: { float fu = u * 16, fv = v * 16; int ci = (int)std::floor(fu), cj = (int)std::floor(fv);
        float t = (((ci + cj) & 2) == 0) ? std::sin(PI * (fv - cj)) : std::sin(PI * (fu - ci));
        col(.35f + .9f * t); p.h = t * .5f; p.rough = .5f - (t - .5f) * .3f; p.metal = 0; break; }
    case P_SAND: { float n = fbm(u, v, 96, 2, 71), g = vnoise(u * 256, v * 256, 256, 256, 73); col(.85f + .2f * n + .15f * (g - .5f)); p.h = g * .3f + n * .2f; p.metal = 0; break; }
    case P_SOIL: { float n = fbm(u, v, 8, 5, 79); col(.5f + .9f * n); p.h = n; p.metal = 0; break; }
    case P_GRASS: { float bl = vnoise(u * 128, v * 16, 128, 16, 83), n = fbm(u, v, 8, 4, 89);
        float k1 = .6f + .6f * bl, k2 = .7f + .5f * n; p.r = mx(d.dark[0] * k1, R * k2, bl); p.g = mx(d.dark[1] * k1, G * k2, bl); p.b = mx(d.dark[2] * k1, B * k2, bl);
        p.h = bl * .6f; p.metal = 0; break; }
    case P_SNOW: { float n = fbm(u, v, 32, 3, 97); col(.96f + .06f * n); p.h = n * .3f; p.metal = 0; break; }
    case P_ICE: { float n = fbm(u, v, 4, 4, 101), cr = 1 - std::fabs(2 * n - 1), k = (.95f + .1f * n), c = sst(.8f, 1.f, cr) * .5f;
        p.r = mx(R, d.dark[0], c) * k; p.g = mx(G, d.dark[1], c) * k; p.b = mx(B, d.dark[2], c) * k; p.h = cr * .4f; p.metal = 0; break; }
    case P_WATER: { float n = fbm(u, v, 6, 4, 103); col(.9f + .2f * n); p.h = n; p.metal = 0; break; }
    }
    return p;
}

inline uint8_t b8(float x) { return (uint8_t)std::lround(cl(x) * 255.f); }

inline TexSet gen(const Def& d, int N = 256) {
    TexSet t; t.w = t.h = N;
    t.a.assign((size_t)N * N * 4, 255); t.n.assign((size_t)N * N * 4, 255); t.o.assign((size_t)N * N * 4, 255);
    std::vector<float> H((size_t)N * N);
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            Px p = pix(d, (i + .5f) / N, (j + .5f) / N); size_t k = ((size_t)j * N + i);
            H[k] = p.h;
            t.a[k * 4] = b8(p.r); t.a[k * 4 + 1] = b8(p.g); t.a[k * 4 + 2] = b8(p.b);
            t.o[k * 4] = b8(p.ao); t.o[k * 4 + 1] = b8(p.rough); t.o[k * 4 + 2] = b8(p.metal);
        }
    const float K = 6.f;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            float dx = H[(size_t)j * N + wrapi(i + 1, N)] - H[(size_t)j * N + wrapi(i - 1, N)];
            float dy = H[(size_t)wrapi(j + 1, N) * N + i] - H[(size_t)wrapi(j - 1, N) * N + i];
            float nx = -dx * K, ny = -dy * K, nz = 1.f, l = std::sqrt(nx * nx + ny * ny + nz * nz);
            size_t k = ((size_t)j * N + i) * 4;
            t.n[k] = b8(nx / l * .5f + .5f); t.n[k + 1] = b8(ny / l * .5f + .5f); t.n[k + 2] = b8(nz / l * .5f + .5f);
        }
    return t;
}

} // namespace mat
