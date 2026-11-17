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
extern const int WIDTH = 640;
extern const int HEIGHT = 480;

// Bool to check whether we want stroked triangle view
extern bool WireframeBool;

// Orbiting int and LookAt bool
float isOrbiting = 0; // 0 = not orbiting, 1 = clockwise, -1 = counterclockwise
bool isLookAtEnabled = false;

// Bool to check if we need to redraw
bool forceRayTraceRedraw = false;

// Bool for different shading techniques
bool GouraudShadingEnabled = false;
bool PhongShadingEnabled = false;

// Bool to check if the sphere and boxes are showing
bool isSphereShowing = true;
bool isBlueBoxShowing = true;
bool isRedBoxShowing = true;

// Flag to enable/disable depth of field effect
bool depthOfFieldEnabled = false;
// Flag to enable/disable swirl effect
bool swirlEnabled = false;
// Flag to enable/disable reflex lens effect
bool reflexLensEnabled = false;

// Flag to enable/disable world mirroring
bool worldMirroringEnabled = false;

// Flag to enable/disable world refractive materials
bool worldRefractiveEnabled = false;

// Flag to enable/disable world metallic materials
bool worldMetallicEnabled = false;

bool environmentMapEnabled = false;
bool environmentMapSeenByCamera = false;

float waterTime = 0.0f;

// Flag to enable/disable soft shadows
bool softShadowsEnabled = false;

// Light position and min/max values for light calculations
glm::vec3 lightMin(0.0f);
glm::vec3 lightMax(0.0f);

// Texture maps
TextureMap environmentMap;
std::vector<TextureMap> textures;
std::unordered_map<std::string, int> textureIndexByMaterial;

int environmentTextureWidth = 0;
int environmentTextureHeight = 0;

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

bool savingFrames = false;
int frameCounter = 0;
bool newFrameRendered = false;

// Filenames
std::string objFilename = "/Assets/textured-cornell-box";
std::string objFilenameSphere = "/Assets/sphere";

// Core 3D rendering data structures (for when we eventually get around to working in 3D ;o)
float focalLength = 2.0f;
float focalDistance = 4.0f;
float lenseSize = 0.13f;
int raysPerPixel = 1;

glm::vec3 cameraPosition(0.0, 0.0, 6.0);
glm::mat3 cameraOrientation = glm::mat3(1.0f);
glm::vec3 lightPosition(0.0, 0.8, 0.0);

bool isAnimating = false;
int animationTick = 0;

// PI so i dont have to call a library
const double PI = 3.14159265358979323846;

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
	bool isMirror;
	bool isWater;
	bool isRefractive;
	bool isMetallic;
	float roughness;
	float refractiveIndex;
	glm::vec3 fresnelReflectance;
	glm::vec3 absorption;
};

// Define a map to associate material names with their properties
const std::unordered_map<std::string, MaterialProperties> materialProperties = {
	{"Mirror", {true, false, false, false, 0.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f)}},
	{"Glass", {false, false, true, false, 0.0f, 1.5f, glm::vec3(0.0f), glm::vec3(0.35f, 0.10f, 0.25f)}},
	{"Water", {false, true, true, false, 0.0f, 1.333f, glm::vec3(0.0f), glm::vec3(0.90f, 0.25f, 0.15f)}},
	{"Gold", {false, false, false, true, 0.15f, 1.0f, glm::vec3(1.00f, 0.71f, 0.29f), glm::vec3(0.0f)}},
	{"Silver", {false, false, false, true, 0.05f, 1.0f, glm::vec3(0.95f, 0.93f, 0.88f), glm::vec3(0.0f)}},
	{"Copper", {false, false, false, true, 0.33f, 1.0f, glm::vec3(0.95f, 0.64f, 0.54f), glm::vec3(0.0f)}},
	{"Aluminium", {false, false, false, true, 0.53f, 1.0f, glm::vec3(0.91f, 0.92f, 0.92f), glm::vec3(0.0f)}},
	{"Iron", {false, false, false, true, 0.8f, 1.0f, glm::vec3(0.56f, 0.57f, 0.58f), glm::vec3(0.04f)}}};

void loadTexture(const std::string &materialName, const std::string &path)
{
	textures.push_back(TextureMap(path));
	textureIndexByMaterial[materialName] = static_cast<int>(textures.size()) - 1;
}

