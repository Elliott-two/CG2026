#pragma once

#include "Renderer.h"

class WhiteNoiseRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
};
