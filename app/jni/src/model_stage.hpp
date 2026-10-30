// Ядро редактора: детали, команды, экспорт OBJ. Без зависимостей от SDL/GL.
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <set>
#include <array>
#include <cctype>
#include "materials.hpp"

struct V3 { float x = 0, y = 0, z = 0; };
inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3 cross3(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float dot3(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 norm3(V3 a) { float l = std::sqrt(dot3(a, a)); if (l < 1e-20f) return {0, 1, 0}; return {a.x / l, a.y / l, a.z / l}; }

struct Part {
    std::string name;
    std::vector<V3> v;
    std::vector<unsigned> idx;
    V3 pos{0, 0, 0}, rot{0, 0, 0}, scl{1, 1, 1}, col{0.53f, 0.53f, 0.53f};
    bool hasColor = false;       // color= задан явно
    std::string material;        // material= / apply_material (явно)
    std::string matName;         // итоговый материал после определения ("" = обычный цвет)
    std::string matSrc;          // "явно" или "авто"
    bool smooth = false;         // сглаженные нормали
    float smoothAngle = 50.f;    // порог угла сглаживания в градусах
    float tile = 0.25f;          // метров на одно повторение текстуры
    float uvOff[2] = {0, 0};     // случайный сдвиг UV (чтобы дефекты не повторялись)
};

// Построитель выпуклой геометрии вокруг начала координат: грани сами разворачиваются наружу
struct Geo {
    std::vector<V3> v;
    std::vector<unsigned> idx;
    void tri(unsigned a, unsigned b, unsigned c) {
        V3 n = cross3(v[b] - v[a], v[c] - v[a]);
        if (dot3(n, n) < 1e-24f) return;
        V3 ce = {(v[a].x + v[b].x + v[c].x) / 3, (v[a].y + v[b].y + v[c].y) / 3, (v[a].z + v[b].z + v[c].z) / 3};
        if (dot3(n, ce) < 0) std::swap(b, c);
        idx.push_back(a); idx.push_back(b); idx.push_back(c);
    }
    void quad(unsigned a, unsigned b, unsigned c, unsigned d) { tri(a, b, c); tri(a, c, d); }
    // Треугольник с заданным направлением наружу (для невыпуклых форм)
    void triH(unsigned a, unsigned b, unsigned c, V3 hint) {
        V3 n = cross3(v[b] - v[a], v[c] - v[a]);
        if (dot3(n, n) < 1e-24f) return;
        if (dot3(n, hint) < 0) std::swap(b, c);
        idx.push_back(a); idx.push_back(b); idx.push_back(c);
    }
    void quadH(unsigned a, unsigned b, unsigned c, unsigned d, V3 hint) { triH(a, b, c, hint); triH(a, c, d, hint); }
};

inline Geo makeBox(float sx, float sy, float sz) {
    Geo g;
    for (int i = 0; i < 8; i++)
        g.v.push_back({(i & 1 ? 0.5f : -0.5f) * sx, (i & 2 ? 0.5f : -0.5f) * sy, (i & 4 ? 0.5f : -0.5f) * sz});
    g.quad(0, 2, 6, 4); g.quad(1, 3, 7, 5);
    g.quad(0, 1, 5, 4); g.quad(2, 3, 7, 6);
    g.quad(0, 1, 3, 2); g.quad(4, 5, 7, 6);
    return g;
}

inline Geo makeCyl(float r, float h, int n) {
    Geo g; const float PI2 = 6.28318530718f;
    for (int i = 0; i < n; i++) g.v.push_back({r * std::cos(PI2 * i / n), h / 2, r * std::sin(PI2 * i / n)});
    for (int i = 0; i < n; i++) g.v.push_back({r * std::cos(PI2 * i / n), -h / 2, r * std::sin(PI2 * i / n)});
    g.v.push_back({0, h / 2, 0}); g.v.push_back({0, -h / 2, 0});
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        g.quad(i, j, n + j, n + i);
        g.tri(2 * n, i, j);
        g.tri(2 * n + 1, n + i, n + j);
    }
    return g;
}

inline Geo makeCone(float r, float h, int n) {
    Geo g; const float PI2 = 6.28318530718f;
    for (int i = 0; i < n; i++) g.v.push_back({r * std::cos(PI2 * i / n), -h / 2, r * std::sin(PI2 * i / n)});
    g.v.push_back({0, h / 2, 0}); g.v.push_back({0, -h / 2, 0});
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        g.tri(n, i, j);
        g.tri(n + 1, i, j);
    }
    return g;
}

