#include <stdint.h>
#include <lua.hpp>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <emmintrin.h>

using namespace std;

struct Pixel {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;

    Pixel() : b(0), g(0), r(0), a(0)
    {}

    Pixel(uint8_t b, uint8_t g, uint8_t r, uint8_t a) : b(b), g(g), r(r), a(a)
    {}

    double v() const noexcept {
        return max(max(r, g), b) / 255.0;
    }
};

class Image {
private:
    int w;
    int h;
    Pixel *data;
    vector<Pixel *> rows;

public:
    Image(int w, int h, Pixel *data) : w(w), h(h), data(data), rows(h) {
        for (int y = 0; y < h; y++) {
            rows[y] = &data[w * y];
        }
    }

    const Pixel *operator[](size_t y) const {
        return rows[y];
    }

    Pixel *operator[](size_t y) {
        return rows[y];
    }

    const Pixel &get(int x, int y) const {
        return data[x + w * y];
    }

    void set(int x, int y, Pixel px) {
        data[x + w * y] = px;
    }

    int width() const noexcept {
        return w;
    }

    int height() const noexcept {
        return h;
    }
};

template <class T>
class Mat {
private:
    int w;
    int h;
    vector<T> data;
    vector<T *> rows;

public:
    Mat() : w(0), h(0) {}

    Mat(int w, int h) : w(w), h(h), data(w *h), rows(h) {
        for (int y = 0; y < h; y++) {
            rows[y] = &data[w * y];
        }
    }

    const T *operator[](size_t y) const {
        return rows[y];
    }

    T *operator[](size_t y) {
        return rows[y];
    }

    const T &get(int x, int y) const {
        return rows[y][x];
    }

    void set(int x, int y, T &val) {
        data[x + w * y] = val;
    }

    void lineCopy(int x, int y, const T *line, size_t size) {
        memcpy_s(&(data.data()[x + w * y]), size * sizeof(T), line, size * sizeof(T));
    }

    int width() const noexcept {
        return w;
    }

    int height() const noexcept {
        return h;
    }
};

struct alignas(16) Vec4i {
    int data[4];
};

struct alignas(16) Vec2d {
    double data[2];
};

class Integral {
private:
    int w;
    int h;
    Mat<Vec4i> bgra;
    Mat<Vec2d> msv;

public:
    Integral(const Mat<Pixel> &img) :
        w(img.width() + 1),
        h(img.height() + 1),
        bgra(w, h),
        msv(w, h)
    {
        for (int y = 1; y < h; y++) {
            __m128i ic = { 0 };
            __m128d dc = { 0 };
            for (int x = 1; x < w; x++) {
                const auto &px = img[y - 1][x - 1];
                const double v = px.v();

                __m128i ia = _mm_set_epi32(px.b, px.g, px.r, px.a);
                __m128i ib = _mm_load_si128((__m128i *)bgra[y - 1][x].data);
                __m128i id = _mm_load_si128((__m128i *)bgra[y - 1][x - 1].data);
                ia = _mm_add_epi32(ia, ib);
                ic = _mm_sub_epi32(ic, id);
                ic = _mm_add_epi32(ia, ic);
                _mm_store_si128((__m128i *)bgra[y][x].data, ic);

                __m128d da = _mm_set_pd(v, v * v);
                __m128d db = _mm_load_pd(msv[y - 1][x].data);
                __m128d dd = _mm_load_pd(msv[y - 1][x - 1].data);
                da = _mm_add_pd(da, db);
                dc = _mm_sub_pd(dc, dd);
                dc = _mm_add_pd(da, dc);
                _mm_store_pd(msv[y][x].data, dc);
            }
        }
    }

    Pixel pixel(int xy[4], int size, uint8_t alpha) const {
        __m128i ia = _mm_load_si128((__m128i *)bgra[xy[1]][xy[0]].data);
        __m128i ib = _mm_load_si128((__m128i *)bgra[xy[1]][xy[2]].data);
        __m128i ic = _mm_load_si128((__m128i *)bgra[xy[3]][xy[0]].data);
        __m128i id = _mm_load_si128((__m128i *)bgra[xy[3]][xy[2]].data);
        ia = _mm_add_epi32(ia, id);
        ib = _mm_add_epi32(ib, ic);
        ia = _mm_sub_epi32(ia, ib);
        alignas(16) int bgra[4];
        _mm_store_si128((__m128i *)bgra, ia);
        uint8_t blue = bgra[3] / size;
        uint8_t green = bgra[2] / size;
        uint8_t red = bgra[1] / size;

        return Pixel{ blue, green, red, alpha };
    }

