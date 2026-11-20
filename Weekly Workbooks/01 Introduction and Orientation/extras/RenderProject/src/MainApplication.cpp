#include "Renderer.h"
#include "RasterisedRenderer.h"
#include "PointCloudRenderer.h"
#include "RayTracedRenderer.h"
#include "PhotonMap.h"
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <random>
#include <set>

// Define globals for WIDTH and HEIGHT of the window
extern const int WIDTH = 640;
extern const int HEIGHT = 480;

// Bool to check whether we want stroked triangle view
extern bool WireframeBool;

// Orbiting float
// Decides the speed of orbiting
float isOrbiting = 0; // 0 = not orbiting, 1 = clockwise, -1 = counterclockwise

// LookAt bool, activating forces the camera to look at the world center
bool isLookAtEnabled = false;

// Bool to force redraw rayTracing
// Used for when things change in the scene
bool forceRayTraceRedraw = false;

// Bool to force a redraw of Photon maps
// Needed for when things to do with lighting may change
bool forcePhotonMapRedraw = false;

// Bool for different shading techniques
bool GouraudShadingEnabled = false;
bool PhongShadingEnabled = false;

// Bool to check if the sphere, boxes, and bunny are showing
// All on at the start of the scene
bool isSphereShowing = true;
bool isBlueBoxShowing = true;
bool isRedBoxShowing = true;
bool isBunnyShowing = true;

// Flags to toggle:

// Depth of field effect
bool depthOfFieldEnabled = false;

// Swirl effect - option for depth of field
bool swirlEnabled = false;

// Reflex lens effect - option for depth of field
bool reflexLensEnabled = false;

// World mirroring - Turns mirrors back to their unreflective state when false
bool worldMirroringEnabled = false;

// World refractive materials - Sets them back to the unrefractive version when false
bool worldRefractiveEnabled = false;

// World metallic materials - Makes them simple flat colours when false
bool worldMetallicEnabled = false;

// Environment maps
bool environmentMapEnabled = false;

// Environment map visibility to camera - when false, the camera can only see the map through a reflection
bool environmentMapSeenByCamera = false;

// Photon mapping -
// When Refractive materials are on - Globals and Caustics can be seen
bool photonMappingEnabled = false;

// Photon debugging - When true, the camera can only see the photons
bool photonDebuggingEnabled = false;

// Soft shadows
bool softShadowsEnabled = false;

// PI used within the main file
// Ran into some problems with it declared over one file
// Each file now has its own named PI
constexpr float MAIN_PI = 3.14159265358979323846f;

// Time variable for water animation speed
float waterTime = 0.0f;

// Light position and min/max values for light calculations
glm::vec3 lightMin(0.0f);
glm::vec3 lightMax(0.0f);

// Texture maps
TextureMap environmentMap;
std::vector<TextureMap> textures;
std::unordered_map<std::string, int> textureIndexByMaterial;

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT);

// Cornell Box and Sphere Renderers
RasterisedRenderer rasterisedRenderer = RasterisedRenderer();
PointCloudRenderer pointCloudRenderer = PointCloudRenderer();
RayTracedRenderer rayTracedRenderer = RayTracedRenderer();

// The default starting point
Renderer *currentRenderer = &pointCloudRenderer;

// Frame based variables
bool savingFrames = false;
int frameCounter = 0;
bool newFrameRendered = false;

// Filenames
std::string objFilename = "src/textured-cornell-box";
std::string objFilenameSphere = "src/sphere";

// Core 3D rendering data structures (for when we eventually get around to working in 3D ;o)
// (I have now got round to working in 3D ;o)
float focalLength = 2.0;
float focalDistance = 4.0f;
float lenseSize = 0.13f;
int raysPerPixel = 4;

// Camera and light positions
glm::vec3 cameraPosition(0.0, 0.0, 6.0);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
glm::vec3 lightPosition(0.0, 0.8, 0.0);

// Animation flags
bool isAnimating = false;
int animationTick = 0;

// Triangle storage
std::vector<ModelTriangle> triangles;
std::vector<ModelTriangle> nonCulledTriangles;
std::vector<std::array<glm::vec3, 3>> vertexNormalsByTriangle;

// Colour storage
std::vector<Colour> palette;
std::unordered_map<std::string, Colour> paletteMap;

// Store the properties of each material type in a struct for easy access
struct MaterialProperties
{
	bool isMirror;				  // Check for mirror
	bool isWater;				  // Check for water
	bool isRefractive;			  // Check for refractive
	bool isMetallic;			  // Check for metallic
	float roughness;			  // Roughness for metallic materials
	float refractiveIndex;		  // Refractive index for refractive materials
	float stretchFactor;		  // Stretch factor for stretched metals
	glm::vec3 fresnelReflectance; // Fresnel reflectance for metallic materials
	glm::vec3 absorption;		  // Absorption for refractive materials
};

// Define a map to associate material names with their properties
std::unordered_map<std::string, MaterialProperties> materialProperties = {
	// Reflective
	{"Mirror", {true, false, false, false, 0.0f, 1.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f)}},

	// Refractive
	{"Glass", {false, false, true, false, 0.0f, 1.5f, 1.0f, glm::vec3(0.0f), glm::vec3(0.35f, 0.10f, 0.25f)}},
	{"Water", {false, true, true, false, 0.0f, 1.333f, 1.0f, glm::vec3(0.0f), glm::vec3(0.90f, 0.25f, 0.15f)}},

	// Metals
	{"Gold", {false, false, false, true, 0.15f, 1.0f, 1.0f, glm::vec3(1.00f, 0.71f, 0.29f), glm::vec3(0.0f)}},
	{"Silver", {false, false, false, true, 0.05f, 1.0f, 1.0f, glm::vec3(0.95f, 0.93f, 0.88f), glm::vec3(0.0f)}},
	{"Copper", {false, false, false, true, 0.33f, 1.0f, 1.0f, glm::vec3(0.95f, 0.64f, 0.54f), glm::vec3(0.0f)}},
	{"Aluminium", {false, false, false, true, 0.53f, 1.0f, 3.0f, glm::vec3(0.91f, 0.92f, 0.92f), glm::vec3(0.0f)}},
	{"Iron", {false, false, false, true, 0.8f, 1.0f, 1.0f, glm::vec3(0.56f, 0.57f, 0.58f), glm::vec3(0.04f)}}};

