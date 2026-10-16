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
	"../../../05 Navigation and Transformation/models/texture (1).ppm");

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
const double PI = 3.14159265358979323846;
std::string objFilename = "../../../05 Navigation and Transformation/models/textured-cornell-box";
std::vector<ModelTriangle> triangles;
std::vector<Colour> palette;
std::unordered_map<std::string, Colour> paletteMap;
float focalLength = 2.0;
glm::vec3 cameraPosition(0.0, 0.0, 4.0);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
glm::vec3 lightPosition(0.0, 0.8, 0.0);

void rotateX(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, int degrees)
{
	double degreeInRadians = degrees * (PI / 180.0);
	double cos_value = std::cos(degreeInRadians);
	double sin_value = std::sin(degreeInRadians);

	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.0, cos_value, sin_value),
		glm::vec3(0.0, -sin_value, cos_value));

	cameraOrientation = rotationMatrix * cameraOrientation;
	cameraPosition = rotationMatrix * cameraPosition;
}

void rotateY(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, int degrees)
{
	double degreeInRadians = degrees * (PI / 180.0);
	double cos_value = std::cos(degreeInRadians);
	double sin_value = std::sin(degreeInRadians);

	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(cos_value, 0.0, -sin_value),
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(sin_value, 0.0, cos_value));

	cameraOrientation = rotationMatrix * cameraOrientation;
	cameraPosition = rotationMatrix * cameraPosition;
}

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

		else if (event.key.keysym.sym == SDLK_LEFT)
		{
			std::cout << "Moving Camera left" << std::endl;
			cameraPosition[0] -= 0.2;
		}

		else if (event.key.keysym.sym == SDLK_RIGHT)
		{
			std::cout << "Moving Camera Right" << std::endl;
			cameraPosition[0] += 0.2;
		}

		else if (event.key.keysym.sym == SDLK_UP)
		{
			std::cout << "Moving Camera Up" << std::endl;
			cameraPosition[1] += 0.2;
		}

		else if (event.key.keysym.sym == SDLK_DOWN)
		{
			std::cout << "Moving Camera Down" << std::endl;
			cameraPosition[1] -= 0.2;
		}

		else if (event.key.keysym.sym == SDLK_LEFTBRACKET)
		{
			std::cout << "Moving Camera in" << std::endl;
			cameraPosition[2] -= 0.2;
		}

		else if (event.key.keysym.sym == SDLK_RIGHTBRACKET)
		{
			std::cout << "Moving Camera Out" << std::endl;
			cameraPosition[2] += 0.2;
		}

		else if (event.key.keysym.sym == SDLK_l)
		{
			std::cout << "Rotating camera by 10 degrees in the x plane" << std::endl;
			rotateX(cameraPosition, cameraOrientation, 10);
		}

		else if (event.key.keysym.sym == SDLK_SEMICOLON)
		{
			std::cout << "Rotating camera by negative 10 degrees in the x plane" << std::endl;
			rotateX(cameraPosition, cameraOrientation, -10);
		}

		else if (event.key.keysym.sym == SDLK_BACKSLASH)
		{
			std::cout << "Rotating camera by 10 degrees in the y plane" << std::endl;
			rotateY(cameraPosition, cameraOrientation, 10);
		}
		else if (event.key.keysym.sym == SDLK_QUOTE)
		{
			std::cout << "Rotating camera by negative 10 degrees in the y plane" << std::endl;
			rotateY(cameraPosition, cameraOrientation, -10);
		}

		else if (event.key.keysym.sym == SDLK_9)
		{
			std::cout << "Change Rotating X" << std::endl;
			OrbitXBool = !OrbitXBool;
		}
		else if (event.key.keysym.sym == SDLK_0)
		{
			std::cout << "Change Rotating Y" << std::endl;
			OrbitYBool = !OrbitYBool;
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

	// Default Colour if doesn't work
	Colour colour(255, 255, 255);

	// Empty if no Textures
	std::array<TexturePoint, 3> texturePoints{};

	// While getting lines
	while (std::getline(file, line))
	{
		// Check not empty and is a return therefore remove line
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		// check empty, if so go next line
		if (line.empty())
		{
			continue;
		}

		// Split into tokens
		std::vector<std::string> tokens = split(line, ' ');

		// Prefix is the first token
		const std::string &prefix = tokens[0];

		// Check vertex
		if (prefix == "v" && tokens.size() >= 4)
		{
			// Push back and scaling
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

			ModelTriangle triangle(vertices[idx[0]], vertices[idx[1]], vertices[idx[2]], colour);
			triangle.texturePoints = texturePoints;
			modelTriangles.push_back(triangle);
		}
		else if (prefix == "usemtl" && tokens.size() >= 2)
		{
			if (tokens[1] == "Cobbles")
			{
				texturePoints = {
					TexturePoint(100, 5),
					TexturePoint(65, 300),
					TexturePoint(360, 360)};
			}
			else
			{
				texturePoints = {};
			}

			auto it = paletteMap.find(tokens[1]);
			if (it != paletteMap.end())
			{
				colour = it->second;
			}
		}
	}

	return modelTriangles;
}

// Parse the colours
std::vector<Colour> colourParseOBJ(const std::string &filename)
{
	// Vector of colours
	std::vector<Colour> colours;

	// Open filestream
	std::ifstream file(filename);
	// Check working
	if (!file.is_open())
		return colours;

	// Current line
	std::string line;

	// Current colourName
	std::string colourName;

	// While getting lines
	while (std::getline(file, line))
	{
		// If the line its not empty but the end is a return
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		// If its empty
		if (line.empty())
		{
			continue;
		}

		// Split the line into "tokens"
		std::vector<std::string> tokens = split(line, ' ');

		if (tokens.empty() || tokens[0].empty())
		{
			continue;
		}

		// First token
		const std::string &prefix = tokens[0];

		// Check for new material
		if (prefix == "newmtl" && tokens.size() >= 2)
		{
			// Save the current colour name
			colourName = tokens[1];
		}
		// Check for colour values
		else if (prefix == "Kd" && tokens.size() >= 4)
		{
			// Push back with colourName
			// Static_cast and std::round combined with multiplying by 255
			// This makes it in 8 bit RGB
			// Stof to turn string to float
			colours.push_back(Colour(
				colourName,
				static_cast<int>(std::round(255 * std::stof(tokens[1]))),
				static_cast<int>(std::round(255 * std::stof(tokens[2]))),
				static_cast<int>(std::round(255 * std::stof(tokens[3])))));
		}
	}
	// Return vectors
	return colours;
}

int main(int argc, char *argv[])
{
	// Get the colour pallete
	palette = colourParseOBJ(objFilename + ".mtl");
	// Add it all to a Dictionary from a vector
	for (int i = 0; i < palette.size(); i++)
	{
		paletteMap[palette[i].name] = palette[i];
	}

	// Parse the triangles
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