void lookAt(glm::vec3 &cameraPosition, glm::mat3 &cameraOrientation, const glm::vec3 &target)
{
	if (glm::length(cameraPosition - target) < 1e-5f)
		return;

	// Calculate the forward vector
	glm::vec3 forward = glm::normalize(cameraPosition - target);

	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

	if (std::abs(forward.y) > 0.999f)
		worldUp = glm::vec3(0.0f, 0.0f, 1.0f);

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
	double degreeInRadians = double(degrees) * (PI / 180.0);
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
	float degreeInRadians = degrees * (PI / 180.0);
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

void panCamera(glm::mat3 &cameraOrientation, float degrees)
{
	float radians = degrees * (PI / 180.0);
	float cosine = std::cos(radians);
	float sine = std::sin(radians);
	glm::mat3 rotationMatrix = glm::mat3(
		glm::vec3(cosine, 0.0f, -sine),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(sine, 0.0f, cosine));
	cameraOrientation = rotationMatrix * cameraOrientation;
}

void tiltCamera(glm::mat3 &cameraOrientation, int degrees)
{
	// Stop the camera from tilting too far up or down
	float forwardY = -cameraOrientation[2].y;
	if ((degrees > 0 && forwardY > 0.95f) || (degrees < 0 && forwardY < -0.95f))
		return;

	float radians = degrees * (PI / 180.0);
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

	const float MOVE_STEP = 0.2f;
	const glm::vec3 camRight = cameraOrientation[0];
	const glm::vec3 camUp = cameraOrientation[1];
	const glm::vec3 camForward = -cameraOrientation[2];
	switch (event.key.keysym.sym)
	{
	case SDLK_1:
		print("Switching to wireframe");
		currentRenderer = &pointCloudRenderer;
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
		forceRayTraceRedraw = true;
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
		print("Tilting camera down");
		tiltCamera(cameraOrientation, -6);
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
	case SDLK_o:
		print("Toggle soft shadows");
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
		break;
	case SDLK_BACKSPACE:
		print("Starting Animation");
		isAnimating = true;
		animationTick = 0;
		// Optional: Reset camera to starting position
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
		cameraOrientation = glm::mat3(1.0f);
		break;
	default:
		break;
	}
}
int ZoomInStartTime = 0;
int ZoomInEndTime = ZoomInStartTime + 480;
int ZoomOutStartTime = ZoomInEndTime;
int ZoomOutEndTime = ZoomOutStartTime + 120;
int BounceStartTime = ZoomOutEndTime;
int BounceEndTime = BounceStartTime + 120;
int WireFrameHoldStartTime = BounceEndTime;
int WireFrameHoldEndTime = WireFrameHoldStartTime + 400;
int WireFrameEndTime = WireFrameHoldEndTime;
int WireFrameBuffer = WireFrameEndTime + 50;
int OrbitStartTime = WireFrameBuffer + 60;
int OrbitBuffer = OrbitStartTime + 2;
int OrbitEndTime = OrbitBuffer + 1460;
int OrbitBuffer2 = OrbitEndTime + 420;
int PanStartTime = OrbitBuffer2;
int PanEndTime = PanStartTime + 400;
int PanBackStartTime = PanEndTime + 200;
int PanBackEndTime = PanBackStartTime + 400;
int GouraudStartTime = PanBackEndTime + 80;
int GouraudEndTime = GouraudStartTime + 120;
int PhongStartTime = GouraudEndTime;
int PhongEndTime = PhongStartTime + 120;
int softShadowsStartTime = PhongEndTime + 120;
int softShadowsEndTime = softShadowsStartTime + 240;
int PanBack2StartTime = softShadowsEndTime;
int PanBack2EndTime = PanBack2StartTime + 120;
int ZoomInStartTime2 = PanBack2EndTime;
int ZoomInEndTime2 = ZoomInStartTime2 + 400;
int ZoomOutStartTime2 = ZoomInEndTime2;
int ZoomOutEndTime2 = ZoomOutStartTime2 + 400;
int unPanCameraStartTime = ZoomOutEndTime2 + 320;
int unPanCameraEndTime = unPanCameraStartTime + 120;
int DepthOfFieldStartTime = unPanCameraEndTime;
int DepthOfFieldEndTime = DepthOfFieldStartTime + 100;
int SwirlStartTime = DepthOfFieldEndTime;
int SwirlEndTime = SwirlStartTime + 100;
int ReflexLensStartTime = SwirlEndTime;
int ReflexLensEndTime = ReflexLensStartTime + 100;
int FocusOnWallStartTime = ReflexLensEndTime;
int FocusOnWallEndTime = FocusOnWallStartTime + 100;

void updateAnimation()
{
	if (!isAnimating)
		return;

	// Phase 1: Orbit and switch to Phong shading for 60 ticks (frames)
	if (animationTick == 0)
	{
		currentRenderer = &pointCloudRenderer;
		WireframeBool = true;
		// window.toggleFullscreen();
		cameraPosition = glm::vec3(0.0f, 0.0f, 18.0f);
		cameraOrientation = glm::mat3(1.0f);
	}
	if (animationTick >= ZoomInStartTime && animationTick < ZoomInEndTime)
	{
		cameraPosition.z -= 0.025;
	}
	if (animationTick >= ZoomOutStartTime && animationTick < ZoomOutEndTime)
	{
		cameraPosition.z += 0.025;
	}
	if (animationTick >= BounceStartTime && animationTick < BounceEndTime)
	{
		cameraPosition.z -= 0.025;
	}
	if (animationTick >= WireFrameHoldStartTime && animationTick < WireFrameHoldEndTime)
	{
		WireframeBool = true;
	}
	if (animationTick >= WireFrameEndTime && animationTick < WireFrameBuffer)
	{
		WireframeBool = false;
	}
	if (animationTick >= OrbitStartTime && animationTick < OrbitBuffer)
	{
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
		isOrbiting = 0.25f;		// Start orbiting clockwise
		isLookAtEnabled = true; // Enable LookAt
	}
	if (animationTick >= OrbitEndTime && animationTick < OrbitBuffer2)
	{
		isOrbiting = 0;
		cameraPosition = glm::vec3(0.0f, 0.0f, 6.0f);
	}
	if (animationTick >= PanStartTime && animationTick < PanEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		isLookAtEnabled = false; // Disable LookAt
		panCamera(cameraOrientation, 0.035f);
	}
	if (animationTick >= PanBackStartTime && animationTick < PanBackEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		panCamera(cameraOrientation, -0.035f);
	}
	if (animationTick >= GouraudStartTime && animationTick < GouraudEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		GouraudShadingEnabled = true;
		cameraPosition.z -= 0.005;
	}
	if (animationTick >= PhongStartTime && animationTick < PhongEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		PhongShadingEnabled = true;
		GouraudShadingEnabled = false;
	}
	if (animationTick >= softShadowsStartTime && animationTick < softShadowsEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		PhongShadingEnabled = true;
		GouraudShadingEnabled = false;
		softShadowsEnabled = true;
	}
	if (animationTick >= PanBack2StartTime && animationTick < PanBack2EndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		softShadowsEnabled = false;
		panCamera(cameraOrientation, 0.02f);
	}
	if (animationTick >= ZoomInStartTime2 && animationTick < ZoomInEndTime2)
	{
		// currentRenderer = &rayTracedRenderer;
		cameraPosition.z -= 0.005;
	}
	if (animationTick >= ZoomOutStartTime2 && animationTick < ZoomOutEndTime2)
	{
		// currentRenderer = &rayTracedRenderer;
		cameraPosition.z += 0.005;
	}
	if (animationTick >= unPanCameraStartTime && animationTick < unPanCameraEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		panCamera(cameraOrientation, -0.02f);
	}
	if (animationTick >= DepthOfFieldStartTime && animationTick < DepthOfFieldEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		depthOfFieldEnabled = true;
		rayTracedRenderer.focusOnPixel(411, 151);
	}
	if (animationTick >= SwirlStartTime && animationTick < SwirlEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		swirlEnabled = true;
		rayTracedRenderer.focusOnPixel(273, 293);
	}
	if (animationTick >= ReflexLensStartTime && animationTick < ReflexLensEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		swirlEnabled = false;
		reflexLensEnabled = true;
		rayTracedRenderer.focusOnPixel(375, 365);
	}
	if (animationTick >= FocusOnWallStartTime && animationTick < FocusOnWallEndTime)
	{
		// currentRenderer = &rayTracedRenderer;
		reflexLensEnabled = false;
		rayTracedRenderer.focusOnPixel(313, 231);
	}

	animationTick++;
}

std::vector<ModelTriangle> triangleParseOBJ(const std::string &filename, float scalingFactor, const glm::vec3 &translation = glm::vec3(0.0f), const glm::mat3 &rotation = glm::mat3(1.0f))
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

	MaterialProperties currentMaterialProperties = {
		false, false, false, false, 0.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f)};

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
			vertices.push_back(rotation * glm::vec3(std::stof(tokens[1]) * scalingFactor, std::stof(tokens[2]) * scalingFactor, std::stof(tokens[3]) * scalingFactor) + translation);
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
			ModelTriangle triangle(vertices[idx[0]], vertices[idx[1]], vertices[idx[2]], colour);

			// Set the material properties for the triangle based on the current material
			triangle.isMirror = currentMaterialProperties.isMirror;
			triangle.isWater = currentMaterialProperties.isWater;
			triangle.isRefractive = currentMaterialProperties.isRefractive;
			triangle.isMetallic = currentMaterialProperties.isMetallic;
			triangle.absorption = currentMaterialProperties.absorption;
			triangle.fresnelReflectance = currentMaterialProperties.fresnelReflectance;
			triangle.roughness = currentMaterialProperties.roughness;

			// Only set the refractive index if the triangle is refractive
			if (triangle.isRefractive)
			{
				triangle.refractiveIndex = currentMaterialProperties.refractiveIndex;
			}
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
			// Look up the material properties for the current material name
			auto material = materialProperties.find(tokens[1]);
			// If the material name exists in the map, set currentMaterialProperties to the corresponding properties; otherwise, set it to default values
			currentMaterialProperties = material != materialProperties.end()
											? material->second
											: MaterialProperties{
												  false, false, false, false, 0.0f, 1.0f,
												  glm::vec3(0.0f), glm::vec3(0.0f)};

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
			else
			{
				colour = Colour(255, 255, 255); // don't inherit the previous material
			}
		}
	}

	return modelTriangles;
}

