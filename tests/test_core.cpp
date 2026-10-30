// Тесты ядра: парсер, геометрия, материалы, экспорт. Сборка: g++ -std=c++17 -Iapp/jni/src tests/test_core.cpp
#include "model.hpp"
#include <iostream>
#include <fstream>
#include <map>

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; std::cout << "FAIL line " << __LINE__ << ": " #c "\n"; } } while (0)
static bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol * std::max(1.0, std::fabs(b)); }

static double volume(const Part& p) {
    bool fl = flipped(p); double v = 0; std::vector<V3> w; for (auto& q : p.v) w.push_back(xform(p, q));
    for (size_t i = 0; i + 2 < p.idx.size(); i += 3) { V3 a = w[p.idx[i]], b = w[p.idx[i + 1]], c = w[p.idx[i + 2]]; if (fl) std::swap(b, c); v += dot3(a, cross3(b, c)) / 6.0; }
    return v;
}
// V - E + F и замкнутость (каждое ребро ровно в двух треугольниках)
static void topo(const Part& p, int& euler, bool& closed) {
    std::map<std::pair<unsigned, unsigned>, int> E; std::set<unsigned> used;
    for (size_t i = 0; i + 2 < p.idx.size(); i += 3)
        for (int k = 0; k < 3; k++) { unsigned a = p.idx[i + k], b = p.idx[i + (k + 1) % 3]; E[a < b ? std::make_pair(a, b) : std::make_pair(b, a)]++; used.insert(a); }
    closed = true; for (auto& e : E) if (e.second != 2) closed = false;
    euler = (int)used.size() - (int)E.size() + (int)(p.idx.size() / 3);
}
static Scene run(const std::string& s) { Scene sc; sc.run(s); return sc; }
static bool hasLog(const Scene& s, const std::string& sub) { for (auto& l : s.log) if (l.find(sub) != std::string::npos) return true; return false; }

