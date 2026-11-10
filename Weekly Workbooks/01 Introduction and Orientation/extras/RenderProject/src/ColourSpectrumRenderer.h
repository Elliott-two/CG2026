#pragma once

#include "Renderer.h"
#include "Utils.h"

class ColourSpectrumRenderer : public Renderer
{
public:
   void draw(DrawingWindow &window) override;
};
