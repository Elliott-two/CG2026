#pragma once

#include "Renderer.h"
#include "../libs/sdw/Utils.h"

class ColourSpectrumRenderer : public Renderer
{
public:
   void draw(DrawingWindow &window) override;
};
