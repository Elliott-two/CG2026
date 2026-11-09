#include "Renderer.h"
#include "RedNoiseRenderer.h"
#include "BlueNoiseRenderer.h"
#include "GreenNoiseRenderer.h"
#include "WhiteNoiseRenderer.h"
#include "ColourSpectrumRenderer.h"
#include "TriangleSpectrumRenderer.h"
#include "RasterisedRenderer.h"
#include "PointCloudRenderer.h"
#include "RayTracedRenderer.h"
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

// Define globals for WIDTH and HEIGHT of the window (can be accessed from any renderer)
extern const int WIDTH = 1000;
extern const int HEIGHT = 1000;

// Bool to check whether we want stroked triangle view
extern bool WireframeBool;

// Texture map for cobbles
TextureMap textureMap(
	"../../../05 Navigation and Transformation/models/texture (1).ppm");

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT);

// Create instances of each renderer

// Noise colours (Not told to do this but had some fun)
RedNoiseRenderer redNoise = RedNoiseRenderer();
GreenNoiseRenderer greenNoise = GreenNoiseRenderer();
BlueNoiseRenderer blueNoise = BlueNoiseRenderer();
WhiteNoiseRenderer whiteNoise = WhiteNoiseRenderer();

// Colour spectrums
ColourSpectrumRenderer colourSpectrum = ColourSpectrumRenderer();
TriangleSpectrumRenderer triangleSpectrum = TriangleSpectrumRenderer();

// Cornell Box and Sphere Renderers
RasterisedRenderer rasterisedRenderer = RasterisedRenderer();
PointCloudRenderer pointCloudRenderer = PointCloudRenderer();
RayTracedRenderer rayTracedRenderer = RayTracedRenderer();

// The default starting point
Renderer *currentRenderer = &pointCloudRenderer;

// Unnecessary for now
bool savingFrames = false;
int frameCounter = 0;

// Filenames
std::string objFilename = "../../../05 Navigation and Transformation/models/textured-cornell-box";
std::string objFilenameSphere = "../../../07 Lighting and Shading/resources/sphere";

// Core 3D rendering data structures (for when we eventually get around to working in 3D ;o)
float focalLength = 2.0;
glm::vec3 cameraPosition(0.0, 0.0, 4.0);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
glm::vec3 lightPosition(-1.0f, 1.5f, 1.0f);

// PI so i dont have to call a library
const double PI = 3.14159265358979323846;

// Triangle storage
std::vector<ModelTriangle> triangles;
std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;

// Colour storage
std::vector<Colour> palette;
std::unordered_map<std::string, Colour> paletteMap;

// Rotation functions
void rotateX(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, int degrees)
{
	double degreeInRadians = degrees * (PI / 180.0);
	double cos_value = std::cos(degreeInRadians);
	double sin_value = std::sin(degreeInRadians);

	// Given formula
	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.0, cos_value, sin_value),
		glm::vec3(0.0, -sin_value, cos_value));

	// Correct order
	cameraOrientation = rotationMatrix * cameraOrientation;
	cameraPosition = rotationMatrix * cameraPosition;
}

// Same as above but for Y
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

// I hate hate hate std::cout and std::endl it looks so ugly
// So i made this cause its so much easier
void print(std::string s)
{
	std::cout << s << std::endl;
}

