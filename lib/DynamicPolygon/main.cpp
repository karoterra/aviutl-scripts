#include <stdint.h>
#include <lua.hpp>
#include <vector>
#include <algorithm>
#include <memory>

#ifdef _DEBUG

#include <cstdio>
#include <Windows.h>
#include <tchar.h>

#endif // _DEBUG

using namespace std;

struct Pixel {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;

    Pixel(uint32_t color) {
        b = color & 0xFF;
        g = (color >> 8) & 0xFF;
        r = (color >> 16) & 0xFF;
        a = (color >> 24) & 0xFF;
    }

    Pixel(uint32_t color, uint8_t alpha) {
        b = color & 0xFF;
        g = (color >> 8) & 0xFF;
        r = (color >> 16) & 0xFF;
        a = alpha;
    }
};

struct Image {
    Pixel *data;
    int width;
    int height;

    inline void setPixel(int x, int y, const Pixel &p) const {
        data[x + width * y] = p;
    }
};

struct Point {
    int x;
    int y;
};

struct Edge {
    double x;
    int y0;
    int y1;
    bool downward;
    double a;

    inline void updateX() {
        x += a;
    }

    Edge clone() const {
        return Edge{ x, y0, y1, downward, a };
    }
};

using VecPoint = vector<Point>;
using EdgePtr = unique_ptr<Edge>;
using VecEdgePtr = vector<EdgePtr>;

class EdgePtrCompare {
public:
    bool operator()(const EdgePtr &e0, const EdgePtr &e1) const noexcept {
        return e0->x < e1->x;
    }
};

using LineFiller = void(*)(const Image &, VecEdgePtr::iterator, VecEdgePtr::iterator, const int, const Pixel &, const int);


#ifdef _DEBUG

// デバッグテキスト出力
template <typename ... Args>
void debug_printf(LPCTSTR fmt, Args ... args) {
    TCHAR buf[1024];
    _sntprintf_s(&buf[0], 1024, _TRUNCATE, fmt, args ...);
    OutputDebugString(&buf[0]);
}

#endif // _DEBUG


