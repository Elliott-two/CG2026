#pragma once

#include "Renderer.h"

class GreenNoiseRenderer : public Renderer
{
public:
    void draw(DrawingWindow &window) override;
};
