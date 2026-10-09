#include "Renderer.h"
#include "RedNoiseRenderer.h"
#include "BlueNoiseRenderer.h"
#include "GreenNoiseRenderer.h"
#include "WhiteNoiseRenderer.h"
#include "ColourSpectrumRenderer.h"
#include "TriangleSpectrumRenderer.h"
#include "RasterisedRenderer.h"
#include "PointCloudRenderer.h"
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

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
PointCloudRenderer pointCloud = PointCloudRenderer();

Renderer *currentRenderer = &rasterisedRenderer; // Start with rasterised renderer as default

// Unnecessary for now
bool savingFrames = false;

int frameCounter = 0;

// Core 3D rendering data structures (for when we eventually get around to working in 3D ;o)
std::string objFilename = "../../../04 Wireframes and Rasterising/models/cornell-box";
std::vector<ModelTriangle> triangles;
std::vector<Colour> palette;
std::unordered_map<std::string, Colour> paletteMap;
float focalLength = 2.0;
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

		else if (event.key.keysym.sym == SDLK_m)
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
		else if (event.key.keysym.sym == SDLK_k)
		{
			std::cout << "Turning Point Cloud" << std::endl;
			currentRenderer = &pointCloud;
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
std::vector<ModelTriangle> triangleParseOBJ(const std::string &filename, float scalingFactor)
{
	// Vector of Triangles
	std::vector<ModelTriangle> modelTriangles;
	// Vector of Vertices
	std::vector<glm::vec3> vertices;

	// ifstream
	std::ifstream file(filename);
	// Check working
	if (!file.is_open())
		return modelTriangles;

	// string = to line
	std::string line;
	Colour colour(255, 255, 255);
	// While getting lines
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;

		std::vector<std::string> tokens = split(line, ' ');
		if (tokens.empty() || tokens[0].empty())
			continue;

		const std::string &prefix = tokens[0];

		if (prefix == "v" && tokens.size() >= 4)
		{
			vertices.push_back(glm::vec3(
				std::stof(tokens[1]) * scalingFactor,
				std::stof(tokens[2]) * scalingFactor,
				std::stof(tokens[3]) * scalingFactor));
		}
		else if (prefix == "f" && tokens.size() >= 4)
		{
			int idx[3];
			for (int i = 0; i < 3; i++)
				idx[i] = std::stoi(split(tokens[i + 1], '/')[0]) - 1;

			modelTriangles.push_back(ModelTriangle(
				vertices[idx[0]], vertices[idx[1]], vertices[idx[2]],
				colour));
		}
		else if (prefix == "usemtl" && tokens.size() >= 2)
		{
			auto it = paletteMap.find(tokens[1]);
			if (it != paletteMap.end())
			{
				colour = it->second;
			}
		}
	}

	return modelTriangles;
}

std::vector<Colour> colourParseOBJ(const std::string &filename)
{
	std::vector<Colour> colours;

	std::ifstream file(filename);
	// Check working
	if (!file.is_open())
		return colours;

	std::string line;
	std::string colourName;
	// While getting lines
	while (std::getline(file, line))
	{

		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			continue;

		std::vector<std::string> tokens = split(line, ' ');
		if (tokens.empty() || tokens[0].empty())
			continue;

		const std::string &prefix = tokens[0];

		if (prefix == "newmtl" && tokens.size() >= 2)
		{
			colourName = tokens[1];
		}
		else if (prefix == "Kd" && tokens.size() >= 4)
		{
			colours.push_back(Colour(
				colourName,
				static_cast<int>(std::round(255 * std::stof(tokens[1]))),
				static_cast<int>(std::round(255 * std::stof(tokens[2]))),
				static_cast<int>(std::round(255 * std::stof(tokens[3])))));
		}
	}
	return colours;
}

int main(int argc, char *argv[])
{
	palette = colourParseOBJ(objFilename + ".mtl");
	for (int i = 0; i < palette.size(); i++)
	{
		paletteMap[palette[i].name] = palette[i];
	}

	triangles = triangleParseOBJ(objFilename + ".obj", 0.35f);

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
