#include "RasterisedRenderer.h"

void RasterisedRenderer::draw(DrawingWindow &window) {}

CanvasTriangle RasterisedRenderer::randomTrianglePointsSorted(DrawingWindow &window)
{
    int width = window.width - 1;
    int height = window.height - 1;

    CanvasPoint rand1(rand() % width, rand() % height);
    CanvasPoint rand2(rand() % width, rand() % height);
    CanvasPoint rand3(rand() % width, rand() % height);

    if (rand1.y > rand2.y)
        std::swap(rand1, rand2);
    if (rand2.y > rand3.y)
        std::swap(rand2, rand3);
    if (rand1.y > rand2.y)
        std::swap(rand1, rand2);
    return CanvasTriangle(rand1, rand2, rand3);
}

void RasterisedRenderer::clearPixels(DrawingWindow &window)
{
    window.clearPixels();
}

void RasterisedRenderer::drawStrokedTriangles(DrawingWindow &window)
{

    CanvasTriangle triangle = randomTrianglePointsSorted(window);

    Colour colour(rand() % 256, rand() % 256, rand() % 256);

    strokedTriangle(triangle, colour, window);
}

void RasterisedRenderer::filledTriangle(DrawingWindow &window)
{
    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    CanvasTriangle sortedTriangle = randomTrianglePointsSorted(window);
    CanvasPoint top = sortedTriangle.v0();
    CanvasPoint middle = sortedTriangle.v1();
    CanvasPoint bottom = sortedTriangle.v2();

    CanvasPoint joiner(static_cast<float>(calculateXIntersection(bottom, top, middle.y)), middle.y);

    CanvasTriangle flatBottomTriangle(top, middle, joiner);

    fillFlatBottomTriangle(flatBottomTriangle, colour, window);

    CanvasTriangle flatTopTriangle(joiner, middle, bottom);
    fillFlatTopTriangle(flatTopTriangle, colour, window);
    strokedTriangle(sortedTriangle, Colour(255, 255, 255), window);
}

void RasterisedRenderer::texturedTriangle(DrawingWindow &window, TextureMap &textureMap)
{

    CanvasPoint top(160, 10);
    CanvasPoint middle(10, 150);
    CanvasPoint bottom(300, 230);

    top.texturePoint = TexturePoint(195, 5);
    middle.texturePoint = TexturePoint(65, 330);
    bottom.texturePoint = TexturePoint(395, 380);

    CanvasTriangle sortedTriangle(top, middle, bottom);

    float joinerX = calculateXIntersection(bottom, top, middle.y);
    float ratio = (middle.y - bottom.y) / (top.y - bottom.y);
    float joinerTextureX = bottom.texturePoint.x + ratio * (top.texturePoint.x - bottom.texturePoint.x);
    float joinerTextureY = bottom.texturePoint.y + ratio * (top.texturePoint.y - bottom.texturePoint.y);

    CanvasPoint joiner(joinerX, middle.y);
    joiner.texturePoint = TexturePoint(joinerTextureX, joinerTextureY);

    textureFlatBottomTriangle(CanvasTriangle(top, middle, joiner), textureMap, window);
    textureFlatTopTriangle(CanvasTriangle(joiner, middle, bottom), textureMap, window);

    strokedTriangle(sortedTriangle, Colour(255, 255, 255), window);
}

void RasterisedRenderer::fillFlatBottomTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v1().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        float currentY = static_cast<float>(y);
        int xIntersection1 = calculateXIntersection(triangle.v0(), triangle.v1(), currentY);
        int xIntersection2 = calculateXIntersection(triangle.v0(), triangle.v2(), currentY);

        int xStart = std::min(xIntersection1, xIntersection2);
        int xEnd = std::max(xIntersection1, xIntersection2);

        for (int x = xStart; x <= xEnd; x++)
        {
            window.setPixelColour(x, y, (colour.red << 16) | (colour.green << 8) | colour.blue);
        }
    }
}
void RasterisedRenderer::fillFlatTopTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v2().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        float currentY = static_cast<float>(y);
        int xIntersection1 = calculateXIntersection(triangle.v0(), triangle.v2(), currentY);
        int xIntersection2 = calculateXIntersection(triangle.v1(), triangle.v2(), currentY);

        int xStart = std::min(xIntersection1, xIntersection2);
        int xEnd = std::max(xIntersection1, xIntersection2);

        for (int x = xStart; x <= xEnd; x++)
        {
            window.setPixelColour(x, y, (colour.red << 16) | (colour.green << 8) | colour.blue);
        }
    }
}

float RasterisedRenderer::calculateXIntersection(CanvasPoint bottom, CanvasPoint top, float middlePointY)
{
    if (std::abs(top.y - bottom.y) < 0.1f)
    {
        return bottom.x;
    }

    float invSlope = (top.x - bottom.x) / (top.y - bottom.y);

    return bottom.x + (middlePointY - bottom.y) * invSlope;
}