// 辺リストの挿入ソート
void insertionSort(VecEdgePtr::iterator first, VecEdgePtr::iterator last) {
    if (distance(first, last) < 2) return;

    for (auto e = first + 1; e < last; e++) {
        auto v = move(*e);
        bool inserted = false;
        for (auto s = e; s > first; s--) {
            if (v->x < (*(s - 1))->x) {
                *s = move(*(s - 1));
            }
            else {
                *s = move(v);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            *first = move(v);
        }
    }
}

// 四捨五入
int roundDouble(double x) {
    if (x > 0) {
        x += 0.5;
    }
    else {
        x -= 0.5;
    }
    return (int)x;
}

// luaスタックから頂点リストを取得
void getPoints(lua_State *L, const int index, VecPoint &points, const int n, const int ox, const int oy) {
    points.clear();
    for (int i = 2; i <= n; i += 2) {
        Point p;
        lua_rawgeti(L, index, i - 1);
        p.x = roundDouble(lua_tonumber(L, -1)) + ox;
        lua_rawgeti(L, index, i);
        p.y = roundDouble(lua_tonumber(L, -1)) + oy;
        lua_pop(L, 2);
        if (points.empty() || p.x != (points.back()).x || p.y != (points.back()).y) {
            points.push_back(p);
        }
    }
}

// 頂点リストから辺リストを作成
void makeEdges(const VecPoint &vertexes, VecEdgePtr &edges) {
    edges.clear();
    const auto vertexCount = vertexes.size();
    if (vertexCount == 0) {
        return;
    }

    for (size_t i = 0; i < vertexCount; i++) {
        const Point &v0 = vertexes[i];
        Point v1 = vertexes[(i + 1) % vertexCount];
        if (v0.y == v1.y) {
            continue;
        }
        const Point &v2 = vertexes[(i + 2) % vertexCount];

        EdgePtr edge = make_unique<Edge>();
        edge->downward = v0.y < v1.y;
        edge->x = (edge->downward) ? v0.x : v1.x;
        edge->a = (double)(v1.x - v0.x) / (double)(v1.y - v0.y);

        if ((v1.y - v0.y) * (v1.y - v2.y) <= 0) {
            if (edge->downward) {
                v1.y--;
            }
            else {
                v1.y++;
                edge->updateX();
            }
        }

        edge->y0 = (edge->downward) ? v0.y : v1.y;
        edge->y1 = (edge->downward) ? v1.y : v0.y;
        edges.push_back(move(edge));
    }
}

// ラインの一区間を塗る
void fillSection(const Image &image, const EdgePtr &edge0, const EdgePtr &edge1, const int y, const Pixel &color, const int antialias) {
    const double x0 = edge0->x;
    const double x1 = edge1->x;
    const int x0Int = roundDouble(edge0->x);
    const int x1Int = roundDouble(edge1->x);

    if (antialias == 1) {
        Pixel color0 = color;
        for (int x = x0Int; x <= x1Int; x++) {
            const double xLeft = x - 0.5, xRight = x + 0.5;
            const double yBottom = y + 0.5, yTop = y - 0.5;
            const double yLeft0 = (xLeft - edge0->x) / edge0->a + y;
            const double yRight0 = (xRight - edge0->x) / edge0->a + y;
            const double yCenter0 = (yLeft0 + yRight0) / 2;
            const double yLeft1 = (xLeft - edge1->x) / edge1->a + y;
            const double yRight1 = (xRight - edge1->x) / edge1->a + y;
            const double yCenter1 = (yLeft1 + yRight1) / 2;
            double area = 1.0;
            // edge0
            if (-1 <= edge0->a && edge0->a <= 1) {
                if (xLeft <= edge0->x && edge0->x <= xRight) {
                    area -= edge0->x - xLeft;
                }
            }
            else if (edge0->a > 1) {
                if (yTop <= yCenter0 && yCenter0 <= yBottom) {
                    area -= yBottom - yCenter0;
                }
            }
            else {
                if (yTop <= yCenter0 && yCenter0 <= yBottom) {
                    area -= yCenter0 - yTop;
                }
            }
            // edge1
            if (-1 <= edge1->a && edge1->a <= 1) {
                if (xLeft <= edge1->x && edge1->x <= xRight) {
                    area -= xRight - edge1->x;
                }
            }
            else if (edge1->a > 1) {
                if (yTop <= yCenter1 && yCenter1 <= yBottom) {
                    area -= yCenter1 - yTop;
                }
            }
            else {
                if (yTop <= yCenter1 && yCenter1 <= yBottom) {
                    area -= yBottom - yCenter1;
                }
            }
            area = clamp(area, 0.0, 1.0);
            color0.a = static_cast<uint8_t>(255 * area);
            image.setPixel(x, y, color0);
        }
    }
    else if (antialias == 2) {
        Pixel color0 = color;
        const double x0p = edge0->x + edge0->a * 0.5;
        const double x0n = x0p - edge0->a;
        const double x1p = edge1->x + edge1->a * 0.5;
        const double x1n = x1p - edge1->a;
        for (int x = x0Int; x <= x1Int; x++) {
            const double xLeft = x - 0.5, xRight = x + 0.5;
            const double yBottom = y + 0.5, yTop = y - 0.5;
            const double yLeft0 = (xLeft - edge0->x) / edge0->a + y;
            const double yRight0 = (xRight - edge0->x) / edge0->a + y;
            const double yLeft1 = (xLeft - edge1->x) / edge1->a + y;
            const double yRight1 = (xRight - edge1->x) / edge1->a + y;
            double area = 1.0;
            // edge0についての面積計算
            if (xLeft <= x0p && x0p <= xRight) {
                if (xLeft <= x0n && x0n <= xRight) {    // 下も上もクロス
                    area -= (x0p - xLeft + x0n - xLeft) / 2;
                }
                else {  // 下だけクロス
                    if (edge0->a > 0) {
                        area -= (x0p - xLeft) * (yBottom - yLeft0) / 2;
                    }
                    else {
                        area -= area - (xRight - x0p) * (yBottom - yRight0) / 2;
                    }
                }
            }
            else {
                if (xLeft <= x0n && x0n <= xRight) {    // 上だけクロス
                    if (edge0->a > 0) {
                        area -= area - (xRight - x0n) * (yRight0 - yTop) / 2;
                    }
                    else {
                        area -= (x0n - xLeft) * (yLeft0 - yTop) / 2;
                    }
                }
                else if (yTop <= yLeft0 && yLeft0 <= yBottom) {  // 下も上もクロスしない
                    if (edge0->a > 0) {
                        area -= (yBottom - yLeft0 + yBottom - yRight0) / 2;
                    }
                    else {
                        area -= (yLeft0 - yTop + yRight0 - yTop) / 2;
                    }
                }
            }
            // edge1についての面積計算
            if (xLeft <= x1p && x1p <= xRight) {
                if (xLeft <= x1n && x1n <= xRight) {    // 下も上もクロス
                    area -= (xRight - x1p + xRight - x1n) / 2;
                }
                else {  // 下だけクロス
                    if (edge1->a > 0) {
                        area -= area - (x1p - xLeft) * (yBottom - yLeft1) / 2;
                    }
                    else {
                        area -= (xRight - x1p) * (yBottom - yRight1) / 2;
                    }
                }
            }
            else {
                if (xLeft <= x1n && x1n <= xRight) {    // 上だけクロス
                    if (edge1->a > 0) {
                        area -= (xRight - x1n) * (yRight1 - yTop) / 2;
                    }
                    else {
                        area -= area - (x1n - xLeft) * (yLeft1 - yTop) / 2;
                    }
                }
                else if (yTop <= yRight1 && yRight1 <= yBottom) {  // 下も上もクロスしない
                    if (edge1->a > 0) {
                        area -= (yLeft1 - yTop + yRight1 - yTop) / 2;
                    }
                    else {
                        area -= (yBottom - yLeft1 + yBottom - yRight1) / 2;
                    }
                }
            }
            area = clamp(area, 0.0, 1.0);
            color0.a = static_cast<uint8_t>(255 * area);
            image.setPixel(x, y, color0);
        }
    }
    else {
        for (int x = x0Int; x <= x1Int; x++) {
            image.setPixel(x, y, color);
        }
    }
}

// Crossing Number
void fill_CN(const Image &image, VecEdgePtr::iterator first, VecEdgePtr::iterator last, const int y, const Pixel &color, const int antialias) {
    for (auto edge0 = first; distance(edge0, last) >= 2; edge0 += 2) {
        auto edge1 = edge0 + 1;
        fillSection(image, *edge0, *edge1, y, color, antialias);

        (*edge0)->updateX();
        (*edge1)->updateX();
    }
}

// Winding Number
void fill_WN(const Image &image, VecEdgePtr::iterator first, VecEdgePtr::iterator last, const int y, const Pixel &color, const int antialias) {
    for (auto i = first; distance(i, last) >= 2; i++) {
        auto e0 = i;
        int wn = ((*e0)->downward) ? 1 : -1;
        for (i++; i < last; i++) {
            wn += ((*i)->downward) ? 1 : -1;
            if (wn == 0) break;
        }
        if (i == last) break;

        auto e1 = i;
        fillSection(image, *e0, *e1, y, color, antialias);

        for (auto j = e0; j <= e1; j++) {
            (*j)->updateX();
        }
    }
}

// ポリゴンを塗りつぶす
int fillPoly(lua_State *L) {
    const Image image{
        reinterpret_cast<Pixel *>(lua_touserdata(L, 1)),  // data
        lua_tointeger(L, 2),    // width
        lua_tointeger(L, 3)     // height
    };
    const int tableSize = lua_tointeger(L, 5);
    const Pixel color(lua_tointeger(L, 6), 255);
    const int fillMode = lua_tointeger(L, 7);
    const int antialias = lua_tointeger(L, 8);

    LineFiller filler;
    switch (fillMode)
    {
    case 1:
        filler = fill_CN;
        break;
    case 2:
        filler = fill_WN;
        break;
    default:
        return 1;
    }

    VecPoint points;
    getPoints(L, 4, points, tableSize, image.width / 2, image.height / 2);
    VecEdgePtr edges;
    makeEdges(points, edges);

    size_t activeIndexEnd = edges.size();
    size_t activeIndexStart = activeIndexEnd;

    for (int y = 0; y < image.height; y++) {
        // アクティブにする
        for (size_t i = activeIndexStart; i > 0; i--) {
            if (edges[i - 1]->y0 == y) {
                activeIndexStart--;
                EdgePtr edge = move(edges[i - 1]);
                edges[i - 1] = move(edges[activeIndexStart]);
                edges[activeIndexStart] = move(edge);
            }
        }
        // 非アクティブにする
        for (size_t i = activeIndexEnd; i > activeIndexStart; i--) {
            if (edges[i - 1]->y1 < y) {
                activeIndexEnd--;
                EdgePtr edge = move(edges[i - 1]);
                edges[i - 1] = move(edges[activeIndexEnd]);
                edges[activeIndexEnd] = move(edge);
            }
        }

        insertionSort(edges.begin() + activeIndexStart, edges.begin() + activeIndexEnd);

        filler(image, edges.begin() + activeIndexStart, edges.begin() + activeIndexEnd, y, color, antialias);
    }

    return 1;
}

static luaL_Reg functions[] = {
    {"fillPoly", fillPoly},
    {nullptr, nullptr},
};

extern "C" __declspec(dllexport) int luaopen_KaroterraDynamicPolygon(lua_State * L) {
    luaL_register(L, "KaroterraDynamicPolygon", functions);
    return 1;
}
