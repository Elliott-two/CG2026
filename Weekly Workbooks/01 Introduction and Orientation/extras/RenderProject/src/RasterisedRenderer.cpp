#include "RasterisedRenderer.h"

void RasterisedRenderer::draw(DrawingWindow &window) {}

void RasterisedRenderer::clearPixels(DrawingWindow &window)
{
    // Clear the window to black before drawing
    window.clearPixels();
}

// Drawing a straight line between two points
void RasterisedRenderer::simpleLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow &window)
{
    // Calculate the differences in x and y coordinates
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;

    // Number of steps is the maximum of the absolute differences in x and y coordinates for smooth line drawing
    float steps = std::max(std::abs(xDiff), std::abs(yDiff));

    // Calculate the step size for x and y coordinates based on the number of steps
    float xStep = xDiff / steps;
    float yStep = yDiff / steps;

    // Draw the line by iterating through the steps and setting pixel colors
    for (float stepIndex = 0.0; stepIndex <= steps; stepIndex++)
    {
        float currentX = from.x + stepIndex * xStep;
        float currentY = from.y + stepIndex * yStep;
        window.setPixelColour(currentX, currentY, (colour.red << 16) | (colour.green << 8) | colour.blue);
    }
}

// Generate a random triangle with sorted vertices based on their y-coordinates
CanvasTriangle RasterisedRenderer::randomTrianglePointsSorted(DrawingWindow &window)
{
    // Screen sizes for given window
    int width = window.width - 1;
    int height = window.height - 1;

    // Three random points with random x and y coordinates within the window dimensions
    CanvasPoint rand1(rand() % width, rand() % height);
    CanvasPoint rand2(rand() % width, rand() % height);
    CanvasPoint rand3(rand() % width, rand() % height);

    // Small bubble sort to sort the points based on their y-coordinates in descending order
    if (rand1.y > rand2.y)
        std::swap(rand1, rand2);
    if (rand2.y > rand3.y)
        std::swap(rand2, rand3);
    if (rand1.y > rand2.y)
        std::swap(rand1, rand2);
    return CanvasTriangle(rand1, rand2, rand3);
}

CanvasTriangle RasterisedRenderer::trianglePointsSorted(DrawingWindow &window, CanvasTriangle triangle)
{
    if (triangle.v0().y > triangle.v1().y)
        std::swap(triangle.v0(), triangle.v1());
    if (triangle.v1().y > triangle.v2().y)
        std::swap(triangle.v1(), triangle.v2());
    if (triangle.v0().y > triangle.v1().y)
        std::swap(triangle.v0(), triangle.v1());
    return triangle;
}

// Calculate the x-coordinate of the intersection point of a line segment defined by two points (bottom and top) with a horizontal line at a given y-coordinate (middlePointY)
float RasterisedRenderer::calculateXIntersection(CanvasPoint bottom, CanvasPoint top, float middlePointY)
{
    // Check for horizontal case where the y-coordinates of the top and bottom points are very close to each other (within a small threshold)
    if (std::abs(top.y - bottom.y) < 0.1f)
    {
        return bottom.x;
    }

    // Calculate the inverse slope of the line segment defined by the bottom and top points
    float invSlope = (top.x - bottom.x) / (top.y - bottom.y);

    // return the x-coordinate of the intersection point using the formula: x = x0 + (y - y0) * invSlope, where (x0, y0) is the bottom point and y is the middlePointY
    return bottom.x + (middlePointY - bottom.y) * invSlope;
}

// Interpolate between two CanvasPoints based on a given y-coordinate (y) to find the corresponding x-coordinate and texture coordinates
CanvasPoint RasterisedRenderer::interpolateCanvasPoint(CanvasPoint from, CanvasPoint to, float y)
{

    // Calculate the difference in y-coordinates between the two points
    float yDifference = to.y - from.y;

    // Calculate the ratio of the distance from the 'from' point to the given y-coordinate relative to the total y-difference between the two points
    float ratio = (std::abs(yDifference) < 0.0001f) ? 0.0f : (y - from.y) / yDifference;

    // Create a new CanvasPoint to store the interpolated values
    CanvasPoint point;

    // X value is from the "from" point plus the ratio of the difference in x-coordinates between the two points
    point.x = from.x + ratio * (to.x - from.x);

    // Y value is the given y-coordinate
    point.y = y;

    // Interpolate the texture coordinates
    point.texturePoint.x = from.texturePoint.x + ratio * (to.texturePoint.x - from.texturePoint.x);

    point.texturePoint.y = from.texturePoint.y + ratio * (to.texturePoint.y - from.texturePoint.y);

    point.depth = from.depth + ratio * (to.depth - from.depth);
    return point;
}