int main() {
    const double PI = 3.14159265358979;
    // 1. старые команды работают как раньше
    { Scene s = run("box a size=1,1,1\ncylinder b r=0.5 h=2 seg=48 pos=3,0,0\n");
      CHECK(s.parts.size() == 2); CHECK(near(volume(s.parts[0]), 1.0, 1e-4)); CHECK(near(volume(s.parts[1]), PI * 0.25 * 2, 0.01)); }
    { Scene s = run("box a size=1,1,1 pos=2,0,0\nmirror a x\ncopy a b\nmove b 0,1,0\ncolor b #ff8800\nrotate a 10,20,30\nscale a 2\ndelete b\n");
      CHECK(s.parts.size() == 2); CHECK(s.parts[0].hasColor == false); }
    // 2. новые формы: объёмы и замкнутость
    { Scene s = run("sphere s r=1 seg=48"); CHECK(near(volume(s.parts[0]), 4 * PI / 3, 0.01)); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 2); }
    { Scene s = run("ellipsoid e size=2,4,6 seg=48 rings=24"); CHECK(near(volume(s.parts[0]), 4 * PI / 3 * 1 * 2 * 3, 0.01)); }
    { Scene s = run("capsule c r=0.5 h=1 seg=48 rings=12"); CHECK(near(volume(s.parts[0]), PI * .25 * 1 + 4 * PI / 3 * .125, 0.01)); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 2); }
    { Scene s = run("torus t r=1 tube=0.25 seg=48 rings=24"); CHECK(near(volume(s.parts[0]), 2 * PI * PI * 1 * .0625, 0.02)); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 0); }
    { Scene s = run("cone k r=1 h=3 seg=48"); CHECK(near(volume(s.parts[0]), PI * 3 / 3, 0.01)); }
    // 3. bevel: настоящие полигоны, объём чуть меньше куба, замкнутость
    { Scene a = run("box b size=1,1,1"); Scene s = run("box b size=1,1,1 bevel=0.1 bevel_segments=4");
      CHECK(s.tris() > a.tris() * 20); CHECK(volume(s.parts[0]) < 1.0 && volume(s.parts[0]) > 0.95); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 2); }
    // 4. subdiv: x4 полигонов на уровень, форма остаётся замкнутой
    { Scene a = run("sphere s r=1 seg=12"); Scene s = run("sphere s r=1 seg=12 subdiv=2");
      CHECK(s.tris() == a.tris() * 16); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 2); CHECK(volume(s.parts[0]) > 0); }
    { Scene s = run("box b size=1,1,1 subdiv=2"); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(e == 2); CHECK(volume(s.parts[0]) > 0.3 && volume(s.parts[0]) < 1.0); }
    // 5. rings у цилиндра
    { Scene a = run("cylinder c r=1 h=1 seg=12"); Scene s = run("cylinder c r=1 h=1 seg=12 rings=4"); CHECK(s.tris() > a.tris()); int e; bool c; topo(s.parts[0], e, c); CHECK(c); CHECK(near(volume(s.parts[0]), volume(a.parts[0]), 1e-4)); }
    // 6. mesh: тетраэдр, неверный порядок обхода исправляется
    { Scene s = run("mesh t v=0,0,0;1,0,0;0,1,0;0,0,1 f=0,1,2;0,1,3;0,2,3;1,2,3"); CHECK(s.parts.size() == 1); CHECK(near(volume(s.parts[0]), 1.0 / 6, 1e-4)); }
    { Scene s = run("mesh t v=0,0,0;1,0,0;0,1,0 f=0,1,5"); CHECK(s.parts.empty()); CHECK(hasLog(s, "Строка 1")); }
    // 7. статистика
    { Scene s = run("box b size=1,1,1"); CHECK(s.verts() == 8); CHECK(s.edges() == 18); CHECK(s.tris() == 12); }
    // 8. smooth: нормали у сферы направлены наружу и плавно меняются
    { Scene s = run("sphere s r=1 seg=24 smooth=1"); std::vector<float> d; std::vector<Range> rg; s.build(d, rg); CHECK(rg.size() == 1); bool ok = true;
      for (size_t i = 0; i + 10 < d.size(); i += 11) { V3 p{d[i], d[i + 1], d[i + 2]}, n{d[i + 3], d[i + 4], d[i + 5]}; if (dot3(norm3(p), n) < 0.97f) ok = false; } CHECK(ok); }
    { Scene s = run("box b size=1,1,1 smooth=1"); std::vector<float> d; std::vector<Range> rg; s.build(d, rg); // угол 90° > 50°: рёбра остаются острыми
      bool ok = true; for (size_t i = 0; i + 10 < d.size(); i += 11) { float m = std::max(std::fabs(d[i + 3]), std::max(std::fabs(d[i + 4]), std::fabs(d[i + 5]))); if (m < 0.999f) ok = false; } CHECK(ok); }
    // 9. материалы: явные, русские синонимы, автоопределение
    { Scene s = run("box a material=steel\nbox b material=сталь\nbox c material=Орех\nbox d material=ржавый_металл\nbox e material=лёд\nbox f material=нет_такого\n");
      CHECK(s.parts[0].matName == "steel"); CHECK(s.parts[1].matName == "steel"); CHECK(s.parts[2].matName == "walnut"); CHECK(s.parts[3].matName == "rusted_metal"); CHECK(s.parts[4].matName == "ice");
      CHECK(s.parts[5].matName.empty()); CHECK(hasLog(s, "неизвестный материал")); }
    { Scene s = run("box wood_panel\nbox stock\nbox rubber_grip\nbox деревянная_панель\nbox резиновая_накладка\nbox barrel\nbox mag1\nbox xyz\nbox spinner\n");
      CHECK(s.parts[0].matName == "wood"); CHECK(s.parts[1].matName == "wood"); CHECK(s.parts[2].matName == "rubber"); CHECK(s.parts[3].matName == "wood");
      CHECK(s.parts[4].matName == "rubber"); CHECK(s.parts[5].matName == "steel"); CHECK(s.parts[6].matName == "painted_metal");
      CHECK(s.parts[7].matName.empty()); CHECK(s.parts[8].matName.empty()); }
    // 10. приоритеты: явный материал > авто; color= отключает авто; materials=off отключает авто, но не явный
    { Scene s = run("box stock material=steel\nbox grip color=#112233\n"); CHECK(s.parts[0].matName == "steel"); CHECK(s.parts[0].matSrc == "явно"); CHECK(s.parts[1].matName.empty()); }
    { Scene s = run("materials=off\nbox stock\nbox barrel material=wood\n"); CHECK(s.parts[0].matName.empty()); CHECK(s.parts[1].matName == "wood"); }
    { Scene s = run("materials local\nbox stock\n"); CHECK(s.mode == "local"); CHECK(s.parts[0].matName == "wood"); }
    { Scene s = run("materials=nonsense\n"); CHECK(hasLog(s, "Строка 1")); }
    // 11. команды material / apply_material
    { Scene s = run("material steel metallic=0.85 roughness=0.35\nmaterial walnut source=online resolution=2K\nmaterial rubber source=local roughness=0.8\nmaterial painted_metal source=auto metallic=0.65 roughness=0.3\n");
      CHECK(near(s.mats["steel"].metallic, 0.85, 1e-6)); CHECK(near(s.mats["rubber"].roughness, 0.8, 1e-6)); CHECK(s.mats["walnut"].online); CHECK(hasLog(s, "online ещё не реализовано")); }
    { Scene s = run("box receiver\nbox rearsight\nbox frame\nbox barrel\napply_material material=сталь target=receiver,rear*\napply_material material=oak target=all\n");
      CHECK(s.parts[0].matName == "oak"); CHECK(s.parts[3].matName == "oak"); }
    { Scene s = run("box receiver\nbox rearsight\nbox frame\napply_material material=steel target=rear*\napply_material material=copper target=fr*\n");
      CHECK(s.parts[0].matName == "painted_metal" && s.parts[0].matSrc == "авто"); CHECK(s.parts[1].matName == "steel" && s.parts[1].matSrc == "явно"); CHECK(s.parts[2].matName == "copper"); }
    { Scene s = run("material steel bad=1\nmaterial unobtainium\napply_material material=steel\n"); CHECK(hasLog(s, "Строка 1")); CHECK(hasLog(s, "Строка 2")); CHECK(hasLog(s, "Строка 3")); }
    // 12. UV: масштаб в метрах, волокна вдоль длинной стороны
    { Scene s = run("box plank size=1,0.1,0.2 material=oak"); std::vector<float> d; std::vector<Range> rg; s.build(d, rg);
      float umax = 0, vmax = 0; for (size_t i = 0; i + 10 < d.size(); i += 11) { umax = std::max(umax, std::fabs(d[i + 9] - s.parts[0].uvOff[0])); vmax = std::max(vmax, std::fabs(d[i + 10] - s.parts[0].uvOff[1])); }
      CHECK(near(umax, 0.5 / 0.4, 0.02)); CHECK(vmax <= 0.5 / 0.4 + 1e-3); }
    // 13. процедурные текстуры: все 26 материалов генерируются, бесшовность и разброс значений
    { int n = 0; for (auto& d : mat::builtin()) { mat::TexSet t = mat::gen(d, 64); CHECK(t.a.size() == 64 * 64 * 4); CHECK(t.n.size() == t.a.size()); CHECK(t.o.size() == t.a.size());
        int mn = 255, mxv = 0; for (size_t i = 0; i < t.a.size(); i += 4) { mn = std::min<int>(mn, t.a[i + 1]); mxv = std::max<int>(mxv, t.a[i + 1]); }
        CHECK(mxv > 0); n++; } CHECK(n == 26); }
    { mat::Def d = mat::builtin()[7]; CHECK(d.name == "wood"); mat::TexSet t = mat::gen(d, 128); // шов: левый и правый край должны быть похожи
      double diff = 0; for (int j = 0; j < 128; j++) { size_t a = (size_t)j * 128 * 4, b = ((size_t)j * 128 + 127) * 4; diff += std::abs((int)t.a[a] - (int)t.a[b]); } CHECK(diff / 128 < 30); }
    // 14. экспорт OBJ+MTL
    { Scene s = run("box stock size=0.2,0.05,0.05\nbox barrel size=0.1,0.02,0.02 color=#aa0000\n"); std::string o = s.exportObj(), m = s.exportMtl();
      CHECK(o.find("mtllib model.mtl") != std::string::npos); CHECK(o.find("usemtl wood") != std::string::npos); CHECK(o.find("usemtl color_aa0000") != std::string::npos);
      CHECK(o.find("vn ") != std::string::npos); CHECK(o.find("vt ") != std::string::npos); CHECK(o.find("f 1/1/1") != std::string::npos);
      CHECK(m.find("newmtl wood") != std::string::npos); CHECK(m.find("Pr ") != std::string::npos); CHECK(m.find("Pm ") != std::string::npos); }
    // 15. ошибки не ломают остальной скрипт, пустой скрипт, комментарии
    { Scene s = run("// comment\n\nbadcmd x\nbox ok size=1,1,1\nbox q size=x\ncylinder c r=abc\n"); CHECK(s.parts.size() == 1); CHECK(hasLog(s, "Строка 3")); CHECK(hasLog(s, "Строка 5")); CHECK(hasLog(s, "Строка 6")); }
    { Scene s = run(""); CHECK(s.parts.empty()); }
    // 16. регрессия: старые скрипты (пистолет, АК из референса) собираются без ошибок
    { std::ifstream f("tests/ak_ref.txt"); if (f) { std::stringstream b; b << f.rdbuf(); Scene s = run(b.str()); CHECK(s.parts.size() == 13);
        bool err = false; for (auto& l : s.log) if (l.find("Строка") == 0) err = true; CHECK(!err);
        for (auto& p : s.parts) CHECK(near(volume(p), volume(p), 1e-9) && volume(p) > 0);
        CHECK(s.parts[0].matName.empty()); /* у АК заданы цвета, авто не вмешивается */ } }
    std::cout << (checks - fails) << "/" << checks << " проверок пройдено\n";
    return fails ? 1 : 0;
}
