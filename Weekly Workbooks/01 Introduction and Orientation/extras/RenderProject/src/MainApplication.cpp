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
extern const int WIDTH = 700;
extern const int HEIGHT = 700;

// Bool to check whether we want stroked triangle view
extern bool WireframeBool;

// Orbiting int and LookAt bool
int isOrbiting = 0; // 0 = not orbiting, 1 = clockwise, -1 = counterclockwise
bool isLookAtEnabled = false;

bool GouraudShadingEnabled = false;
bool PhongShadingEnabled = false;

bool isSphereShowing = true;
bool isBlueBoxShowing = true;
bool isRedBoxShowing = true;

// Flag to enable/disable depth of field effect
bool depthOfFieldEnabled = true;
// Flag to enable/disable swirl effect
bool swirlEnabled = false;
// Flag to enable/disable reflex lens effect
bool reflexLensEnabled = true;

bool worldMirroringEnabled = true; // Flag to enable/disable world mirroring

// Texture maps
std::vector<TextureMap> textures;
std::unordered_map<std::string, int> textureIndexByMaterial;

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
float focalLength = 2.0f;
float focalDistance = 4.0f;
float lenseSize = 0.13f;
int raysPerPixel = 24;

glm::vec3 cameraPosition(0.0, 0.0, 4.0);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
glm::vec3 lightPosition(0.0, 0.8, 0.0);

// PI so i dont have to call a library
const double PI = 3.14159265358979323846;

// Triangle storage
std::vector<ModelTriangle> triangles;
std::vector<ModelTriangle> nonCulledTriangles;
std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;

// Colour storage
std::vector<Colour> palette;
std::unordered_map<std::string, Colour> paletteMap;

void loadTexture(const std::string &materialName, const std::string &path)
{
	textures.push_back(TextureMap(path));
	textureIndexByMaterial[materialName] = static_cast<int>(textures.size()) - 1;
}

