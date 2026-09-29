#include "BlueNoiseRenderer.h"

void BlueNoiseRenderer::draw(DrawingWindow &window)
{

    window.clearPixels();
    for (size_t y = 0; y < window.height; y++)
    {
        for (size_t x = 0; x < window.width; x++)
        {
            float red = 0.0;
            float green = 0.0;
            float blue = rand() % (blueMax + 1);
            uint32_t colour = (ALPHA << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}