// When given an object and a material, apply the respective properties to the object
void applyMaterialToObject(const std::string &objectName, const std::string &materialName)
{
	// Default properties - Basic material
	MaterialProperties properties = {
		false, false, false, false, 0.0f, 1.0f, 1.0f, glm::vec3(1.0f), glm::vec3(0.0f)};

	// Find material in Map
	auto material = materialProperties.find(materialName);
	if (material != materialProperties.end())
	{
		properties = material->second;
	}

	// Find colour in Map
	Colour colour = Colour(255, 255, 255);
	auto paletteColour = paletteMap.find(materialName);
	if (paletteColour != paletteMap.end())
	{
		colour = paletteColour->second;
	}

	bool anythingChanged = false;
	// For every triangle in triangles
	for (ModelTriangle &triangle : triangles)
	{
		// Skip the objects that dont share the name
		if (triangle.objectName != objectName)
		{
			continue;
		}

		bool anythingChangedForThisTriangle = (triangle.colour.red != colour.red ||
											   triangle.colour.green != colour.green ||
											   triangle.colour.blue != colour.blue ||
											   triangle.isMirror != properties.isMirror ||
											   triangle.isWater != properties.isWater ||
											   triangle.isRefractive != properties.isRefractive ||
											   triangle.isMetallic != properties.isMetallic ||
											   triangle.absorption != properties.absorption ||
											   triangle.fresnelReflectance != properties.fresnelReflectance ||
											   triangle.roughness != properties.roughness ||
											   triangle.stretchFactor != properties.stretchFactor ||
											   triangle.refractiveIndex != properties.refractiveIndex);
		if (anythingChangedForThisTriangle)
		{
			anythingChanged = true;
		}
		// Assign all of the properties
		triangle.colour = colour;
		triangle.textureIndex = -1;
		triangle.isMirror = properties.isMirror;
		triangle.isWater = properties.isWater;
		triangle.isRefractive = properties.isRefractive;
		triangle.isMetallic = properties.isMetallic;
		triangle.absorption = properties.absorption;
		triangle.fresnelReflectance = properties.fresnelReflectance;
		triangle.roughness = properties.roughness;
		triangle.stretchFactor = properties.stretchFactor;
		triangle.refractiveIndex = properties.isRefractive ? properties.refractiveIndex : 1.0f;
	}

	// We must force both redraws
	// Force RayTrace as the colour can be seen from the camera
	// Force PhotonMap as survivability is based on colour and material type
	if (anythingChanged)
	{
		forceRayTraceRedraw = true;
		forcePhotonMapRedraw = true;
	}
}

// Function to change the objects roughness
void setRoughnessForObject(const std::string &objectName, float roughness)
{
	// Loop over every triangle to check for the objects name
	for (ModelTriangle &triangle : triangles)
	{
		// Cannot change the roughness of a non metallic object
		if (triangle.objectName == objectName && triangle.isMetallic)
		{
			if (triangle.roughness == roughness)
			{
				return;
			}
			triangle.roughness = roughness;
		}
	}

	// Must force redraw of the scene as the colours can be seen
	forceRayTraceRedraw = true;
}

// Function to change the objects roughness
void increaseRoughnessForObject(const std::string &objectName, float roughness)
{
	// Loop over every triangle to check for the objects name
	for (ModelTriangle &triangle : triangles)
	{
		// Cannot change the roughness of a non metallic object
		if (triangle.objectName == objectName && triangle.isMetallic)
		{
			triangle.roughness += roughness;
		}
	}

	// Must force redraw of the scene as the colours can be seen
	forceRayTraceRedraw = true;
}

void setAbsorptionForObject(const std::string &objectName, const glm::vec3 &absorption)
{
	for (ModelTriangle &triangle : triangles)
	{
		if (triangle.objectName == objectName && triangle.isWater)
		{
			if (triangle.absorption == absorption)
			{
				return;
			}
			triangle.absorption = absorption;
		}
	}

	// Force both redraw as light changes based on absorption
	forceRayTraceRedraw = true;
	forcePhotonMapRedraw = true;
}

void increaseAbsorptionForObject(const std::string &objectName, const glm::vec3 &absorption)
{
	for (ModelTriangle &triangle : triangles)
	{
		if (triangle.objectName == objectName && triangle.isWater)
		{
			triangle.absorption += absorption;
		}
	}

	// Force both redraw as light changes based on absorption
	forceRayTraceRedraw = true;
	forcePhotonMapRedraw = true;
}

// For every colour, create a material to help change them easier
void registerPaletteColoursAsMaterials()
{
	// Plain materials
	const MaterialProperties defaultProperties = {
		false, false, false, false, 0.0f, 1.0f, 1.0f, glm::vec3(1.0f), glm::vec3(0.0f)};

	// Loop over every colour in the pallette
	for (const Colour &colour : palette)
	{
		// Add into the map
		materialProperties.emplace(colour.name, defaultProperties);
		paletteMap[colour.name] = colour;
	}
}

void loadTexture(const std::string &materialName, const std::string &path)
{
	textures.push_back(TextureMap(path));
	textureIndexByMaterial[materialName] = static_cast<int>(textures.size()) - 1;
}

// LookAt forces the camera to look at the world center
void lookAt(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, const glm::vec3 &target)
{
	// The direction from the taret back to the camera
	glm::vec3 forwardVector = cameraPosition - target;
	// If the distance between the camera position and the target is very very small, we can simply return
	if (glm::length(forwardVector) < 1e-5f)
	{
		return;
	}

	// Calculate the forward vector
	glm::vec3 forward = glm::normalize(forwardVector);

	// The worldUp vector, positive Y means up
	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

	// Check if our forward vectors y coordinate is very close to 1
	// if so our world up becomes positive Z
	// This prevents division by 0
	// Without this almost parallel cross product is nearly zero
	if (std::abs(forward.y) > 0.999f)
	{
		worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	// Calculate the right vector by crossing the global "up"
	glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));

	// Calculate the true up vector for the camera
	glm::vec3 up = glm::cross(forward, right);

	// Construct the new orientation matrix
	cameraOrientation = glm::mat3(right, up, forward);
}