std::array<glm::vec3, 3> vertexNormals(const ModelTriangle &triangle)
{
	float smoothThreshold = triangle.objectName == "sphere"
								? -1.0f
								: std::cos(glm::radians(45.0f));
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
	// Load in textures
	loadTexture("Cobbles", "../../../05 Navigation and Transformation/models/texture (1).ppm");
	loadTexture("Environment", "../../../05 Navigation and Transformation/models/environment2.ppm");

	// Get the environment map and its dimensions
	environmentMap = textures[textureIndexByMaterial["Environment"]];

	std::vector<std::string> filennames = {
		"../../../05 Navigation and Transformation/models/textured-cornell-box",
		"../../../07 Lighting and Shading/resources/sphere",
		"../../../05 Navigation and Transformation/models/Bunny-LowPoly",
	};
	std::string cornellBoxFilename = filennames[0]; // Use the first filename
	// Get the colour pallete
	palette = colourParseOBJ(cornellBoxFilename + ".mtl");
	// Add it all to a Dictionary from a vector
	for (int i = 0; i < palette.size(); i++)
	{
		paletteMap[palette[i].name] = palette[i];
	}

	// Parse the triangles
	for (size_t i = 0; i < filennames.size(); ++i)
	{
		float scalingFactor = 0.35f;
		glm::vec3 translation(0.0f);
		glm::mat3 rotation(1.0f);

		if (i == 1)
		{
			translation = glm::vec3(0.58f, 0.055f, -0.96f);
		}
		else if (i == 2)
		{
			scalingFactor = 0.005f;
			translation = glm::vec3(0.17f, -0.4f, 0.50f);
			rotation = glm::mat3(
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
		}
		std::vector<ModelTriangle> parsedTriangles = triangleParseOBJ(
			filennames[i] + ".obj", scalingFactor, translation, rotation);
		triangles.insert(triangles.end(), parsedTriangles.begin(), parsedTriangles.end());
	}

	// Clear the vector? Given by intellisense so im going to trust it
	vertexNormalsByTriangle.clear();
	// Reserve the space, also given by intellisense
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
	lightPosition.y -= 0.01f;

	SDL_Event event;

	glm::vec3 tempCameraPosition = cameraPosition;
	glm::mat3 tempCameraOrientation = cameraOrientation;

	while (true)
	{
		// We MUST poll for events - otherwise the window will freeze !
		while (window.pollForInputEvents(event))
			handleEvent(event, window);

		newFrameRendered = (currentRenderer != &rayTracedRenderer);
		// In C++ we can't use "." notation for dynamic dispatch, so we have to use -> instead :o(
		currentRenderer->draw(window);

		if (isOrbiting != 0)
			rotateY(cameraPosition, cameraOrientation, isOrbiting);
		if (isLookAtEnabled)
			lookAt(cameraPosition, cameraOrientation, glm::vec3(0.0f));
		// Need to ask the SDL window to render the frame, otherwise nothing actually gets shown on the screen !
		window.renderFrame();

		updateAnimation();
		// Bool to check if the camera has changed since the last frame
		bool cameraChanged = (tempCameraPosition != cameraPosition || tempCameraOrientation != cameraOrientation);
		// If we are saving frames, a new frame has been rendered, and either the camera has changed or the current renderer is the ray traced renderer, then save the frame to a file
		if (savingFrames && newFrameRendered && (cameraChanged || currentRenderer == &rayTracedRenderer))
		{
			std::string paddedCounter = std::to_string(frameCounter);
			while (paddedCounter.length() < 6)
				paddedCounter = "0" + paddedCounter;
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
