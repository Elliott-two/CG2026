#pragma once

#include "Renderer.h"
#include "../libs/sdw/Utils.h"

class TriangleSpectrumRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
};