// Rotation functions
void rotateX(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, float degrees)
{
	// Convert to radians for matrix manipulation
	// Year 1 Maths is in fact useful
	double degreeInRadians = double(degrees) * (MAIN_PI / 180.0);

	// Assign variables for ease of use
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
void rotateY(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, float degrees)
{
	float degreeInRadians = degrees * (MAIN_PI / 180.0);
	float cos_value = std::cos(degreeInRadians);
	float sin_value = std::sin(degreeInRadians);

	// Given formula
	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(cos_value, 0.0, -sin_value),
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(sin_value, 0.0, cos_value));

	cameraOrientation = rotationMatrix * cameraOrientation;
	cameraPosition = rotationMatrix * cameraPosition;
}

// Look the camera left or right without moving the camera
void panCamera(glm::mat3 &cameraOrientation, float degrees)
{
	float degreeInRadians = degrees * (MAIN_PI / 180.0);
	float cos_value = std::cos(degreeInRadians);
	float sin_value = std::sin(degreeInRadians);

	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(cos_value, 0.0f, -sin_value),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(sin_value, 0.0f, cos_value));

	// Dont change position
	cameraOrientation = rotationMatrix * cameraOrientation;
}

// Look up or down without actually moving the camera
void tiltCamera(glm::mat3 &cameraOrientation, float degrees)
{
	// Our forward vector
	float forwardY = -cameraOrientation[2].y;

	// Stop the camera from tilting too far up or down
	if ((degrees > 0.0f && forwardY > 0.95f) || (degrees < 0.0f && forwardY < -0.95f))
	{
		return;
	}

	float radians = degrees * (MAIN_PI / 180.0);
	float cosine = std::cos(radians);
	float sine = std::sin(radians);
	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, cosine, sine),
		glm::vec3(0.0f, -sine, cosine));
	cameraOrientation = cameraOrientation * rotationMatrix;
}

// I hate hate hate std::cout and std::endl it looks so ugly
// So i made this cause its so much easier
void print(std::string s)
{
	std::cout << s << std::endl;
}

// Input keymap:
// Mouse left-click: focus the ray tracer on the selected pixel
// 1/2/3: wireframe / rasterised / ray-traced renderer
// =/-: move camera forward / backward; arrow keys: move camera up/down/left/right
// W/S: tilt camera up/down; A/D: pan camera left/right
// Shift+S: toggle soft shadows; //.: toggle clockwise/counterclockwise orbiting
// 4/5: toggle Gouraud / Phong shading; L: toggle look-at mode
// R/G/M: toggle mirrored / glassy / metallic materials
// E/]: toggle environment maps / environment-map visibility
// Enter: toggle fullscreen
// C/V/B/N: toggle sphere / blue box / red box / bunny visibility
// F/Z/X: toggle depth of field / swirl / reflex lens effects
// P: toggle frame saving; Backspace: start animation
// K/J: toggle photon mapping / photon debugging
// Handle all events
void handleEvent(SDL_Event event, DrawingWindow &window, Renderer *renderer = currentRenderer)
{

	// Nice little focus on where i click
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		if (currentRenderer == &rayTracedRenderer)
		{
			rayTracedRenderer.focusOnPixel(event.button.x, event.button.y);
		}
		return;
	}

	if (event.type != SDL_KEYDOWN)
	{
		return;
	}

	// Basic orientations
	const float MOVE_STEP = 0.2f;
	const glm::vec3 camRight = cameraOrientation[0];
	const glm::vec3 camUp = cameraOrientation[1];
	const glm::vec3 camForward = -cameraOrientation[2];
	// Basic switch cases pretty self explanatory
	switch (event.key.keysym.sym)
	{
	case SDLK_1:
		print("Switching to Wireframe");
		currentRenderer = &pointCloudRenderer;
		WireframeBool = true;
		break;
	case SDLK_2:
		print("Switching to Rasterised Renderer");
		WireframeBool = false;
		currentRenderer = &pointCloudRenderer;
		break;
	case SDLK_3:
		print("Switching to Ray Tracing Renderer");
		GouraudShadingEnabled = false;
		PhongShadingEnabled = false;
		WireframeBool = false;
		forceRayTraceRedraw = true;
		currentRenderer = &rayTracedRenderer;
		break;
	case SDLK_EQUALS:
		print("Moving camera in");
		cameraPosition += camForward * MOVE_STEP;
		break;
	case SDLK_MINUS:
		print("Moving camera out");
		cameraPosition -= camForward * MOVE_STEP;
		break;
	case SDLK_UP:
		print("Moving camera up");
		cameraPosition += camUp * MOVE_STEP;
		break;
	case SDLK_DOWN:
		print("Moving camera down");
		cameraPosition -= camUp * MOVE_STEP;
		break;
	case SDLK_LEFT:
		print("Moving camera left");
		cameraPosition -= camRight * MOVE_STEP;
		break;
	case SDLK_RIGHT:
		print("Moving camera right");
		cameraPosition += camRight * MOVE_STEP;
		break;
	case SDLK_a:
		print("Panning camera left");
		panCamera(cameraOrientation, 6);
		break;
	case SDLK_d:
		print("Panning camera right");
		panCamera(cameraOrientation, -6);
		break;
	case SDLK_w:
		print("Tilting camera up");
		tiltCamera(cameraOrientation, 6);
		break;
	case SDLK_s:
		if (event.key.keysym.mod & KMOD_SHIFT)
		{
			if (softShadowsEnabled)
			{
				print("Disabling soft shadows");
				softShadowsEnabled = false;
			}
			else
			{
				print("Enabling soft shadows");
				softShadowsEnabled = true;
			}
		}
		else
		{
			print("Tilting camera down");
			tiltCamera(cameraOrientation, -6);
		}
		break;
	case SDLK_COMMA:
		if (isOrbiting == 0)
		{
			print("Orbiting camera clockwise around the object in the Y axis");
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
			print("Orbiting camera counterclockwise around the object in the Y axis");
			isOrbiting = -1;
		}
		else
		{
			print("Stopping orbiting");
			isOrbiting = 0;
		}
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
			currentRenderer = &rayTracedRenderer;
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
			currentRenderer = &rayTracedRenderer;
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
		print("Toggling glassy materials");
		if (worldRefractiveEnabled)
		{
			print("Disabling glassy materials");
			worldRefractiveEnabled = false;
		}
		else
		{
			print("Enabling glassy materials");
			worldRefractiveEnabled = true;
		}
		break;
	case SDLK_m:
		print("Toggle metallic surfaces");
		if (worldMetallicEnabled)
		{
			print("Disabling metallic surfaces");
			worldMetallicEnabled = false;
		}
		else
		{
			print("Enabling metallic surfaces");
			worldMetallicEnabled = true;
		}
		break;
	case SDLK_e:
		print("Toggle environment maps");
		if (environmentMapEnabled)
		{
			print("Disabling environment maps");
			environmentMapEnabled = false;
		}
		else
		{
			print("Enabling environment maps");
			environmentMapEnabled = true;
		}
		break;
	case SDLK_RIGHTBRACKET:
		print("Toggling environment map visibility");
		if (environmentMapSeenByCamera)
		{
			print("Disabling environment map visibility");
			environmentMapSeenByCamera = false;
		}
		else
		{
			print("Enabling environment map visibility");
			environmentMapSeenByCamera = true;
		}
		break;
	case SDLK_RETURN:
		print("Toggling fullscreen");
		window.toggleFullscreen();
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
	case SDLK_n:
		print("Toggle Bunny visibility");
		if (isBunnyShowing)
		{
			print("Hiding Bunny");
			isBunnyShowing = false;
		}
		else
		{
			print("Showing Bunny");
			isBunnyShowing = true;
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
	case SDLK_p:
		print("Toggle saving frames");
		if (savingFrames)
		{
			print("Stopping saving frames");
			savingFrames = false;
		}
		else
		{
			print("Starting saving frames");
			savingFrames = true;
		}
		break;
	case SDLK_BACKSPACE:
		print("Starting Animation");
		isAnimating = true;
		animationTick = 0;
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
		cameraOrientation = glm::mat3(1.0f);
		break;
	case SDLK_k:
		print("Toggle photon mapping");
		if (photonMappingEnabled)
		{
			print("Disabling photon mapping");
			photonMappingEnabled = false;
		}
		else
		{
			print("Enabling photon mapping");
			photonMappingEnabled = true;
		}
		break;
	case SDLK_j:
		print("Toggle photon debugging");
		if (photonDebuggingEnabled)
		{
			print("Disabling photon debugging");
			photonDebuggingEnabled = false;
		}
		else
		{
			print("Enabling photon debugging");
			photonDebuggingEnabled = true;
		}
		break;
	default:
		break;
	}
}

