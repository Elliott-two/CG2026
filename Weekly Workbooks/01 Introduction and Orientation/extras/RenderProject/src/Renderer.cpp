#include "Renderer.h"

void Renderer::draw(DrawingWindow &window)
{
}
void Renderer::adjustRed(int delta)
{
    redMax += delta;
    if (redMax < 0)
        redMax = 0;
    if (redMax > 255)
        redMax = 255;
}
void Renderer::adjustGreen(int delta)
{
    greenMax += delta;
    if (greenMax < 0)
        greenMax = 0;
    if (greenMax > 255)
        greenMax = 255;
}
void Renderer::adjustBlue(int delta)
{
    blueMax += delta;
    if (blueMax < 0)
        blueMax = 0;
    if (blueMax > 255)
        blueMax = 255;
}