void lookAt(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, const glm::vec3 &target)
{
	// Calculate the forward vector
	glm::vec3 forward = glm::normalize(cameraPosition - target);

	// Calculate the right vector by crossing the global "up"
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

	// Calculate the true up vector for the camera
	glm::vec3 up = glm::cross(forward, right);

	// Construct the new orientation matrix
	cameraOrientation = glm::mat3(right, up, forward);
}

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

	// Given formula
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
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		if (currentRenderer == &rayTracedRenderer)
		{
			rayTracedRenderer.focusOnPixel(event.button.x, event.button.y);
		}
		return;
	}

	if (event.type != SDL_KEYDOWN)
		return;

	switch (event.key.keysym.sym)
	{
	case SDLK_1:
		print("Switching to wireframe");
		WireframeBool = true;
		break;
	case SDLK_2:
		print("Switching to point cloud");
		WireframeBool = false;
		currentRenderer = &pointCloudRenderer;
		break;
	case SDLK_3:
		print("Switching to ray tracing render");
		currentRenderer = &rayTracedRenderer;
		break;
	case SDLK_EQUALS:
		print("Moving camera in");
		cameraPosition[2] -= 0.2;
		break;
	case SDLK_MINUS:
		print("Moving camera out");
		cameraPosition[2] += 0.2;
		break;
	case SDLK_UP:
		print("Moving camera up");
		cameraPosition[1] += 0.2;
		break;
	case SDLK_DOWN:
		print("Moving camera down");
		cameraPosition[1] -= 0.2;
		break;
	case SDLK_LEFT:
		print("Moving camera left");
		cameraPosition[0] -= 0.2;
		break;
	case SDLK_RIGHT:
		print("Moving camera right");
		cameraPosition[0] += 0.2;
		break;
	case SDLK_a:
		print("Rotating camera left");
		rotateY(cameraPosition, cameraOrientation, 10);
		break;
	case SDLK_d:
		print("Rotating camera right");
		rotateY(cameraPosition, cameraOrientation, -10);
		break;
	case SDLK_COMMA:
		if (isOrbiting == 0)
		{
			print("Orbiting camera clockwise around the object in the X axis");
			isOrbiting = 1;
		}
		else
		{
			print("Stopping orbiting");
			isOrbiting = 0;
		}
		break;
	case SDLK_PERIOD:
		if (isOrbiting == 0)
		{
			print("Orbiting camera counterclockwise around the object in the X axis");
			isOrbiting = -1;
		}
		else
		{
			print("Stopping orbiting");
			isOrbiting = 0;
		}
		break;
	case SDLK_w:
		print("Rotating camera up");
		rotateX(cameraPosition, cameraOrientation, 10);
		break;
	case SDLK_4:
		print("Toggle Gouraud shading");
		if (GouraudShadingEnabled)
		{
			print("Disabling Gouraud shading");
			GouraudShadingEnabled = false;
		}
		else
		{
			print("Enabling Gouraud shading");
			GouraudShadingEnabled = true;
			PhongShadingEnabled = false;
		}

		break;
	case SDLK_5:
		print("Toggle Phong shading");
		if (PhongShadingEnabled)
		{
			print("Disabling Phong shading");
			PhongShadingEnabled = false;
		}
		else
		{
			print("Enabling Phong shading");
			PhongShadingEnabled = true;
			GouraudShadingEnabled = false;
		}
		break;
	case SDLK_l:
		if (isLookAtEnabled)
		{
			print("Disabling Lookat");
			isLookAtEnabled = false;
		}
		else
		{
			print("Enabling Lookat");
			isLookAtEnabled = true;
		}
		break;
	case SDLK_s:
		print("Rotating camera down");
		rotateX(cameraPosition, cameraOrientation, -10);
		break;
	case SDLK_r:
		print("Toggling world mirroring");
		if (worldMirroringEnabled)
		{
			print("Disabling world mirroring");
			worldMirroringEnabled = false;
		}
		else
		{
			print("Enabling world mirroring");
			worldMirroringEnabled = true;
		}
		break;
	case SDLK_g:
		print("Placeholder: toggle glassy materials");
		break;
	case SDLK_m:
		print("Placeholder: toggle metallic surfaces");
		break;
	case SDLK_e:
		print("Placeholder: toggle environment maps");
		break;
	case SDLK_RETURN:
		print("Placeholder: toggle fullscreen");
		break;
	case SDLK_c:
		print("Toggle Sphere visibility");
		if (isSphereShowing)
		{
			print("Hiding Sphere");
			isSphereShowing = false;
		}
		else
		{
			print("Showing Sphere");
			isSphereShowing = true;
		}
		break;
	case SDLK_v:
		print("Toggle Blue Box visibility");
		if (isBlueBoxShowing)
		{
			print("Hiding Blue Box");
			isBlueBoxShowing = false;
		}
		else
		{
			print("Showing Blue Box");
			isBlueBoxShowing = true;
		}
		break;
	case SDLK_b:
		print("Toggle Red Box visibility");
		if (isRedBoxShowing)
		{
			print("Hiding Red Box");
			isRedBoxShowing = false;
		}
		else
		{
			print("Showing Red Box");
			isRedBoxShowing = true;
		}
		break;
	case SDLK_f:
		print("Toggle Depth of Field");
		if (depthOfFieldEnabled)
		{
			print("Disabling Depth of Field");
			depthOfFieldEnabled = false;
			swirlEnabled = false;	   // Disable swirl effect when depth of field is disabled
			reflexLensEnabled = false; // Disable reflex lens effect when depth of field is disabled
		}
		else
		{
			print("Enabling Depth of Field");
			depthOfFieldEnabled = true;
		}
		break;
	case SDLK_z:
		print("Toggle Swirl Effect");
		if (swirlEnabled)
		{
			print("Disabling Swirl Effect");
			swirlEnabled = false;
		}
		else
		{
			print("Enabling Swirl Effect");
			swirlEnabled = true;
			reflexLensEnabled = false; // Disable reflex lens effect when swirl is enabled
		}
		break;
	case SDLK_x:
		print("Toggle Reflex Lens Effect");
		if (reflexLensEnabled)
		{
			print("Disabling Reflex Lens Effect");
			reflexLensEnabled = false;
		}
		else
		{
			print("Enabling Reflex Lens Effect");
			reflexLensEnabled = true;
			swirlEnabled = false; // Disable swirl effect when reflex lens is enabled
		}
		break;
	default:
		break;
	}
}