constexpr int ANIMATION_FPS = 25;

const bool ANIMATION_DRAFT_RUN = false;
const bool ANIMATION_RASTER_PREVIEW = false;

struct AnimationSegment
{
	int start;
	int end;
};

static int animationTimelineCursor = 0;

static AnimationSegment addSegment(float seconds)
{
	AnimationSegment segment;
	segment.start = animationTimelineCursor;
	segment.end = animationTimelineCursor + static_cast<int>(seconds * ANIMATION_FPS);
	animationTimelineCursor = segment.end;
	return segment;
}

const AnimationSegment WIREFRAME_FLY_IN = addSegment(4.0f);
const AnimationSegment RASTER_ORBIT = addSegment(7.0f);
const AnimationSegment PAN_AND_TILT = addSegment(4.0f);
const AnimationSegment FLAT_RAY_TRACE = addSegment(3.0f);
const AnimationSegment GOURAUD_SHADING = addSegment(3.0f);
const AnimationSegment PHONG_SHADING = addSegment(3.0f);
const AnimationSegment HARD_TO_SOFT_SHADOWS = addSegment(4.5f);
const AnimationSegment MIRROR_REVEAL = addSegment(4.0f);
const AnimationSegment GLOBAL_PHOTONS = addSegment(4.5f);
const AnimationSegment GLOBAL_PHOTON_DEBUG = addSegment(2.0f);
const AnimationSegment IRON_METAL = addSegment(1.5f);
const AnimationSegment ROUGHNESS_SWEEP = addSegment(3.0f);
const AnimationSegment GOLD_AND_ALUMINIUM = addSegment(3.0f);
const AnimationSegment ENVIRONMENT_MAP = addSegment(6.0f);
const AnimationSegment GLASS_REVEAL = addSegment(2.5f);
const AnimationSegment WATER_REVEAL = addSegment(2.0f);
const AnimationSegment ABSORPTION_FADE = addSegment(3.0f);
const AnimationSegment CAUSTICS = addSegment(3.0f);
const AnimationSegment CAUSTIC_DEBUG = addSegment(2.0f);
const AnimationSegment FOCUS_PULL = addSegment(4.5f);
const AnimationSegment SWIRL_BOKEH = addSegment(2.5f);
const AnimationSegment RING_BOKEH = addSegment(2.5f);
const AnimationSegment ENDING = addSegment(3.0f);
const int ANIMATION_END_TICK = animationTimelineCursor;

const AnimationSegment SHADING_CLOSE_UP = {GOURAUD_SHADING.start, PHONG_SHADING.end};

const glm::vec3 SCENE_CENTRE(0.0f, 0.0f, 0.0f);
const glm::vec3 WIDE_POSITION(0.0f, 0.0f, 6.0f);
const glm::vec3 SPHERE_CENTRE(0.58f, 0.58f, -0.61f);
const glm::vec3 BUNNY_CENTRE(0.32f, -0.10f, 0.49f);
const glm::vec3 MIRROR_CENTRE(-0.23f, -0.38f, 0.03f);
const glm::vec3 SPHERE_SHADOW_AREA(0.60f, 0.30f, -0.90f);
const glm::vec3 MAGENTA_CORNER(-0.60f, -0.40f, -0.40f);
const glm::vec3 CAUSTIC_AREA(0.80f, 0.35f, -0.90f);

static bool inSegment(const AnimationSegment &segment)
{
	return animationTick >= segment.start && animationTick < segment.end;
}

static bool segmentStarts(const AnimationSegment &segment)
{
	return animationTick == segment.start;
}

static float segmentProgress(const AnimationSegment &segment)
{
	const int length = segment.end - segment.start;
	if (length <= 1)
	{
		return 1.0f;
	}
	const float t = static_cast<float>(animationTick - segment.start) / static_cast<float>(length - 1);
	return glm::clamp(t, 0.0f, 1.0f);
}

static float easedProgress(const AnimationSegment &segment)
{
	const float t = segmentProgress(segment);
	return t * t * (3.0f - 2.0f * t);
}

