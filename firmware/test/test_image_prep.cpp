// Host test for image_prep.h. Run: c++ -std=c++11 -I../main test_image_prep.cpp && ./a.out
#include <assert.h>
#include <stdio.h>
#include <vector>

#include "image_prep.h"

static uint32_t packed(int r, int g, int b) { return (r << 16) | (g << 8) | b; }

int main()
{
    // 160x120 image: R = x, G = y, B = 7. Centered square crop is x in [20, 140).
    const int W = 160, H = 120, OUT = 96;
    std::vector<uint8_t> px(W * H * 3);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            uint8_t *p = &px[3 * (y * W + x)];
            p[0] = x; p[1] = y; p[2] = 7;
        }
    RgbImage img = {px.data(), W, H};

    std::vector<float> f(OUT * OUT);
    image_to_features(img, OUT, 0, f.size(), f.data());

    // Top-left output samples near the crop's top-left, bottom-right near its bottom-right.
    assert((uint32_t)f[0] == packed(20, 0, 7));
    assert((uint32_t)f[OUT * OUT - 1] == packed(139, 119, 7));
    // Every sample stays inside the centered square.
    for (float v : f) {
        int r = ((uint32_t)v >> 16) & 0xFF, g = ((uint32_t)v >> 8) & 0xFF;
        assert(r >= 20 && r < 140 && g >= 0 && g < 120);
    }
    // Reading in chunks (as Edge Impulse does) gives the same result as one pass.
    std::vector<float> chunked(OUT * OUT);
    for (size_t off = 0; off < chunked.size(); off += 1000) {
        size_t n = chunked.size() - off < 1000 ? chunked.size() - off : 1000;
        image_to_features(img, OUT, off, n, chunked.data() + off);
    }
    assert(chunked == f);
    // Packed 24-bit values survive the float round-trip exactly.
    uint8_t white[3] = {255, 255, 255};
    RgbImage one = {white, 1, 1};
    float w;
    image_to_features(one, 1, 0, 1, &w);
    assert((uint32_t)w == 0xFFFFFF);

    puts("image_prep: all tests passed");
    return 0;
}