    Pixel pixel(int xy[4], int size) const {
        __m128i ia = _mm_load_si128((__m128i *)bgra[xy[1]][xy[0]].data);
        __m128i ib = _mm_load_si128((__m128i *)bgra[xy[1]][xy[2]].data);
        __m128i ic = _mm_load_si128((__m128i *)bgra[xy[3]][xy[0]].data);
        __m128i id = _mm_load_si128((__m128i *)bgra[xy[3]][xy[2]].data);
        ia = _mm_add_epi32(ia, id);
        ib = _mm_add_epi32(ib, ic);
        ia = _mm_sub_epi32(ia, ib);
        alignas(16) int bgra[4];
        _mm_store_si128((__m128i *)bgra, ia);
        uint8_t blue = bgra[3] / size;
        uint8_t green = bgra[2] / size;
        uint8_t red = bgra[1] / size;
        uint8_t alpha = bgra[0] / size;

        return Pixel{ blue, green, red, alpha };
    }

    double varV(int xy[4], int size) const {
        __m128d da = _mm_load_pd(msv[xy[1]][xy[0]].data);
        __m128d db = _mm_load_pd(msv[xy[1]][xy[2]].data);
        __m128d dc = _mm_load_pd(msv[xy[3]][xy[0]].data);
        __m128d dd = _mm_load_pd(msv[xy[3]][xy[2]].data);
        da = _mm_add_pd(da, dd);
        db = _mm_add_pd(db, dc);
        da = _mm_sub_pd(da, db);
        alignas(16) double ms[2];
        _mm_store_pd(ms, da);
        return size * ms[0] - ms[1] * ms[1];
    }
};

void padding(const Image &src, Mat<Pixel> &dst, int blur) {
    // copy
    for (int y = 0; y < src.height(); y++) {
        dst.lineCopy(blur, y + blur, src[y], src.width());
    }

    // padding
    for (int y = 0; y < blur; y++) {
        for (int x = 0; x < blur; x++) {
            dst[y][x] = src[0][0];
            dst[y][x + blur + src.width()] = src[0][src.width() - 1];
            dst[y + blur + src.height()][x] = src[src.height() - 1][0];
            dst[y + blur + src.height()][x + blur - src.width()] = src[src.height() - 1][src.width() - 1];
        }
    }
    for (int x = 0; x < src.width(); x++) {
        for (int y = 0; y < blur; y++) {
            dst[y][x + blur] = src[0][x];
            dst[y + blur + src.height()][x + blur] = src[src.height() - 1][x];
        }
    }
    for (int y = 0; y < src.height(); y++) {
        for (int x = 0; x < blur; x++) {
            dst[y + blur][x] = src[y][0];
            dst[y + blur][x + blur + src.width()] = src[y][src.width() - 1];
        }
    }
}

int kuwahara(lua_State *L) {
    Pixel *data = reinterpret_cast<Pixel *>(lua_touserdata(L, 1));
    const int w = lua_tointeger(L, 2);
    const int h = lua_tointeger(L, 3);
    const int blur = lua_tointeger(L, 4);
    const bool alpha = lua_toboolean(L, 5);
    const int threads = clamp(lua_tointeger(L, 6), 1, omp_get_max_threads());
    if (blur <= 0) {
        return 1;
    }
    const int subSize = (blur + 1) * (blur + 1);
    Image img(w, h, data);
    Mat<Pixel> padded(w + blur * 2, h + blur * 2);
    padding(img, padded, blur);
    Integral integral(padded);

#pragma omp parallel for num_threads(threads)
    for (int y = 0; y < h; y++) {
        int yi = y + blur + 1;
        for (int x = 0; x < w; x++) {
            int xi = x + blur + 1;
            int xy[4][4] = {
                {xi, yi, xi - blur - 1, yi - blur - 1},
                {xi + blur, yi, xi - 1, yi - blur - 1},
                {xi, yi + blur, xi - blur - 1, yi - 1},
                {xi + blur, yi + blur, xi - 1, yi - 1},
            };

            double vars[4] = {
                integral.varV(xy[0], subSize),
                integral.varV(xy[1], subSize),
                integral.varV(xy[2], subSize),
                integral.varV(xy[3], subSize),
            };
            int vi = min_element(vars, vars + 4) - vars;

            if (alpha) {
                img[y][x] = integral.pixel(xy[vi], subSize);
            }
            else {
                img[y][x] = integral.pixel(xy[vi], subSize, img[y][x].a);
            }
        }
    }

    return 1;
}

static luaL_Reg functions[] = {
    {"kuwahara", kuwahara},
    {nullptr, nullptr},
};

extern "C" __declspec(dllexport) int luaopen_KaroterraOilPainting(lua_State * L) {
    luaL_register(L, "KaroterraOilPainting", functions);
    return 1;
}