inline Geo makeSphere(float r, int n) {
    Geo g; const float PI = 3.14159265359f;
    int R = std::max(4, n / 2);
    for (int i = 0; i <= R; i++)
        for (int j = 0; j < n; j++) {
            float ph = PI * i / R, th = 2 * PI * j / n;
            g.v.push_back({r * std::sin(ph) * std::cos(th), r * std::cos(ph), r * std::sin(ph) * std::sin(th)});
        }
    for (int i = 0; i < R; i++)
        for (int j = 0; j < n; j++) {
            int j2 = (j + 1) % n;
            g.quad(i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j);
        }
    return g;
}


// ---- Контуры: список точек "a,b;a,b;..." ----
inline std::vector<std::pair<float, float>> parsePairs(const std::string& s) {
    std::vector<std::pair<float, float>> r; std::stringstream ss(s); std::string it;
    while (std::getline(ss, it, ';')) {
        if (it.empty()) continue;
        size_t c = it.find(',');
        if (c == std::string::npos) throw std::runtime_error("точка вида a,b: " + it);
        char* e1 = nullptr; char* e2 = nullptr;
        std::string a = it.substr(0, c), b = it.substr(c + 1);
        float x = std::strtof(a.c_str(), &e1), y = std::strtof(b.c_str(), &e2);
        if (a.empty() || b.empty() || *e1 || *e2) throw std::runtime_error("не число в точке: " + it);
        r.push_back({x, y});
    }
    return r;
}

// Разбиение простого многоугольника (против часовой) на треугольники методом отсечения ушей
inline std::vector<unsigned> earclip(const std::vector<float>& A, const std::vector<float>& B) {
    std::vector<unsigned> out; std::vector<int> id(A.size());
    for (size_t i = 0; i < id.size(); i++) id[i] = (int)i;
    auto cr = [&](int i, int j, int k) { return (A[j] - A[i]) * (B[k] - B[i]) - (B[j] - B[i]) * (A[k] - A[i]); };
    int guard = 0;
    while (id.size() > 3 && guard++ < 100000) {
        int m = (int)id.size(); bool clipped = false;
        for (int i = 0; i < m && !clipped; i++) {
            int p = id[(i + m - 1) % m], c = id[i], q = id[(i + 1) % m];
            if (cr(p, c, q) <= 1e-14f) continue;
            bool ok = true;
            for (int k = 0; k < m && ok; k++) {
                int v = id[k]; if (v == p || v == c || v == q) continue;
                if (cr(p, c, v) >= 0 && cr(c, q, v) >= 0 && cr(q, p, v) >= 0) ok = false;
            }
            if (!ok) continue;
            out.push_back(p); out.push_back(c); out.push_back(q);
            id.erase(id.begin() + i); clipped = true;
        }
        if (!clipped) { // вырожденный случай: срезаем первую вершину
            out.push_back(id[m - 1]); out.push_back(id[0]); out.push_back(id[1]); id.erase(id.begin());
        }
    }
    if (id.size() == 3) { out.push_back(id[0]); out.push_back(id[1]); out.push_back(id[2]); }
    return out;
}

// Профиль в боковом виде (z вперёд, y вверх), выдавленный по оси X на ширину w
inline Geo makeExtrude(std::vector<std::pair<float, float>> P, float w) {
    if (P.size() < 3) throw std::runtime_error("нужно минимум 3 точки");
    float area = 0;
    for (size_t i = 0; i < P.size(); i++) { auto& a = P[i]; auto& b = P[(i + 1) % P.size()]; area += a.first * b.second - b.first * a.second; }
    if (area < 0) std::reverse(P.begin(), P.end());
    int n = (int)P.size(); Geo g; std::vector<float> A, B;
    for (auto& p : P) { A.push_back(p.first); B.push_back(p.second); }
    for (int i = 0; i < n; i++) g.v.push_back({-w / 2, P[i].second, P[i].first});
    for (int i = 0; i < n; i++) g.v.push_back({w / 2, P[i].second, P[i].first});
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n; float da = A[j] - A[i], db = B[j] - B[i];
        g.quadH(i, j, n + j, n + i, {0, -da, db});
    }
    auto tris = earclip(A, B);
    for (size_t t = 0; t + 2 < tris.size(); t += 3) {
        g.triH(n + tris[t], n + tris[t + 1], n + tris[t + 2], {1, 0, 0});
        g.triH(tris[t], tris[t + 1], tris[t + 2], {-1, 0, 0});
    }
    return g;
}