std::vector<ModelTriangle> triangleParseOBJ(const std::string &filename, float scalingFactor)
{

	// Vector of Triangles
	std::vector<ModelTriangle> modelTriangles;
	// Vector of Vertices
	std::vector<glm::vec3> vertices;

	std::vector<glm::vec2> textureVertices;

	// ifstream
	std::ifstream file(filename);
	// Check working
	if (!file.is_open())
		return modelTriangles;

	// string = to line
	std::string line;

	// Keep track of the current texture index for the triangles
	int currentTextureIndex = -1;

	// Default Colour if doesn't work
	Colour colour(255, 255, 255);

	// Keep track of the current material name for the triangles
	std::string currentMaterialName;

	bool isCurrentMirror = false; // Flag to indicate if the current material is a mirror

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
		else if (prefix == "vt" && tokens.size() >= 3)
		{
			// Push back texture vertex
			textureVertices.push_back(glm::vec2(
				std::stof(tokens[1]),
				std::stof(tokens[2])));
		}
		else if (prefix == "o" && tokens.size() >= 2)
		{
			currentMaterialName = tokens[1];
		}
		else if (prefix == "f" && tokens.size() >= 4)
		{
			// Parse the vertex indices and texture indices
			int idx[3];
			int texIdx[3];
			for (int i = 0; i < 3; i++)
			{
				// Split the face token by '/' to separate vertex and texture indices
				std::vector<std::string> parts = split(tokens[i + 1], '/');
				// Convert the vertex index to an integer and adjust for 0-based indexing
				idx[i] = std::stoi(parts[0]) - 1;
				// Convert the texture index to an integer and adjust for 0-based indexing, if it exists; otherwise, set to -1
				texIdx[i] = (parts.size() > 1 && !parts[1].empty()) ? std::stoi(parts[1]) - 1 : -1;
			}
			if (colour.name == "Mirror")
			{
				isCurrentMirror = true;
			}
			else
			{
				isCurrentMirror = false;
			}
			ModelTriangle triangle(vertices[idx[0]], vertices[idx[1]], vertices[idx[2]], colour);

			triangle.isMirror = isCurrentMirror; // Set the isMirror flag for the triangle
			// If there is a texture
			if (currentTextureIndex >= 0)
			{
				bool valid = true;
				for (int i = 0; i < 3; i++)
				{
					// if the texture index is invalid, set valid to false
					if (texIdx[i] < 0 || texIdx[i] >= static_cast<int>(textureVertices.size()))
						valid = false;
				}
				if (valid)
				{
					// Get the texture map for the current material
					const TextureMap &texture = textures[currentTextureIndex];
					for (int i = 0; i < 3; i++)
					{
						// Get the texture coordinates (u, v) for the vertex
						float u = textureVertices[texIdx[i]].x;
						float v = textureVertices[texIdx[i]].y;
						// Convert (u, v) to pixel coordinates in the texture map and assign to the triangle's texturePoints
						triangle.texturePoints[i] = TexturePoint(u * texture.width, v * texture.height);
					}
					triangle.textureIndex = currentTextureIndex;
				}
			}
			glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
			glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];

			// Winding order is clockwise? Can't really wrap my head around it, but this works and the opposite leaves it in shadow
			glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
			triangle.normal = normal;
			triangle.objectName = currentMaterialName;
			modelTriangles.push_back(triangle);
		}
		else if (prefix == "usemtl" && tokens.size() >= 2)
		{
			// Check if the material name exists in the textureIndexByMaterial map
			auto tex = textureIndexByMaterial.find(tokens[1]);
			// If it exists, set currentTextureIndex to the corresponding index; otherwise, set it to -1
			currentTextureIndex = (tex != textureIndexByMaterial.end()) ? tex->second : -1;

			// Check if the material name exists in the paletteMap
			auto it = paletteMap.find(tokens[1]);
			// If it exists, set the colour to the corresponding Colour; otherwise, keep the previous colour
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
	float smoothThreshold = std::cos(glm::radians(45.0f)); // include all faces sharing the vertex
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
	loadTexture("Cobbles", "../../../05 Navigation and Transformation/models/texture (1).ppm");

	std::vector<std::string> filennames = {
		"../../../05 Navigation and Transformation/models/textured-cornell-box",
		"../../../07 Lighting and Shading/resources/sphere"};
	std::string cornellBoxFilename = filennames[0]; // Use the first filename
	// Get the colour pallete
	palette = colourParseOBJ(cornellBoxFilename + ".mtl");
	// Add it all to a Dictionary from a vector
	for (int i = 0; i < palette.size(); i++)
	{
		paletteMap[palette[i].name] = palette[i];
	}

	// Parse the triangles
	for (const std::string &filename : filennames)
	{
		std::vector<ModelTriangle> parsedTriangles = triangleParseOBJ(filename + ".obj", 0.35f);
		triangles.insert(triangles.end(), parsedTriangles.begin(), parsedTriangles.end());
	}

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

	glm::vec3 tempCameraPosition = cameraPosition;
	glm::mat3 tempCameraOrientation = cameraOrientation;

	while (true)
	{
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event))
			handleEvent(event, window);
		// In C++ we can't use "." notation for dynamic dispatch, so we have to use -> instead :o(
		currentRenderer->draw(window);

		if (isOrbiting != 0)
		{
			rotateY(cameraPosition, cameraOrientation, isOrbiting);
			if (isLookAtEnabled)
			{
				lookAt(cameraPosition, cameraOrientation, glm::vec3(0.0f, 0.0f, 0.0f));
			}
		}
		// Need to ask the SDL window to render the frame, otherwise nothing actually gets shown on the screen !
		window.renderFrame();
		if (savingFrames && (tempCameraPosition != cameraPosition || tempCameraOrientation != cameraOrientation))
		{
			tempCameraPosition = cameraPosition;
			tempCameraOrientation = cameraOrientation;
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
