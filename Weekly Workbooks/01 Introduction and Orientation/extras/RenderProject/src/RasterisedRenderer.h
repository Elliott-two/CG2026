#pragma once

#include "Renderer.h"

class RasterisedRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
    void clearPixels(DrawingWindow &window);
    void filledTriangle(DrawingWindow &window);
    CanvasTriangle randomTrianglePointsSorted(DrawingWindow &window);
    void fillFlatBottomTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void fillFlatTopTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void texturedTriangle(DrawingWindow &window, TextureMap &textureMap);
    float calculateXIntersection(CanvasPoint bottom, CanvasPoint top, float middlePointY);
    void simpleLine(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow &window);
    void strokedTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window);
    void drawFirstShape(DrawingWindow &window);
    void drawStrokedTriangles(DrawingWindow &window);
    CanvasPoint interpolateCanvasPoint(CanvasPoint from, CanvasPoint to, float y);
    void textureFlatBottomTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window);
    void textureFlatTopTriangle(CanvasTriangle triangle, TextureMap &textureMap, DrawingWindow &window);
};