// Тело вращения вокруг оси Y по профилю (радиус,высота) снизу вверх
inline Geo makeLathe(std::vector<std::pair<float, float>> P, int seg) {
    if (P.size() < 2) throw std::runtime_error("нужно минимум 2 точки");
    float area = 0; size_t m = P.size();
    for (size_t i = 0; i < m; i++) { auto& a = P[i]; auto& b = P[(i + 1) % m]; area += a.first * b.second - b.first * a.second; }
    // замыкаем по оси: последняя -> (0,y_last) -> (0,y_first) -> первая
    std::vector<std::pair<float, float>> C = P; C.push_back({0, P.back().second}); C.push_back({0, P.front().second});
    area = 0; for (size_t i = 0; i < C.size(); i++) { auto& a = C[i]; auto& b = C[(i + 1) % C.size()]; area += a.first * b.second - b.first * a.second; }
    if (area < 0) std::reverse(P.begin(), P.end());
    int K = (int)P.size(); Geo g; const float PI2 = 6.28318530718f;
    for (int k = 0; k < K; k++)
        for (int j = 0; j < seg; j++) g.v.push_back({P[k].first * std::cos(PI2 * j / seg), P[k].second, P[k].first * std::sin(PI2 * j / seg)});
    for (int k = 0; k + 1 < K; k++) {
        float dr = P[k + 1].first - P[k].first, dy = P[k + 1].second - P[k].second;
        for (int j = 0; j < seg; j++) {
            int j2 = (j + 1) % seg; float tm = PI2 * (j + 0.5f) / seg;
            V3 hint = {dy * std::cos(tm), -dr, dy * std::sin(tm)};
            g.quadH(k * seg + j, k * seg + j2, (k + 1) * seg + j2, (k + 1) * seg + j, hint);
        }
    }
    for (int e = 0; e < 2; e++) { // крышки, если профиль не доходит до оси
        int k = e ? K - 1 : 0;
        if (P[k].first > 1e-6f) {
            unsigned c = (unsigned)g.v.size(); g.v.push_back({0, P[k].second, 0});
            for (int j = 0; j < seg; j++) g.triH(c, k * seg + j, k * seg + (j + 1) % seg, {0, e ? 1.f : -1.f, 0});
        }
    }
    return g;
}



// Капсула вдоль Y: цилиндр высотой h и две полусферы радиуса r
inline Geo makeCapsule(float r, float h, int seg, int rings) {
    std::vector<std::pair<float, float>> P; const float HP = 1.57079632679f;
    rings = std::max(2, rings);
    P.push_back({0, -h / 2 - r});
    for (int i = 1; i <= rings; i++) { float a = HP * i / rings; P.push_back({r * std::sin(a), -h / 2 - r * std::cos(a)}); }
    for (int i = 0; i <= rings; i++) { float a = HP * i / rings; P.push_back({r * std::cos(a), h / 2 + r * std::sin(a)}); }
    P.back().first = 0; // верхний полюс
    return makeLathe(P, std::max(6, seg));
}

// Список троек/четвёрок "a,b,c;a,b,c;..."
inline std::vector<std::vector<float>> parseList(const std::string& s) {
    std::vector<std::vector<float>> r; std::stringstream ss(s); std::string it;
    while (std::getline(ss, it, ';')) {
        if (it.empty()) continue;
        std::vector<float> row; std::stringstream s2(it); std::string x;
        while (std::getline(s2, x, ',')) {
            char* e = nullptr; float f = std::strtof(x.c_str(), &e);
            if (x.empty() || *e) throw std::runtime_error("не число в списке: " + x);
            row.push_back(f);
        }
        r.push_back(row);
    }
    return r;
}

