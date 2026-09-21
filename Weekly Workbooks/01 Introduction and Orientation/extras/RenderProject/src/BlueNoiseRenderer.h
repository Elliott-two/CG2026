#pragma once

#include "Renderer.h"

class BlueNoiseRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
};