void RasterisedRenderer::resetDepthBuffer()
{
    depthBuffer.assign(WIDTH * HEIGHT, 0.0f); // 0 = infinitely far away
}

void RasterisedRenderer::fillScanline(CanvasPoint left, CanvasPoint right, int y, Colour colour, DrawingWindow &window)
{
    if (y < 0 || y >= HEIGHT)
        return;
    if (left.x > right.x)
        std::swap(left, right);

    uint32_t packed = (255u << 24) | (colour.red << 16) | (colour.green << 8) | colour.blue;
    int xStart = static_cast<int>(std::round(left.x));
    int xEnd = static_cast<int>(std::round(right.x));

    for (int x = xStart; x <= xEnd; x++)
    {
        if (x < 0 || x >= WIDTH)
            continue;

        float t = (xEnd == xStart) ? 0.0f : static_cast<float>(x - xStart) / (xEnd - xStart);
        float depth = left.depth + t * (right.depth - left.depth);

        int i = y * WIDTH + x;
        if (depth > depthBuffer[i]) // closer than what's already there
        {
            depthBuffer[i] = depth;
            window.setPixelColour(x, y, packed);
        }
    }
}

void RasterisedRenderer::drawFirstShape(DrawingWindow &window)
{
    // Clear the window to black before drawing
    window.clearPixels();

    // Screen sizes for given window
    int width = window.width - 1;
    int height = window.height - 1;

    // Define the points for the shape
    CanvasPoint topLeft(0, 0);
    CanvasPoint bottomRight(width, height);
    CanvasPoint topRight(width, 0);
    CanvasPoint bottomLeft(0, height);

    // Center is calculated as the midpoint of the window dimensions
    CanvasPoint center(width / 2, height / 2);

    Colour white(255, 255, 255);

    // Draw line from top left to center
    simpleLine(topLeft, center, white, window);

    // Draw line from top right to center
    simpleLine(topRight, center, white, window);

    // Draw line from the top middle to the bottom middle
    simpleLine(CanvasPoint(width / 2, 0), CanvasPoint(width / 2, height), white, window);

    // Draw a line of 2/5 of the width of the window from the left middle to the right middle through the center
    // This line is a guess-timate of the image given
    simpleLine(CanvasPoint(3 * width / 10, height / 2), CanvasPoint(7 * width / 10, height / 2), white, window);
}

// Function that actually draws the stroked triangle by connecting the three vertices of the triangle with lines
void RasterisedRenderer::strokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    // Draw lines between the three vertices of the triangle
    simpleLine(triangle.v0(), triangle.v1(), colour, window);
    simpleLine(triangle.v1(), triangle.v2(), colour, window);
    simpleLine(triangle.v2(), triangle.v0(), colour, window);
}

// Function that sends the values to the drawing function
void RasterisedRenderer::drawStrokedTriangles(DrawingWindow &window)
{
    // Random triangle points are generated and sorted based on their y-coordinates
    CanvasTriangle triangle = randomTrianglePointsSorted(window);

    // Random colour is generated for the triangle
    Colour colour(rand() % 256, rand() % 256, rand() % 256);

    // Gives data to the function that draws the triangle
    strokedTriangle(triangle, colour, window);
}

void RasterisedRenderer::fillFlatBottomTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v1().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        CanvasPoint left = interpolateCanvasPoint(triangle.v0(), triangle.v1(), y);
        CanvasPoint right = interpolateCanvasPoint(triangle.v0(), triangle.v2(), y);
        fillScanline(left, right, y, colour, window);
    }
}

void RasterisedRenderer::fillFlatTopTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window)
{
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v2().y));

    for (int y = yStart; y <= yEnd; y++)
    {
        CanvasPoint left = interpolateCanvasPoint(triangle.v0(), triangle.v2(), y);
        CanvasPoint right = interpolateCanvasPoint(triangle.v1(), triangle.v2(), y);
        fillScanline(left, right, y, colour, window);
    }
}

