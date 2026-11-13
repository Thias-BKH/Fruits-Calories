#pragma once
// Turns a decoded RGB888 frame into the features the Edge Impulse model expects.
// Pure C++ with no ESP-IDF dependencies so it can be unit-tested on a PC (see test/).

#include <stddef.h>
#include <stdint.h>

struct RgbImage {
    const uint8_t *pixels;  // RGB888, row-major, 3 bytes per pixel
    int width;
    int height;
};

// Matches the model's EI_CLASSIFIER_RESIZE_FIT_SHORTEST: take the largest centered
// square, then scale it to out_size x out_size (nearest neighbour).
// Writes `length` features starting at feature index `offset`, each one pixel packed
// as 0xRRGGBB in a float, which is what Edge Impulse image models take as input.
inline void image_to_features(const RgbImage &img, int out_size,
                              size_t offset, size_t length, float *out)
{
    const int side = img.width < img.height ? img.width : img.height;
    const int x0 = (img.width - side) / 2;
    const int y0 = (img.height - side) / 2;

    for (size_t i = 0; i < length; i++) {
        const int idx = (int)(offset + i);
        const int ox = idx % out_size;
        const int oy = idx / out_size;
        // Sample the centre of each output cell.
        const int sx = x0 + ((2 * ox + 1) * side) / (2 * out_size);
        const int sy = y0 + ((2 * oy + 1) * side) / (2 * out_size);
        const uint8_t *p = img.pixels + 3 * (sy * img.width + sx);
        out[i] = (float)((p[0] << 16) | (p[1] << 8) | p[2]);
    }
}
