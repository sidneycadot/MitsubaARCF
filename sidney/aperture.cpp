// Standard C++ includes
#include <cmath>
#include <iostream>
#include <limits>


template <typename T>
inline T stretch(const T x,
                 const T sizeTotal,
                 const T sizeBar)
{
    const T sizeDiff = sizeTotal - sizeBar;

    // Handle degenerate case
    if (sizeDiff == 0)
    {
        return std::abs(x) < sizeBar / 2 ? 0 : 2 * x / sizeBar;
    }

    return (2 * x + std::abs(x - sizeBar / 2) - std::abs(x + sizeBar / 2)) / sizeDiff;
}


template <typename T>
class Aperture
{
public:

    Aperture(const T xSizeTotal,
             const T xSizeBar,
             const T ySizeTotal,
             const T ySizeBar) :
        _xSizeTotal(xSizeTotal),
        _xSizeBar  (xSizeBar),
        _ySizeTotal(ySizeTotal),
        _ySizeBar  (ySizeBar)
    {
    }

    bool inside(const T x,
                const T y) const
    {
        const T xx = stretch(x, _xSizeTotal, _xSizeBar);
        const T yy = stretch(y, _ySizeTotal, _ySizeBar);

        return xx * xx + yy * yy < 1;
    }

private:

    const T _xSizeTotal;
    const T _xSizeBar;

    const T _ySizeTotal;
    const T _ySizeBar;
};


int main()
{
    using namespace std;

    Aperture<double> aperture(16, 0,
                              32, 16);

    for (double y = -20; y <= +20; ++y)
    {
        for (double x = -20; x <= +20; ++x)
        {
            cout << (aperture.inside(x, y) ? '*' : '.');
        }

        cout << endl;
    }

    return EXIT_SUCCESS;
}
