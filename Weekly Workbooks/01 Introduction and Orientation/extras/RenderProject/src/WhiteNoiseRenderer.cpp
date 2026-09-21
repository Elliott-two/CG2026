#include "WhiteNoiseRenderer.h"

void WhiteNoiseRenderer::draw(DrawingWindow &window)
{
    window.clearPixels();
    for (size_t y = 0; y < window.height; y++)
    {
        for (size_t x = 0; x < window.width; x++)
        {
            float red = rand() % (redMax + 1);
            float green = rand() % (greenMax + 1);
            float blue = rand() % (blueMax + 1);
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}