// Склейка совпадающих вершин и удаление вырожденных треугольников
inline void weld(Geo& g) {
    std::map<std::array<long long, 3>, unsigned> m; std::vector<unsigned> remap(g.v.size()); std::vector<V3> nv;
    for (size_t i = 0; i < g.v.size(); i++) {
        std::array<long long, 3> k = {std::llround(g.v[i].x * 1e7), std::llround(g.v[i].y * 1e7), std::llround(g.v[i].z * 1e7)};
        auto it = m.find(k);
        if (it == m.end()) { unsigned id = (unsigned)nv.size(); m[k] = id; nv.push_back(g.v[i]); remap[i] = id; } else remap[i] = it->second;
    }
    std::vector<unsigned> ni;
    for (size_t t = 0; t + 2 < g.idx.size(); t += 3) {
        unsigned a = remap[g.idx[t]], b = remap[g.idx[t + 1]], c = remap[g.idx[t + 2]];
        if (a == b || b == c || a == c) continue;
        ni.push_back(a); ni.push_back(b); ni.push_back(c);
    }
    g.v = nv; g.idx = ni;
}

// Подразделение поверхности (Loop): каждый уровень даёт x4 треугольников и настоящую геометрию
inline void subdivide(Geo& g, int levels) {
    typedef std::pair<unsigned, unsigned> EK;
    auto key = [](unsigned a, unsigned b) { return a < b ? EK(a, b) : EK(b, a); };
    for (int L = 0; L < levels; L++) {
        if (g.idx.size() / 3 * 4 > 400000) break;
        size_t nv = g.v.size();
        std::map<EK, std::vector<unsigned>> E;
        for (size_t t = 0; t + 2 < g.idx.size(); t += 3) {
            unsigned a = g.idx[t], b = g.idx[t + 1], c = g.idx[t + 2];
            E[key(a, b)].push_back(c); E[key(b, c)].push_back(a); E[key(c, a)].push_back(b);
        }
        std::vector<V3> out = g.v; std::map<EK, unsigned> mid;
        std::vector<std::vector<unsigned>> nb(nv), bnd(nv);
        for (auto& e : E) {
            unsigned a = e.first.first, b = e.first.second; V3 A = g.v[a], B = g.v[b], p;
            if (e.second.size() == 2) {
                V3 C = g.v[e.second[0]], D = g.v[e.second[1]];
                p = {.375f * (A.x + B.x) + .125f * (C.x + D.x), .375f * (A.y + B.y) + .125f * (C.y + D.y), .375f * (A.z + B.z) + .125f * (C.z + D.z)};
            } else p = {.5f * (A.x + B.x), .5f * (A.y + B.y), .5f * (A.z + B.z)};
            mid[e.first] = (unsigned)out.size(); out.push_back(p);
            nb[a].push_back(b); nb[b].push_back(a);
            if (e.second.size() != 2) { bnd[a].push_back(b); bnd[b].push_back(a); }
        }
        for (size_t i = 0; i < nv; i++) {
            if (nb[i].empty()) continue;
            V3 v = g.v[i];
            if (!bnd[i].empty()) {
                if (bnd[i].size() == 2) { V3 p = g.v[bnd[i][0]], q = g.v[bnd[i][1]];
                    out[i] = {.75f * v.x + .125f * (p.x + q.x), .75f * v.y + .125f * (p.y + q.y), .75f * v.z + .125f * (p.z + q.z)}; }
                continue;
            }
            int n = (int)nb[i].size(); float beta = n == 3 ? 3.f / 16 : 3.f / (8 * n); V3 s{0, 0, 0};
            for (unsigned j : nb[i]) { s.x += g.v[j].x; s.y += g.v[j].y; s.z += g.v[j].z; }
            out[i] = {(1 - n * beta) * v.x + beta * s.x, (1 - n * beta) * v.y + beta * s.y, (1 - n * beta) * v.z + beta * s.z};
        }
        std::vector<unsigned> ni;
        for (size_t t = 0; t + 2 < g.idx.size(); t += 3) {
            unsigned a = g.idx[t], b = g.idx[t + 1], c = g.idx[t + 2], ab = mid[key(a, b)], bc = mid[key(b, c)], ca = mid[key(c, a)];
            unsigned q[12] = {a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca};
            ni.insert(ni.end(), q, q + 12);
        }
        g.v = out; g.idx = ni;
    }
}

