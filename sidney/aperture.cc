#include <iostream>
#include <cmath>

template <typename fp_t>
inline fp_t stretch(const fp_t x, const fp_t totalWidth, const fp_t barWidth)
{
    return (2 * x + std::abs(x - barWidth / 2) - std::abs(x + barWidth / 2)) / (totalWidth - barWidth);
}

template <typename fp_t>
bool inside_aperture(const fp_t totalWidth, const fp_t totalHeight, const fp_t barWidth, const fp_t barHeight, const fp_t x, const fp_t y)
{
    const fp_t xx = stretch(x, totalWidth, barWidth);
    const fp_t yy = stretch(y, totalHeight, barHeight);

    return xx * xx + yy * yy <= 1;
}

int main()
{
    for (int y = -20; y <= +20; ++y)
    {
        for (int x = -20; x <= +20; ++x)
        {
            const bool b = inside_aperture<double>(8, 16, 0, 8, x / 2.0, y / 2.0);
            std::cout << (b ? '*' : '.');
        }

        std::cout << std::endl;
    }
    return 0;
}