static glm::vec3 currentCameraTarget(0.0f, 0.0f, 0.0f);
static glm::vec3 shotStartPosition(0.0f, 0.0f, 6.0f);
static glm::vec3 shotStartTarget(0.0f, 0.0f, 0.0f);

static void placeCamera(const glm::vec3 &position, const glm::vec3 &target)
{
	cameraPosition = position;
	currentCameraTarget = target;
	lookAt(cameraPosition, cameraOrientation, target);
}

static void placeCameraYawPitch(const glm::vec3 &position, float yawDegrees, float pitchDegrees)
{
	const float yaw = glm::radians(yawDegrees);
	const float pitch = glm::radians(pitchDegrees);
	const glm::vec3 forward(std::sin(yaw) * std::cos(pitch),
							std::sin(pitch),
							-std::cos(yaw) * std::cos(pitch));
	placeCamera(position, position + 6.0f * forward);
}

static void captureShotStart(const AnimationSegment &segment)
{
	if (segmentStarts(segment))
	{
		shotStartPosition = cameraPosition;
		shotStartTarget = currentCameraTarget;
	}
}

static void moveCameraTo(const AnimationSegment &segment, const glm::vec3 &position, const glm::vec3 &target, float travelSeconds)
{
	captureShotStart(segment);
	const float travelTicks = travelSeconds * ANIMATION_FPS;
	float t = 1.0f;
	if (travelTicks > 0.0f)
	{
		t = glm::clamp(static_cast<float>(animationTick - segment.start) / travelTicks, 0.0f, 1.0f);
	}
	t = t * t * (3.0f - 2.0f * t);
	placeCamera(glm::mix(shotStartPosition, position, t), glm::mix(shotStartTarget, target, t));
}

static void followCameraPath(const AnimationSegment &segment, const glm::vec3 &pathPosition, const glm::vec3 &pathTarget, float blendSeconds)
{
	captureShotStart(segment);
	float blend = glm::clamp(static_cast<float>(animationTick - segment.start) / (blendSeconds * ANIMATION_FPS), 0.0f, 1.0f);
	blend = blend * blend * (3.0f - 2.0f * blend);
	placeCamera(glm::mix(shotStartPosition, pathPosition, blend), glm::mix(shotStartTarget, pathTarget, blend));
}

static void resetAnimationState()
{

	applyMaterialToObject("sphere", "Red");
	applyMaterialToObject("Bunny", "Grey");
	applyMaterialToObject("short_box", "Red");
	applyMaterialToObject("tall_box", "Blue");
	applyMaterialToObject("tall_box_mirror", "Mirror");
	applyMaterialToObject("left_wall", "Magenta");
	applyMaterialToObject("right_wall", "Yellow");
	applyMaterialToObject("ceiling", "Cyan");
	applyMaterialToObject("back_wall", "White");

	GouraudShadingEnabled = false;
	PhongShadingEnabled = false;
	softShadowsEnabled = false;
	worldMirroringEnabled = false;
	worldRefractiveEnabled = false;
	worldMetallicEnabled = false;
	environmentMapEnabled = false;
	environmentMapSeenByCamera = false;
	photonMappingEnabled = false;
	photonDebuggingEnabled = false;
	depthOfFieldEnabled = false;
	swirlEnabled = false;
	reflexLensEnabled = false;
	isSphereShowing = true;
	isBlueBoxShowing = true;
	isRedBoxShowing = true;
	isBunnyShowing = true;

	isOrbiting = 0.0f;
	isLookAtEnabled = false;

	raysPerPixel = 4;
	focalDistance = 6.0f;
	waterTime = 0.0f;

	currentRenderer = &pointCloudRenderer;
	WireframeBool = true;
	forceRayTraceRedraw = true;
}