// Параллелепипед со скруглёнными рёбрами: bs сегментов на скругление (настоящие полигоны)
inline Geo makeRoundBox(float sx, float sy, float sz, float b, int bs) {
    Geo g; float h[3] = {std::fabs(sx) / 2, std::fabs(sy) / 2, std::fabs(sz) / 2};
    float mn = std::min(h[0], std::min(h[1], h[2])); b = std::min(b, mn * .999f); bs = std::max(1, std::min(bs, 12));
    auto coords = [&](float hh) { std::vector<float> c; for (int i = 0; i <= bs; i++) c.push_back(-hh + b * i / bs); for (int i = 0; i <= bs; i++) c.push_back(hh - b + b * i / bs); return c; };
    std::vector<float> C[3] = {coords(h[0]), coords(h[1]), coords(h[2])};
    for (int a = 0; a < 3; a++)
        for (int s = -1; s <= 1; s += 2) {
            int u = (a + 1) % 3, v = (a + 2) % 3; size_t nu = C[u].size(), nvv = C[v].size(); unsigned base = (unsigned)g.v.size();
            for (size_t i = 0; i < nu; i++)
                for (size_t j = 0; j < nvv; j++) {
                    float p[3]; p[a] = s * h[a]; p[u] = C[u][i]; p[v] = C[v][j];
                    float in[3], dd[3], len2 = 0;
                    for (int k = 0; k < 3; k++) { float lim = h[k] - b; in[k] = std::max(-lim, std::min(lim, p[k])); dd[k] = p[k] - in[k]; len2 += dd[k] * dd[k]; }
                    float len = std::sqrt(len2), q[3];
                    for (int k = 0; k < 3; k++) q[k] = len > 1e-9f ? in[k] + dd[k] / len * b : p[k];
                    g.v.push_back({q[0], q[1], q[2]});
                }
            for (size_t i = 0; i + 1 < nu; i++)
                for (size_t j = 0; j + 1 < nvv; j++)
                    g.quad(base + (unsigned)(i * nvv + j), base + (unsigned)((i + 1) * nvv + j), base + (unsigned)((i + 1) * nvv + j + 1), base + (unsigned)(i * nvv + j + 1));
        }
    return g;
}

// Тор вокруг оси Y: R — большой радиус, r — радиус трубки
inline Geo makeTorus(float R, float r, int n, int m) {
    Geo g; const float PI2 = 6.28318530718f; n = std::max(3, n); m = std::max(3, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            float th = PI2 * i / n, ph = PI2 * j / m, rr = R + r * std::cos(ph);
            g.v.push_back({rr * std::cos(th), r * std::sin(ph), rr * std::sin(th)});
        }
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            int i2 = (i + 1) % n, j2 = (j + 1) % m; float th = PI2 * (i + .5f) / n, ph = PI2 * (j + .5f) / m;
            V3 hint = {std::cos(ph) * std::cos(th), std::sin(ph), std::cos(ph) * std::sin(th)};
            g.quadH(i * m + j, i2 * m + j, i2 * m + j2, i * m + j2, hint);
        }
    return g;
}

// Локальная точка -> мировая (масштаб, затем повороты Z, Y, X как в three.js XYZ, затем сдвиг)
inline V3 xform(const Part& p, V3 q) {
    const float D = 3.14159265359f / 180.f;
    q = {q.x * p.scl.x, q.y * p.scl.y, q.z * p.scl.z};
    float cx = std::cos(p.rot.x * D), sx = std::sin(p.rot.x * D);
    float cy = std::cos(p.rot.y * D), sy = std::sin(p.rot.y * D);
    float cz = std::cos(p.rot.z * D), sz = std::sin(p.rot.z * D);
    V3 a = {q.x * cz - q.y * sz, q.x * sz + q.y * cz, q.z};
    V3 b = {a.x * cy + a.z * sy, a.y, -a.x * sy + a.z * cy};
    V3 c = {b.x, b.y * cx - b.z * sx, b.y * sx + b.z * cx};
    return c + p.pos;
}

