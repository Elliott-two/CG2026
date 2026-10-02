#include "Renderer.h"
#include "RedNoiseRenderer.h"
#include "BlueNoiseRenderer.h"
#include "GreenNoiseRenderer.h"
#include "WhiteNoiseRenderer.h"
#include "ColourSpectrumRenderer.h"
#include "TriangleSpectrumRenderer.h"
#include "RasterisedRenderer.h"
#include <fstream>
#include <vector>

// Define globals for WIDTH and HEIGHT of the window (can be accessed from any renderer)
extern const int WIDTH = 1000;
extern const int HEIGHT = 1000;

TextureMap textureMap(
	"../../../03 Triangles and Textures/texture.ppm");

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT);

// Create instances of each renderer
RedNoiseRenderer redNoise = RedNoiseRenderer();
GreenNoiseRenderer greenNoise = GreenNoiseRenderer();
BlueNoiseRenderer blueNoise = BlueNoiseRenderer();
WhiteNoiseRenderer whiteNoise = WhiteNoiseRenderer();
ColourSpectrumRenderer colourSpectrum = ColourSpectrumRenderer();
TriangleSpectrumRenderer triangleSpectrum = TriangleSpectrumRenderer();
RasterisedRenderer rasterisedRenderer = RasterisedRenderer();

Renderer *currentRenderer = &rasterisedRenderer; // Start with rasterised renderer as default

// Unnecessary for now
bool savingFrames = false;

int frameCounter = 0;

// Core 3D rendering data structures (for when we eventually get around to working in 3D ;o)
std::vector<ModelTriangle> triangles;
std::vector<Colour> palette;
glm::vec3 cameraPosition(0.0, 0.0, 4.0);
glm::mat3 cameraOrientation;
glm::vec3 lightPosition(0.0, 0.8, 0.0);

void handleEvent(SDL_Event event, DrawingWindow &window, Renderer *renderer = currentRenderer)
{
	if (event.type == SDL_KEYDOWN)
	{
		if (event.key.keysym.sym == SDLK_u)
		{
			std::cout << "Drawing Stroked Triangles" << std::endl;
			rasterisedRenderer.drawStrokedTriangles(window);
		}
		else if (event.key.keysym.sym == SDLK_f)
		{
			std::cout << "Drawing Filled Triangles" << std::endl;
			rasterisedRenderer.filledTriangle(window);
		}
		else if (event.key.keysym.sym == SDLK_BACKSPACE)
		{
			std::cout << "Clearing Pixels" << std::endl;
			rasterisedRenderer.clearPixels(window);
		}
		else if (event.key.keysym.sym == SDLK_t)
		{
			std::cout << "Drawing Textured Triangles" << std::endl;
			rasterisedRenderer.texturedTriangle(window, textureMap);
		}

		// Switch between renderers based on key presses
		else if (event.key.keysym.sym == SDLK_p)
		{
			std::cout << "Turning Rasterised Renderer" << std::endl;
			currentRenderer = &rasterisedRenderer;
			rasterisedRenderer.clearPixels(window); // Clear the window when switching to rasterised renderer
		}
		else if (event.key.keysym.sym == SDLK_c)
		{
			std::cout << "Turning Colour Spectrum" << std::endl;
			currentRenderer = &colourSpectrum;
		}

		else if (event.key.keysym.sym == SDLK_t)
		{
			std::cout << "Turning Triangle Spectrum" << std::endl;
			currentRenderer = &triangleSpectrum;
		}

		else if (event.key.keysym.sym == SDLK_r)
		{
			std::cout << "Turning Red" << std::endl;
			currentRenderer = &redNoise;
		}

		else if (event.key.keysym.sym == SDLK_g)
		{
			std::cout << "Turning Green" << std::endl;
			currentRenderer = &greenNoise;
		}

		else if (event.key.keysym.sym == SDLK_b)
		{
			std::cout << "Turning Blue" << std::endl;
			currentRenderer = &blueNoise;
		}

		else if (event.key.keysym.sym == SDLK_w)
		{
			std::cout << "Turning White" << std::endl;
			currentRenderer = &whiteNoise;
		}

		// Adjust the RGB values based on key presses
		else if (event.key.keysym.sym == SDLK_1)
		{
			currentRenderer->adjustRed(+5);
			std::cout
				<< "Increase Red" << std::endl;
		}

		else if (event.key.keysym.sym == SDLK_2)
		{
			currentRenderer->adjustRed(-5);
			std::cout
				<< "Decrease Red" << std::endl;
		}

		else if (event.key.keysym.sym == SDLK_3)
		{
			currentRenderer->adjustBlue(+5);
			std::cout
				<< "Increase Blue" << std::endl;
		}

		else if (event.key.keysym.sym == SDLK_4)
		{
			currentRenderer->adjustBlue(-5);
			std::cout
				<< "Decrease Blue" << std::endl;
		}

		else if (event.key.keysym.sym == SDLK_5)
		{
			currentRenderer->adjustGreen(+5);
			std::cout
				<< "Increase Green" << std::endl;
		}

		else if (event.key.keysym.sym == SDLK_6)
		{
			currentRenderer->adjustGreen(-5);
			std::cout
				<< "Decrease Green" << std::endl;
		}
	}
}

int main(int argc, char *argv[])
{

	SDL_Event event;
	while (true)
	{
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event))
			handleEvent(event, window);
		// In C++ we can't use "." notation for dynamic dispatch, so we have to use -> instead :o(
		currentRenderer->draw(window);
		// Need to ask the SDL window to render the frame, otherwise nothing actually gets shown on the screen !
		window.renderFrame();
		if (savingFrames)
		{
			std::string paddedCounter = std::to_string(frameCounter);
			while (paddedCounter.length() < 6)
				paddedCounter = "0" + paddedCounter;
			// Use whichever image file format works best on your platform (comment out the one you don't need)
			window.savePPM("saved-frames", paddedCounter + ".ppm");
			window.saveBMP("saved-frames", paddedCounter + ".bmp");
			frameCounter++;
		}
	}
}