void updateAnimation()
{
	if (!isAnimating)
	{
		return;
	}

	waterTime = static_cast<float>(animationTick) / ANIMATION_FPS;

	if (animationTick == 0)
	{
		resetAnimationState();
		savingFrames = true;
		frameCounter = 0;
	}

	if (animationTick == 1 && !ANIMATION_DRAFT_RUN)
	{
		frameCounter = 0;
		savingFrames = true;
	}

	if (animationTick >= ANIMATION_END_TICK)
	{
		isAnimating = false;
		savingFrames = false;
		print("Animation finished: " + std::to_string(frameCounter) + " frames saved");
		return;
	}

	if (inSegment(WIREFRAME_FLY_IN))
	{
		placeCamera(glm::mix(glm::vec3(0.0f, 0.0f, 18.0f), WIDE_POSITION, easedProgress(WIREFRAME_FLY_IN)), SCENE_CENTRE);
		WireframeBool = segmentProgress(WIREFRAME_FLY_IN) < 0.6f;
	}

	if (inSegment(RASTER_ORBIT))
	{
		WireframeBool = false;
		const float angle = glm::radians(360.0f * easedProgress(RASTER_ORBIT));
		placeCamera(glm::vec3(6.0f * std::sin(angle), 0.0f, 6.0f * std::cos(angle)), SCENE_CENTRE);
	}

	if (inSegment(PAN_AND_TILT))
	{
		const float t = segmentProgress(PAN_AND_TILT);
		if (t < 0.5f)
		{
			placeCameraYawPitch(WIDE_POSITION, 15.0f * std::sin(2.0f * MAIN_PI * (t * 2.0f)), 0.0f);
		}
		else
		{
			placeCameraYawPitch(WIDE_POSITION, 0.0f, 10.0f * std::sin(2.0f * MAIN_PI * ((t - 0.5f) * 2.0f)));
		}
	}

	if (segmentStarts(FLAT_RAY_TRACE))
	{
		currentRenderer = &rayTracedRenderer;
		GouraudShadingEnabled = false;
		PhongShadingEnabled = false;
		forceRayTraceRedraw = true;
	}
	if (inSegment(FLAT_RAY_TRACE))
	{
		moveCameraTo(FLAT_RAY_TRACE, glm::vec3(0.25f, 0.45f, 3.2f), SPHERE_CENTRE, 3.0f);
	}

	if (segmentStarts(GOURAUD_SHADING))
	{
		GouraudShadingEnabled = true;
		PhongShadingEnabled = false;
	}
	if (segmentStarts(PHONG_SHADING))
	{
		GouraudShadingEnabled = false;
		PhongShadingEnabled = true;
	}
	if (inSegment(SHADING_CLOSE_UP))
	{
		moveCameraTo(SHADING_CLOSE_UP, glm::vec3(0.30f, 0.50f, 2.7f), SPHERE_CENTRE, 6.0f);
	}

	if (inSegment(HARD_TO_SOFT_SHADOWS))
	{
		moveCameraTo(HARD_TO_SOFT_SHADOWS, glm::vec3(0.0f, 0.25f, 3.2f), SPHERE_SHADOW_AREA, 1.5f);
		softShadowsEnabled = segmentProgress(HARD_TO_SOFT_SHADOWS) >= 0.5f;
	}

	if (inSegment(MIRROR_REVEAL))
	{
		moveCameraTo(MIRROR_REVEAL, glm::vec3(0.75f, 0.30f, 2.6f), MIRROR_CENTRE, 2.0f);
		worldMirroringEnabled = segmentProgress(MIRROR_REVEAL) >= 0.5f;
	}

	if (inSegment(GLOBAL_PHOTONS))
	{
		moveCameraTo(GLOBAL_PHOTONS, glm::vec3(0.50f, 0.20f, 3.0f), MAGENTA_CORNER, 1.5f);
		photonMappingEnabled = segmentProgress(GLOBAL_PHOTONS) >= 0.5f;
	}

	if (inSegment(GLOBAL_PHOTON_DEBUG))
	{
		photonDebuggingEnabled = true;
	}

	if (segmentStarts(IRON_METAL))
	{
		photonDebuggingEnabled = false;
		photonMappingEnabled = false;
		softShadowsEnabled = false;
		worldMetallicEnabled = true;
		applyMaterialToObject("Bunny", "Iron");
		setRoughnessForObject("Bunny", 0.05f);
	}
	if (inSegment(IRON_METAL))
	{
		moveCameraTo(IRON_METAL, glm::vec3(0.55f, 0.05f, 2.2f), BUNNY_CENTRE, 1.5f);
	}

	if (inSegment(ROUGHNESS_SWEEP))
	{
		setRoughnessForObject("Bunny", glm::mix(0.05f, 0.8f, segmentProgress(ROUGHNESS_SWEEP)));
	}

	if (segmentStarts(GOLD_AND_ALUMINIUM))
	{
		applyMaterialToObject("Bunny", "Gold");
		applyMaterialToObject("short_box", "Aluminium");
	}
	if (inSegment(GOLD_AND_ALUMINIUM))
	{
		moveCameraTo(GOLD_AND_ALUMINIUM, glm::vec3(0.50f, 0.20f, 3.0f), glm::vec3(0.32f, -0.35f, 0.40f), 2.0f);
	}

	if (segmentStarts(ENVIRONMENT_MAP))
	{
		environmentMapEnabled = true;
	}
	if (inSegment(ENVIRONMENT_MAP))
	{
		const float angle = glm::radians(glm::mix(-35.0f, 35.0f, easedProgress(ENVIRONMENT_MAP)));
		followCameraPath(ENVIRONMENT_MAP, glm::vec3(4.5f * std::sin(angle), 0.3f, 4.5f * std::cos(angle)), SCENE_CENTRE, 1.5f);

		environmentMapSeenByCamera = segmentProgress(ENVIRONMENT_MAP) >= 0.5f;
	}

	if (segmentStarts(GLASS_REVEAL))
	{
		environmentMapSeenByCamera = false;
		worldRefractiveEnabled = true;
		applyMaterialToObject("sphere", "Glass");
		applyMaterialToObject("short_box", "Glass");
	}
	if (inSegment(GLASS_REVEAL))
	{
		moveCameraTo(GLASS_REVEAL, glm::vec3(0.05f, 0.15f, 3.4f), glm::vec3(0.45f, 0.10f, -0.20f), 2.5f);
	}

	if (segmentStarts(WATER_REVEAL))
	{
		applyMaterialToObject("short_box", "Water");
		setAbsorptionForObject("short_box", glm::vec3(0.0f));
	}
	if (inSegment(WATER_REVEAL))
	{
	}

	if (inSegment(ABSORPTION_FADE))
	{
		setAbsorptionForObject("short_box", glm::mix(glm::vec3(0.0f), glm::vec3(0.90f, 0.25f, 0.15f), segmentProgress(ABSORPTION_FADE)));
	}

	if (segmentStarts(CAUSTICS))
	{
		photonMappingEnabled = true;
	}
	if (inSegment(CAUSTICS))
	{
		moveCameraTo(CAUSTICS, glm::vec3(-0.10f, 0.30f, 2.8f), CAUSTIC_AREA, 2.0f);
	}

	if (inSegment(CAUSTIC_DEBUG))
	{
		photonDebuggingEnabled = true;
	}

	if (segmentStarts(FOCUS_PULL))
	{
		photonDebuggingEnabled = false;
		photonMappingEnabled = false;
		depthOfFieldEnabled = true;
		swirlEnabled = false;
		reflexLensEnabled = false;
		raysPerPixel = 16;
	}
	if (inSegment(FOCUS_PULL))
	{
		moveCameraTo(FOCUS_PULL, WIDE_POSITION, SCENE_CENTRE, 1.5f);

		const float travelTicks = 1.5f * ANIMATION_FPS;
		const float pullTicks = static_cast<float>(FOCUS_PULL.end - FOCUS_PULL.start) - travelTicks;
		float pull = glm::clamp((static_cast<float>(animationTick - FOCUS_PULL.start) - travelTicks) / pullTicks, 0.0f, 1.0f);
		pull = pull * pull * (3.0f - 2.0f * pull);
		focalDistance = glm::mix(5.3f, 7.0f, pull);
	}

	if (segmentStarts(SWIRL_BOKEH))
	{
		swirlEnabled = true;
		reflexLensEnabled = false;
		focalDistance = 5.5f;
	}

	if (segmentStarts(RING_BOKEH))
	{
		swirlEnabled = false;
		reflexLensEnabled = true;
		focalDistance = 6.6f;
	}

	if (segmentStarts(ENDING))
	{
		depthOfFieldEnabled = false;
		swirlEnabled = false;
		reflexLensEnabled = false;
		raysPerPixel = 4;
		currentRenderer = &pointCloudRenderer;
		WireframeBool = false;
	}
	if (inSegment(ENDING))
	{
		placeCamera(glm::mix(WIDE_POSITION, glm::vec3(0.0f, 0.0f, 6.0f), easedProgress(ENDING)), SCENE_CENTRE);
	}
	if (ANIMATION_RASTER_PREVIEW)
	{
		currentRenderer = &pointCloudRenderer;
		if (!inSegment(WIREFRAME_FLY_IN))
		{
			WireframeBool = false;
		}
	}

	animationTick++;
}