void RasterisedRenderer::simpleLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow &window)
{
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float steps = std::max(std::abs(xDiff), std::abs(yDiff));
    float xStep = xDiff / steps;
    float yStep = yDiff / steps;
    for (float i = 0.0; i <= steps; i++)
    {
        float x = from.x + i * xStep;
        float y = from.y + i * yStep;
        window.setPixelColour(x, y, (colour.red << 16) | (colour.green << 8) | colour.blue);
    }
}

void RasterisedRenderer::strokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    simpleLine(triangle.v0(), triangle.v1(), colour, window);
    simpleLine(triangle.v1(), triangle.v2(), colour, window);
    simpleLine(triangle.v2(), triangle.v0(), colour, window);
}

void RasterisedRenderer::drawFirstShape(DrawingWindow &window)
{
    // Clear the window to black before drawing
    window.clearPixels();
    int width = window.width - 1;
    int height = window.height - 1;

    CanvasPoint topLeft(0, 0);
    CanvasPoint bottomRight(width, height);
    CanvasPoint topRight(width, 0);
    CanvasPoint bottomLeft(0, height);
    CanvasPoint center(width / 2, height / 2);

    Colour colour(255, 255, 255); // White color

    simpleLine(topLeft, center, colour, window);
    simpleLine(topRight, center, colour, window);
    simpleLine(CanvasPoint(width / 2, 0), CanvasPoint(width / 2, height), colour, window);
    simpleLine(CanvasPoint(3 * width / 10, height / 2), CanvasPoint(7 * width / 10, height / 2), colour, window);
    // Render the frame to the window
    window.renderFrame();
}

CanvasPoint RasterisedRenderer::interpolateCanvasPoint(CanvasPoint from, CanvasPoint to, float y)
{
    float dy = to.y - from.y;
    float ratio = (std::abs(dy) < 0.0001f) ? 0.0f : (y - from.y) / dy;

    CanvasPoint point;

    point.x = from.x + ratio * (to.x - from.x);
    point.y = y;

    point.texturePoint.x = from.texturePoint.x + ratio * (to.texturePoint.x - from.texturePoint.x);

    point.texturePoint.y = from.texturePoint.y + ratio * (to.texturePoint.y - from.texturePoint.y);

    return point;
}

void RasterisedRenderer::textureFlatBottomTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window)
{
    int yStart = static_cast<int>(
        std::round(triangle.v0().y));

    int yEnd = static_cast<int>(
        std::round(triangle.v1().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        float currentY = static_cast<float>(y);

        CanvasPoint left = interpolateCanvasPoint(triangle.v0(), triangle.v1(), currentY);

        CanvasPoint right = interpolateCanvasPoint(triangle.v0(), triangle.v2(), currentY);

        if (left.x > right.x)
        {
            std::swap(left, right);
        }

        int xStart = static_cast<int>(std::round(left.x));
        int xEnd = static_cast<int>(std::round(right.x));

        for (int x = xStart; x <= xEnd; x++)
        {
            float ratio = 0.0f;

            if (xEnd != xStart)
            {
                ratio = static_cast<float>(x - xStart) / static_cast<float>(xEnd - xStart);
            }

            float textureX = left.texturePoint.x + ratio * (right.texturePoint.x - left.texturePoint.x);

            float textureY = left.texturePoint.y + ratio * (right.texturePoint.y - left.texturePoint.y);

            int texX = static_cast<int>(std::round(textureX));

            int texY = static_cast<int>(std::round(textureY));

            texX = std::max(0, std::min(texX, static_cast<int>(textureMap.width) - 1));

            texY = std::max(0, std::min(texY, static_cast<int>(textureMap.height) - 1));

            int index = texY * textureMap.width + texX;

            window.setPixelColour(x, y, textureMap.pixels[index]);
        }
    }
}

void RasterisedRenderer::textureFlatTopTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window)
{
    int yStart = static_cast<int>(
        std::round(triangle.v0().y));

    int yEnd = static_cast<int>(
        std::round(triangle.v2().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        float currentY = static_cast<float>(y);

        CanvasPoint left = interpolateCanvasPoint(triangle.v0(), triangle.v2(), currentY);

        CanvasPoint right = interpolateCanvasPoint(triangle.v1(), triangle.v2(), currentY);

        if (left.x > right.x)
        {
            std::swap(left, right);
        }

        int xStart = static_cast<int>(
            std::round(left.x));

        int xEnd = static_cast<int>(
            std::round(right.x));

        for (int x = xStart; x <= xEnd; x++)
        {
            float ratio = 0.0f;

            if (xEnd != xStart)
            {
                ratio = static_cast<float>(x - xStart) / static_cast<float>(xEnd - xStart);
            }

            float textureX = left.texturePoint.x + ratio * (right.texturePoint.x - left.texturePoint.x);

            float textureY = left.texturePoint.y + ratio * (right.texturePoint.y - left.texturePoint.y);

            int texX = static_cast<int>(std::round(textureX));

            int texY = static_cast<int>(std::round(textureY));

            texX = std::max(0, std::min(texX, static_cast<int>(textureMap.width) - 1));

            texY = std::max(0, std::min(texY, static_cast<int>(textureMap.height) - 1));

            int index = texY * textureMap.width + texX;

            window.setPixelColour(x, y, textureMap.pixels[index]);
        }
    }
}