// Function that fills a triangle by splitting it into two flat-bottom and flat-top triangles and filling them separately
void RasterisedRenderer::filledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour)
{
    // Generate a random triangle with sorted vertices based on their y-coordinates
    CanvasTriangle sortedTriangle = trianglePointsSorted(window, triangle);

    // Assign the vertices of the sorted triangle to top, middle, and bottom points for easier reference
    CanvasPoint top = sortedTriangle.v0();
    CanvasPoint middle = sortedTriangle.v1();
    CanvasPoint bottom = sortedTriangle.v2();

    // Calculate the x-coordinate of the intersection point of the line segment defined by the bottom and top points with a horizontal line at the y-coordinate of the middle point
    CanvasPoint joiner = interpolateCanvasPoint(top, bottom, middle.y);

    // Create two new triangles: one with a flat bottom and one with a flat top, using the joiner point as the shared vertex
    CanvasTriangle flatBottomTriangle(top, middle, joiner);

    // Fill the flat-bottom triangle with the generated colour
    fillFlatBottomTriangle(flatBottomTriangle, colour, window);

    // Create the flat-top triangle using the joiner point, middle point, and bottom point
    CanvasTriangle flatTopTriangle(joiner, middle, bottom);

    // Fill the flat-top triangle with the generated colour
    fillFlatTopTriangle(flatTopTriangle, colour, window);
}

// Function that draws a textured triangle by splitting it into two flat-bottom and flat-top triangles and filling them separately with texture mapping
void RasterisedRenderer::texturedTriangle(DrawingWindow &window, TextureMap &textureMap)
{

    // Define the vertices of the triangle with their corresponding texture coordinates
    CanvasPoint top(160, 10);
    CanvasPoint middle(10, 150);
    CanvasPoint bottom(300, 230);

    // Assign texture coordinates to the vertices of the triangle
    top.texturePoint = TexturePoint(195, 5);
    middle.texturePoint = TexturePoint(65, 330);
    bottom.texturePoint = TexturePoint(395, 380);

    // Create a CanvasTriangle using the defined vertices
    CanvasTriangle sortedTriangle(top, middle, bottom);

    // Calculate the x-coordinate of the intersection point of the line segment defined by the bottom and top points with a horizontal line at the y-coordinate of the middle point
    float joinerX = calculateXIntersection(bottom, top, middle.y);

    // Calculate the texture coordinates of the intersection point using linear interpolation based on the ratio of the distances along the y-axis
    float ratio = (middle.y - bottom.y) / (top.y - bottom.y);
    float joinerTextureX = bottom.texturePoint.x + ratio * (top.texturePoint.x - bottom.texturePoint.x);
    float joinerTextureY = bottom.texturePoint.y + ratio * (top.texturePoint.y - bottom.texturePoint.y);

    // Create a new CanvasPoint for the intersection point with the calculated x-coordinate and texture coordinates
    CanvasPoint joiner(joinerX, middle.y);

    // Assign the calculated texture coordinates to the joiner point
    joiner.texturePoint = TexturePoint(joinerTextureX, joinerTextureY);

    // Fill the flat-bottom triangle with texture mapping using the textureFlatBottomTriangle function
    textureFlatBottomTriangle(CanvasTriangle(top, middle, joiner), textureMap, window);
    textureFlatTopTriangle(CanvasTriangle(joiner, middle, bottom), textureMap, window);

    // Draw the outline of the triangle using the strokedTriangle function
    strokedTriangle(sortedTriangle, Colour(255, 255, 255), window);
}

// Fill a flat-bottom triangle with texture mapping by iterating through the y-coordinates and calculating the x-intersections of the triangle edges and corresponding texture coordinates
void RasterisedRenderer::textureFlatBottomTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window)
{
    // Calculate the starting and ending y-coordinates for the triangle
    // Using static_cast<int> and std::round to ensure proper rounding of the y-coordinates
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v1().y));

    // Iterate through the y-coordinates from yStart to yEnd
    for (int y = yStart; y <= yEnd; y++)
    {
        // Calculate the current y-coordinate as a float for interpolation
        // Using static_cast<float> to ensure proper type conversion
        float currentY = static_cast<float>(y);

        // Interpolate between the sets of vertecies
        CanvasPoint left = interpolateCanvasPoint(triangle.v0(), triangle.v1(), currentY);

        CanvasPoint right = interpolateCanvasPoint(triangle.v0(), triangle.v2(), currentY);

        // Swap left and right if they are wrong
        if (left.x > right.x)
        {
            std::swap(left, right);
        }

        // Calculate start and end X
        // Using static_cast and std::round for proper rounding
        int xStart = static_cast<int>(std::round(left.x));
        int xEnd = static_cast<int>(std::round(right.x));

        // looping over x
        for (int x = xStart; x <= xEnd; x++)
        {
            // Initialise Ratio
            float ratio = 0.0f;

            // Check for end = start case
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
    // The only change from the previous function is that the triangle is now flat on the top, so we take different vertices to calculate the x-intersections of the triangle edges with the current y-coordinate
    int yStart = static_cast<int>(std::round(triangle.v0().y));
    int yEnd = static_cast<int>(std::round(triangle.v2().y));

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
