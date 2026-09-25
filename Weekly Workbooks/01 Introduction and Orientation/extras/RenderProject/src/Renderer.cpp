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

std::vector<float> Renderer::interpolateSingleFloats(float from, float to, int numberOfValues)
{
    std::vector<float> values;
    float step = (to - from) / (numberOfValues - 1);
    for (int i = 0; i < numberOfValues; i++)
    {
        values.push_back(from + step * i);
    }
    return values;
}

std::vector<glm::vec3> Renderer::interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues)
{
    std::vector<glm::vec3> values;
    glm::vec3 step = (to - from) / float(numberOfValues - 1);
    for (int i = 0; i < numberOfValues; i++)
    {
        values.push_back(from + step * float(i));
    }
    return values;
}
