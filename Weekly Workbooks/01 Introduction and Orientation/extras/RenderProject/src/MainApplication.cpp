#include "Renderer.h"
#include "RedNoiseRenderer.h"
#include "BlueNoiseRenderer.h"
#include "GreenNoiseRenderer.h"
#include "ColourSpectrumRenderer.h"
#include <fstream>
#include <vector>

// Define globals for WIDTH and HEIGHT of the window (can be accessed from any renderer)
extern const int WIDTH = 1080;
extern const int HEIGHT = 720;

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT);
RedNoiseRenderer redNoise = RedNoiseRenderer();
GreenNoiseRenderer greenNoise = GreenNoiseRenderer();
BlueNoiseRenderer blueNoise = BlueNoiseRenderer();
Renderer *currentRenderer = &greenNoise;
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
		if (event.key.keysym.sym == SDLK_LEFT)
			std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT)
			std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP)
			std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN)
			std::cout << "DOWN" << std::endl;
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

		else if (event.key.keysym.sym == SDLK_c)
			currentRenderer = new ColourSpectrumRenderer();
		else if (event.key.keysym.sym == SDLK_r)
			std::cout
				<< "R" << std::endl;
		else if (event.key.keysym.sym == SDLK_g)
			std::cout
				<< "G" << std::endl;
		else if (event.key.keysym.sym == SDLK_b)
			std::cout
				<< "B" << std::endl;
		else if (event.key.keysym.sym == SDLK_u)
			std::cout
				<< "U" << std::endl;
		else if (event.key.keysym.sym == SDLK_f)
			std::cout
				<< "F" << std::endl;
		else if (event.key.keysym.sym == SDLK_TAB)
			savingFrames = !savingFrames;
		else if (event.key.keysym.sym == SDLK_RETURN)
			window.toggleFullscreen();
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