// Parse the triangles from given OBJ files
std::vector<ModelTriangle> triangleParseOBJ(const std::string &filename, float scalingFactor, const glm::vec3 &translation = glm::vec3(0.0f), const glm::mat3 &rotation = glm::mat3(1.0f))
{

	// Vector of Triangles
	std::vector<ModelTriangle> modelTriangles;
	// Vector of Vertices
	std::vector<glm::vec3> vertices;

	// Vector of texture vertices, for cobbles
	std::vector<glm::vec2> textureVertices;

	// ifstream
	std::ifstream file(filename);
	// Check working
	if (!file.is_open())
	{
		return modelTriangles;
	}

	// string = to line
	std::string line;

	// Keep track of the current texture index for the triangles
	int currentTextureIndex = -1;

	// Default Colour if doesn't work
	Colour colour(255, 255, 255);

	// Keep track of the current material name for the triangles
	std::string currentMaterialName;

	// Default properties
	MaterialProperties currentMaterialProperties = {
		false, false, false, false, 0.0f, 1.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f)};

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

		// For vertex
		if (prefix == "v" && tokens.size() >= 4)
		{
			// Push back and scaling
			vertices.push_back(rotation * glm::vec3(std::stof(tokens[1]) * scalingFactor, std::stof(tokens[2]) * scalingFactor, std::stof(tokens[3]) * scalingFactor) + translation);
		}
		// Vertex textures
		else if (prefix == "vt" && tokens.size() >= 3)
		{
			// Push back texture vertex
			textureVertices.push_back(glm::vec2(
				std::stof(tokens[1]),
				std::stof(tokens[2])));
		}
		// Assign the object name
		else if (prefix == "o" && tokens.size() >= 2)
		{
			currentMaterialName = tokens[1];
		}
		// Faces
		else if (prefix == "f" && tokens.size() >= 4)
		{
			// Parse the vertex indices and texture indices
			int index[3];
			int textureIndex[3];
			for (int i = 0; i < 3; i++)
			{
				// Split the face token by '/' to separate vertex and texture indices
				std::vector<std::string> parts = split(tokens[i + 1], '/');
				// Convert the vertex index to an integer and adjust for 0-based indexing
				index[i] = std::stoi(parts[0]) - 1;
				// Convert the texture index to an integer and adjust for 0-based indexing, if it exists; otherwise, set to -1
				if (parts.size() > 1 && !parts[1].empty())
				{
					textureIndex[i] = std::stoi(parts[1]) - 1;
				}
				else
				{
					textureIndex[i] = -1;
				}
			}
			ModelTriangle triangle(vertices[index[0]], vertices[index[1]], vertices[index[2]], colour);

			// Set the material properties for the triangle based on the current material
			triangle.isMirror = currentMaterialProperties.isMirror;
			triangle.isWater = currentMaterialProperties.isWater;
			triangle.isRefractive = currentMaterialProperties.isRefractive;
			triangle.isMetallic = currentMaterialProperties.isMetallic;
			triangle.absorption = currentMaterialProperties.absorption;
			triangle.fresnelReflectance = currentMaterialProperties.fresnelReflectance;
			triangle.roughness = currentMaterialProperties.roughness;
			triangle.stretchFactor = currentMaterialProperties.stretchFactor;

			// Only set the refractive index if the triangle is refractive
			if (triangle.isRefractive)
			{
				triangle.refractiveIndex = currentMaterialProperties.refractiveIndex;
			}
			// If there is a texture
			if (currentTextureIndex >= 0)
			{
				bool textureIsValid = true;
				for (int i = 0; i < 3; i++)
				{
					// if the texture index is invalid, set valid to false
					if (textureIndex[i] < 0 || textureIndex[i] >= static_cast<int>(textureVertices.size()))
					{
						textureIsValid = false;
					}
				}
				if (textureIsValid)
				{
					// Get the texture map for the current material
					const TextureMap &texture = textures[currentTextureIndex];
					for (int i = 0; i < 3; i++)
					{
						// Get the texture coordinates (u, v) for the vertex
						float u = textureVertices[textureIndex[i]].x;
						float v = textureVertices[textureIndex[i]].y;
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
		// For materials
		else if (prefix == "usemtl" && tokens.size() >= 2)
		{
			// Look up the material properties for the current material name
			auto material = materialProperties.find(tokens[1]);
			if (material != materialProperties.end())
			{
				currentMaterialProperties = material->second;
			}
			else // Not in map
			{
				// Default
				currentMaterialProperties = MaterialProperties{
					false, false, false, false, 0.0f, 1.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f)};
			}

			// Check if the material name exists in map
			auto texture = textureIndexByMaterial.find(tokens[1]);
			// If it exists, set currentTextureIndex to the corresponding index
			if (texture != textureIndexByMaterial.end())
			{
				currentTextureIndex = texture->second;
			}
			else // Not in map
			{
				currentTextureIndex = -1;
			}

			// Check if the material name exists in the paletteMap
			auto it = paletteMap.find(tokens[1]);
			// If it exists, set the colour to the corresponding Colour
			if (it != paletteMap.end())
			{
				colour = it->second;
			}
			else
			{
				colour = Colour(255, 255, 255); // don't inherit the previous material
			}
		}
	}

	return modelTriangles;
}

// Array of vertex normals of size 3
std::array<glm::vec3, 3> vertexNormals(const ModelTriangle &triangle)
{
	// Change the smooth based on object
	// Sphere averages all faces
	// Others only average within 45 degrees
	float smoothThreshold = std::cos(glm::radians(45.0f));

	// Sphere always smooths out
	if (triangle.objectName == "sphere" || triangle.objectName == "Bunny")
	{
		smoothThreshold = -1.0f;
	}

	std::array<glm::vec3, 3> normals;

	// Loop over the three vertices
	for (int i = 0; i < 3; ++i)
	{
		// Assign vertices
		glm::vec3 vertex = triangle.vertices[i];
		glm::vec3 normalSum(0.0f);

		// Loop through all triangles to find those that share the vertex
		for (const ModelTriangle &other : triangles)
		{
			if (other.vertices[0] != vertex && other.vertices[1] != vertex && other.vertices[2] != vertex)
			{
				continue;
			}

			// Calculate the normal of the other triangle
			glm::vec3 edge1 = other.vertices[1] - other.vertices[0];
			glm::vec3 edg2 = other.vertices[2] - other.vertices[0];
			glm::vec3 otherNormal = glm::normalize(glm::cross(edge1, edg2));

			// Check if the angle between the normals is within the smooth threshold
			// If the dot product is greater than the threshold, they are considered smooth
			if (glm::dot(otherNormal, triangle.normal) > smoothThreshold)
			{
				normalSum += otherNormal;
			}
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
	{
		return colours;
	}

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

		// Reading texture maps for materials
		else if (prefix == "map_Kd" && tokens.size() >= 2)
		{
			std::string texturePath = "src/" + tokens[1];
			loadTexture(colourName, texturePath);
		}
	}
	// Return vectors
	return colours;
}

int main(int argc, char *argv[])
{

	// Load in textures
	loadTexture("Environment", "src/environment.ppm");

	// Get the environment map and its dimensions
	environmentMap = textures[textureIndexByMaterial["Environment"]];

	// all obj files in here
	std::vector<std::string> filenNames = {
		"src/textured-cornell-box",
		"src/sphere",
		"src/Bunny-LowPoly",
	};
	std::string cornellBoxFilename = filenNames[0]; // Use the first filename
	// Get the colour pallete
	palette = colourParseOBJ(cornellBoxFilename + ".mtl");
	// Add it all to a Dictionary from a vector
	for (int i = 0; i < palette.size(); i++)
	{
		paletteMap[palette[i].name] = palette[i];
	}
	registerPaletteColoursAsMaterials();

	// Parse the triangles
	for (size_t i = 0; i < filenNames.size(); ++i)
	{
		// Set the scaling factor, translation, and rotation based on the index of the filename
		float scalingFactor = 0.35f;
		glm::vec3 translation(0.0f);
		glm::mat3 rotation(1.0f);

		// For the sphere
		if (i == 1)
		{
			translation = glm::vec3(0.58f, 0.055f, -0.96f);
		}
		// For the bunny use approximates based on looks
		else if (i == 2)
		{
			// Approximate scaling factor
			scalingFactor = 0.005f;

			// Approximate translation
			translation = glm::vec3(0.17f, -0.4f, 0.50f);
			rotation = glm::mat3(
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			// Should end up on top of the small_box
		}
		// Parse all the triangles
		std::vector<ModelTriangle> parsedTriangles = triangleParseOBJ(filenNames[i] + ".obj", scalingFactor, translation, rotation);

		// Insert into the main triangles section
		triangles.insert(triangles.end(), parsedTriangles.begin(), parsedTriangles.end());
	}

	// Clear and reserve vector
	vertexNormalsByTriangle.clear();
	vertexNormalsByTriangle.reserve(triangles.size());

	glm::vec3 boxMin(std::numeric_limits<float>::max());
	glm::vec3 boxMax(std::numeric_limits<float>::lowest());

	// Loop through triangles and find the vertex normals
	// Also find the min and max of the light source
	for (const ModelTriangle &triangle : triangles)
	{
		vertexNormalsByTriangle.push_back(vertexNormals(triangle));

		// Check if the triangle belongs to the light source
		if (triangle.objectName == "light")
		{
			for (const glm::vec3 &vertex : triangle.vertices)
			{
				boxMin = glm::min(boxMin, vertex);
				boxMax = glm::max(boxMax, vertex);
			}
		}
	}

	// Define light position
	// Stop self shadowing by moving the light down a bit
	lightMin = boxMin;
	lightMax = boxMax;
	lightPosition = (boxMin + boxMax) * 0.5f;

	SDL_Event event;

	glm::vec3 tempCameraPosition = cameraPosition;
	glm::mat3 tempCameraOrientation = cameraOrientation;

	const int RESUME_FROM_FRAME = 687; // e.g. 812 if frames 000000-000811 are already saved

	if (RESUME_FROM_FRAME > 0)
	{
		isAnimating = true;
		animationTick = 0;
		for (int tick = 0; tick <= RESUME_FROM_FRAME; tick++)
		{
			updateAnimation(); // sets the camera, materials and switches exactly as the full run would
		}
		frameCounter = RESUME_FROM_FRAME;
		savingFrames = !ANIMATION_DRAFT_RUN;
	}

	while (true)
	{
		// We MUST poll for events - otherwise the window will freeze !
		while (window.pollForInputEvents(event))
		{
			handleEvent(event, window);
		}

		newFrameRendered = (currentRenderer != &rayTracedRenderer);
		// In C++ we can't use "." notation for dynamic dispatch, so we have to use -> instead :o(
		currentRenderer->draw(window);

		if (isOrbiting != 0)
		{
			rotateY(cameraPosition, cameraOrientation, isOrbiting);
		}
		if (isLookAtEnabled)
		{
			lookAt(cameraPosition, cameraOrientation, glm::vec3(0.0f));
		}
		// Need to ask the SDL window to render the frame, otherwise nothing actually gets shown on the screen !
		window.renderFrame();

		updateAnimation();
		// Bool to check if the camera has changed since the last frame
		bool cameraChanged = (tempCameraPosition != cameraPosition || tempCameraOrientation != cameraOrientation);
		// If we are saving frames, a new frame has been rendered, and either the camera has changed or the current renderer is the ray traced renderer, then save the frame to a file
		if (savingFrames && newFrameRendered && (isAnimating || cameraChanged || currentRenderer == &rayTracedRenderer))
		{
			std::string paddedCounter = std::to_string(frameCounter);
			while (paddedCounter.length() < 6)
			{
				paddedCounter = "0" + paddedCounter;
			}
			// Use whichever image file format works best on your platform (comment out the one you don't need)
			window.savePPM("saved-frames", paddedCounter + ".ppm");
			// window.saveBMP("saved-frames", paddedCounter + ".bmp");
			frameCounter++;
			waterTime += 1.0f / 25.0f;
			forceRayTraceRedraw = true;
		}
		tempCameraPosition = cameraPosition;
		tempCameraOrientation = cameraOrientation;
	}
}