// Handle all events
void handleEvent(SDL_Event event, DrawingWindow &window, Renderer *renderer = currentRenderer)
{
	if (event.type == SDL_KEYDOWN)
	{
		// Switched to switch Case from if-else chain as it looks nicer
		switch (event.key.keysym.sym)
		{
		// Drawing Triangles
		case SDLK_u:
			print("Drawing Stroked Triangles");
			rasterisedRenderer.drawStrokedTriangles(window);
			break;
		case SDLK_t:
			print("Drawing Textured Triangles");
			rasterisedRenderer.texturedTriangle(window, textureMap);
			break;
		case SDLK_v:
			print("Toggling Wireframe");
			WireframeBool = !WireframeBool;
			break;
		// Clear Pixels
		case SDLK_BACKSPACE:
			print("Clearing Pixels");
			rasterisedRenderer.clearPixels(window);
			break;
		// Camera Movements
		case SDLK_LEFT:
			print("Moving Camera left");
			cameraPosition[0] -= 0.2;
			break;
		case SDLK_RIGHT:
			print("Moving Camera Right");
			cameraPosition[0] += 0.2;
			break;
		case SDLK_UP:
			print("Moving Camera Up");
			cameraPosition[1] += 0.2;
			break;
		case SDLK_DOWN:
			print("Moving Camera Down");
			cameraPosition[1] -= 0.2;
			break;
		case SDLK_LEFTBRACKET:
			print("Moving Camera in");
			cameraPosition[2] -= 0.2;
			break;
		case SDLK_RIGHTBRACKET:
			print("Moving Camera Out");
			cameraPosition[2] += 0.2;
			break;
		case SDLK_l:
			print("Rotating camera by 10 degrees in the x plane");
			rotateX(cameraPosition, cameraOrientation, 10);
			break;
		case SDLK_SEMICOLON:
			print("Rotating camera by negative 10 degrees in the x plane");
			rotateX(cameraPosition, cameraOrientation, -10);
			break;
		case SDLK_BACKSLASH:
			print("Rotating camera by 10 degrees in the y plane");
			rotateY(cameraPosition, cameraOrientation, 10);
			break;
		case SDLK_QUOTE:
			print("Rotating camera by negative 10 degrees in the y plane");
			rotateY(cameraPosition, cameraOrientation, -10);
			break;
		case SDLK_9:
			print("Change Rotating X");
			OrbitXBool = !OrbitXBool;
			break;
		case SDLK_0:
			print("Change Rotating Y");
			OrbitYBool = !OrbitYBool;
			break;

		// Switch between renderers based on key presses
		case SDLK_p:
			print("Turning Rasterised Renderer");
			currentRenderer = &rasterisedRenderer;
			rasterisedRenderer.clearPixels(window); // Clear the window when switching to rasterised renderer
			break;
		case SDLK_c:
			print("Turning Colour Spectrum");
			currentRenderer = &colourSpectrum;
			break;
		case SDLK_m:
			print("Turning Triangle Spectrum");
			currentRenderer = &triangleSpectrum;
			break;
		case SDLK_r:
			print("Turning Red");
			currentRenderer = &redNoise;
			break;
		case SDLK_g:
			print("Turning Green");
			currentRenderer = &greenNoise;
			break;
		case SDLK_b:
			print("Turning Blue");
			currentRenderer = &blueNoise;
			break;
		case SDLK_w:
			print("Turning White");
			currentRenderer = &whiteNoise;
			break;
		case SDLK_k:
			print("Turning Point Cloud");
			currentRenderer = &pointCloudRenderer;
			break;
		case SDLK_RETURN:
			print("Turning RayTracing");
			currentRenderer = &rayTracedRenderer;
			break;

		// Adjust the RGB values based on key presses
		case SDLK_1:
			currentRenderer->adjustRed(+5);
			print("Increase Red");
			break;
		case SDLK_2:
			currentRenderer->adjustRed(-5);
			print("Decrease Red");
			break;
		case SDLK_3:
			currentRenderer->adjustBlue(+5);
			print("Increase Blue");
			break;
		case SDLK_4:
			currentRenderer->adjustBlue(-5);
			print("Decrease Blue");
			break;
		case SDLK_5:
			currentRenderer->adjustGreen(+5);
			print("Increase Green");
			break;
		case SDLK_6:
			currentRenderer->adjustGreen(-5);
			print("Decrease Green");
			break;
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
			glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
			glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];

			// Winding order is clockwise? Can't really wrap my head around it, but this works and the opposite leaves it in shadow
			glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
			triangle.normal = normal;
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

std::array<glm::vec3, 3> vertexNormals(const ModelTriangle &triangle)
{
	float smoothThreshold = -1.0f; // include all faces sharing the vertex
	std::array<glm::vec3, 3> normals;

	for (int i = 0; i < 3; ++i)
	{
		// Assign vertices
		glm::vec3 vertex = triangle.vertices[i];
		glm::vec3 normalSum(0.0f);

		// Loop through all triangles to find those that share the vertex
		for (const ModelTriangle &other : triangles)
		{
			if (other.vertices[0] != vertex && other.vertices[1] != vertex && other.vertices[2] != vertex)
				continue;

			// Calculate the normal of the other triangle
			glm::vec3 otherNormal = glm::normalize(glm::cross(other.vertices[1] - other.vertices[0], other.vertices[2] - other.vertices[0]));

			// Check if the angle between the normals is within the smooth threshold
			// If the dot product is greater than the threshold, they are considered smooth
			if (glm::dot(otherNormal, triangle.normal) > smoothThreshold)
				normalSum += otherNormal;
		}
		// Normalize the summed normal to get the average normal for the vertex
		normals[i] = glm::normalize(normalSum);
	}
	return normals;
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
	triangles = triangleParseOBJ(objFilenameSphere + ".obj", 0.35f);

	// Clear the vector? Given by intellisense so im going to trust it
	vertexNormalsByTriangle.clear();
	// Reserve the space, also given by intellisense
	vertexNormalsByTriangle.reserve(triangles.size());

	// Loop through triangles and find the vertex normals
	for (const ModelTriangle &triangle : triangles)
	{
		vertexNormalsByTriangle.push_back(vertexNormals(triangle));
	}

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
			// window.saveBMP("saved-frames", paddedCounter + ".bmp");
			frameCounter++;
		}
	}
}
