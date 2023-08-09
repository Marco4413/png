#include "png/kernel.h"

#include <cmath>

PNG::Kernel::Kernel(size_t width, size_t height, std::initializer_list<double> init)
    : Width(width), Height(height), AnchorX(width/2), AnchorY(height/2), Data(new double[Width * Height]{0.0})
{
    if (init.size() == 0)
        return;

    size_t i = 0;
    const size_t size = Width * Height;
    for (double value : init) {
        if (i >= size)
            break;
        Data[i++] = value;
    }

    const double lastValue = *(init.end()-1);
    for (; i < size; i++)
        Data[i] = lastValue;
}