inline bool flipped(const Part& p) { return p.scl.x * p.scl.y * p.scl.z < 0; }

static inline std::string hexOf(V3 c) {
    char b[16];
    auto h = [](float f) { int v = (int)std::lround(std::min(1.f, std::max(0.f, f)) * 255); return v; };
    std::snprintf(b, sizeof b, "#%02x%02x%02x", h(c.x), h(c.y), h(c.z));
    return b;
}

struct Scene {
    std::vector<Part> parts;
    std::vector<std::string> log;

    Part* find(const std::string& n) { for (auto& p : parts) if (p.name == n) return &p; return nullptr; }
    Part& get(const std::string& n) { Part* p = find(n); if (!p) throw std::runtime_error("нет детали: " + n); return *p; }
    void add(Part p) { if (find(p.name)) throw std::runtime_error("имя занято: " + p.name); parts.push_back(std::move(p)); }

    static float num(const std::string& s) {
        char* e = nullptr; float f = std::strtof(s.c_str(), &e);
        if (s.empty() || *e) throw std::runtime_error("не число: " + s);
        return f;
    }
    static V3 vec(const std::string& s) {
        std::vector<float> a; std::string cur; std::stringstream ss(s);
        while (std::getline(ss, cur, ',')) a.push_back(num(cur));
        if (a.size() == 1) return {a[0], a[0], a[0]};
        if (a.size() != 3) throw std::runtime_error("нужно 3 числа через запятую: " + s);
        return {a[0], a[1], a[2]};
    }
    static V3 color(const std::string& s) {
        if (s.size() != 7 || s[0] != '#') throw std::runtime_error("цвет вида #aabbcc: " + s);
        char* e = nullptr; long v = std::strtol(s.c_str() + 1, &e, 16);
        if (*e) throw std::runtime_error("цвет вида #aabbcc: " + s);
        return {((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f};
    }

    void exec(const std::string& line0) {
        std::string line = line0;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        std::istringstream is(line);
        std::vector<std::string> t; std::string w;
        while (is >> w) t.push_back(w);
        if (t.empty() || t[0].rfind("//", 0) == 0) return;
        std::string c = t[0];
        for (auto& ch : c) ch = (char)std::tolower((unsigned char)ch);
        if (c == "clear") { parts.clear(); return; }
        if (c == "list") { std::string s; for (auto& p : parts) s += p.name + " "; log.push_back(s.empty() ? "пусто" : s); return; }
        if (t.size() < 2) throw std::runtime_error("нужно имя детали");
        const std::string& n = t[1];
        std::vector<std::pair<std::string, std::string>> kv; std::vector<std::string> pos;
        for (size_t i = 2; i < t.size(); i++) {
            size_t e = t[i].find('=');
            if (e != std::string::npos && e > 0) kv.push_back({t[i].substr(0, e), t[i].substr(e + 1)});
            else pos.push_back(t[i]);
        }
        auto K = [&](const char* k, const char* d) -> std::string { for (auto& p : kv) if (p.first == k) return p.second; return d; };
        auto has = [&](const char* k) { for (auto& p : kv) if (p.first == k) return true; return false; };

        if (c == "box" || c == "cylinder" || c == "cone" || c == "sphere" || c == "extrude" || c == "lathe") {
            Part p; p.name = n;
            int seg = std::min(48, std::max(3, (int)num(K("seg", "12"))));
            Geo g;
            if (c == "box") { V3 s = vec(K("size", "0.1,0.1,0.1")); g = makeBox(s.x, s.y, s.z); }
            else if (c == "cylinder") g = makeCyl(num(K("r", "0.05")), num(K("h", "0.1")), seg);
            else if (c == "cone") g = makeCone(num(K("r", "0.05")), num(K("h", "0.1")), seg);
            else if (c == "extrude") g = makeExtrude(parsePairs(K("pts", "")), num(K("w", "0.02")));
            else if (c == "lathe") g = makeLathe(parsePairs(K("prof", "")), std::max(6, seg));
            else g = makeSphere(num(K("r", "0.05")), std::max(4, seg));
            p.v = g.v; p.idx = g.idx;
            if (has("pos")) p.pos = vec(K("pos", ""));
            if (has("rot")) p.rot = vec(K("rot", ""));
            if (has("scale")) p.scl = vec(K("scale", ""));
            if (has("color")) p.col = color(K("color", ""));
            add(p);
            return;
        }
        Part& m = get(n);
        auto arg = [&]() -> const std::string& { if (pos.empty()) throw std::runtime_error("не хватает значения"); return pos[0]; };
        if (c == "move") { V3 d = vec(arg()); m.pos = m.pos + d; }
        else if (c == "pos") m.pos = vec(arg());
        else if (c == "rotate") m.rot = vec(arg());
        else if (c == "scale") m.scl = vec(arg());
        else if (c == "color") m.col = color(arg());
        else if (c == "copy") { Part k = m; k.name = arg(); add(k); }
        else if (c == "mirror") {
            std::string ax = pos.empty() ? "x" : pos[0];
            Part k = m; k.name = n + "_m";
            if (ax == "x") { k.pos.x = -k.pos.x; k.scl.x = -k.scl.x; }
            else if (ax == "y") { k.pos.y = -k.pos.y; k.scl.y = -k.scl.y; }
            else if (ax == "z") { k.pos.z = -k.pos.z; k.scl.z = -k.scl.z; }
            else throw std::runtime_error("ось x, y или z");
            add(k);
        }
        else if (c == "delete") { parts.erase(std::remove_if(parts.begin(), parts.end(), [&](const Part& p) { return p.name == n; }), parts.end()); }
        else throw std::runtime_error("неизвестная команда: " + c);
    }

    // Пересобрать сцену из текста скрипта
    void run(const std::string& script) {
        parts.clear(); log.clear();
        std::stringstream ss(script); std::string line; int i = 0;
        while (std::getline(ss, line)) {
            i++;
            try { exec(line); }
            catch (const std::exception& e) { log.push_back("Строка " + std::to_string(i) + ": " + e.what()); }
        }
        log.push_back("Готово: " + std::to_string(parts.size()) + " деталей");
    }

    int tris() const { int t = 0; for (auto& p : parts) t += (int)p.idx.size() / 3; return t; }

    std::string exportObj() const {
        std::string s = "# Command modeler export\n"; unsigned off = 1; char b[128];
        for (auto& p : parts) {
            s += "o " + p.name + "\n# color " + hexOf(p.col) + "\n";
            for (auto& q : p.v) { V3 w = xform(p, q); std::snprintf(b, sizeof b, "v %.5f %.5f %.5f\n", w.x, w.y, w.z); s += b; }
            bool fl = flipped(p);
            for (size_t i = 0; i + 2 < p.idx.size(); i += 3) {
                unsigned a = p.idx[i] + off, bb = p.idx[i + 1] + off, c = p.idx[i + 2] + off;
                if (fl) std::swap(bb, c);
                std::snprintf(b, sizeof b, "f %u %u %u\n", a, bb, c); s += b;
            }
            off += (unsigned)p.v.size();
        }
        return s;
    }

    // Мировые треугольники для рисования: позиция(3) нормаль(3) цвет(3) на вершину
    void build(std::vector<float>& out) const {
        out.clear();
        for (auto& p : parts) {
            bool fl = flipped(p);
            std::vector<V3> w; for (auto& q : p.v) w.push_back(xform(p, q));
            for (size_t i = 0; i + 2 < p.idx.size(); i += 3) {
                unsigned a = p.idx[i], b = p.idx[i + 1], c = p.idx[i + 2];
                if (fl) std::swap(b, c);
                V3 n = norm3(cross3(w[b] - w[a], w[c] - w[a]));
                unsigned k[3] = {a, b, c};
                for (int j = 0; j < 3; j++) {
                    V3 v = w[k[j]];
                    float d[9] = {v.x, v.y, v.z, n.x, n.y, n.z, p.col.x, p.col.y, p.col.z};
                    out.insert(out.end(), d, d + 9);
                }
            }
        }
    }
};
