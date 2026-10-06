#pragma once

#include "Renderer.h"

class RasterisedRenderer : public Renderer
{
public:
    // Window tools
    void draw(DrawingWindow &window) override;
    void clearPixels(DrawingWindow &window);

    // Drawing tools
    void simpleLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow &window);
    CanvasTriangle randomTrianglePointsSorted(DrawingWindow &window);
    float calculateXIntersection(CanvasPoint bottom, CanvasPoint top, float middlePointY);
    CanvasPoint interpolateCanvasPoint(CanvasPoint from, CanvasPoint to, float y);

    // Shapes

    // First Shape
    void drawFirstShape(DrawingWindow &window);

    // Stroked Triangles
    void strokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void drawStrokedTriangles(DrawingWindow &window);

    // Filled Triangles
    void fillFlatBottomTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void fillFlatTopTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void filledTriangle(DrawingWindow &window);

    // Textured Triangles
    void textureFlatBottomTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window);
    void textureFlatTopTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window);
    void texturedTriangle(DrawingWindow &window, TextureMap &textureMap);
};
