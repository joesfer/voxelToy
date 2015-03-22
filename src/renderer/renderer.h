#pragma once

#include "camera/camera.h"
#include "shaders/shader.h"
#include "timer/gpuTimer.h"
#include "log/logger.h"
#include "renderer/renderSettings.h"
#include "renderer/glResources.h"
#include "renderer/services/service.h"
#include "renderer/actions.h"
#include "renderer/material/material.h"

#include <GL/gl.h>

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>

#include <vector>
#include <string>

class Mesh;

class Renderer
{
public:

	Renderer();
	~Renderer();

	// Initialize renderer. Requires path to the shaders root folder.
	void initialize(const std::string& shaderPath);

	// Resize frame. Note we might not render at full resolution, hence the
	// method takes _both_ the viewport (window) dimensions, as well as the
	// framebuffer (the rendering) resolution. The viewport coordinates
	// specifies how the framebuffer is fitted within the window.
	void resizeFrame(int frameBufferWidth, 
				     int frameBufferHeight,
				     int viewportX, int viewportY,
				     int viewportW, int viewportH);
	
	// Depending on the integrator, the 'render' operation may resolve the full
	// frame, or it may be a progressive operation which requires many
	// samples/passes to complete.
	enum RenderResult
	{
		RR_SAMPLES_PENDING,
		RR_FINISHED_RENDERING,
	};

	// Render a (potentially partial) frame. The returned enum indicates whether
	// more passes are expected. The results of each render pass are accumulated
	// until a call to 'resetRender' is performed.
	RenderResult render();

	// Reload all shaders.
	void reloadShaders(const std::string& shaderPath);

	// Wipe the current voxel data and voxelize an input mesh.
    void loadMesh(const std::string& file);
	// Wipe the current voxel data and load a .vox file.
    void loadVoxFile(const std::string& file);
	// As a variance-reduction technique, we eliminate all those voxels which
	// are completely surrounded by other voxels from the list of emissive
	// voxels. These would otherwise be randomly sampled, but never contribute
	// to the image.
	void pruneInteriorEmissiveVoxels(const std::vector<GLint>& voxelMaterials, 
									 Imath::V3i& volumeResolution, 
									 std::vector<GLint>& emissiveVoxelIndices);

	// Save the current accumulated framebuffer to an image file.
    void saveImage(const std::string& file);

	// Reset render accumulation. This is required when a parameter such as the
	// camera position changes.
    void resetRender();

	// Callback from the parent UI to give the renderer an opportunity to handle
	// mouse and key events. Return true if the event was acknowledged, false
	// otherwise.
	bool onMouseMove(int dx, int dy, int buttons);
	bool onKeyPress(int key);

	// Return modifiable camera settings
	Camera& camera() { return m_camera; }
	
	// Return modifiable render settings
	RenderSettings& renderSettings() { return m_renderSettings; }

	// Update resources with potentially updated render settings. Call this 
	// method when the render settings have been modified.
	void updateRenderSettings();

	// Return status message to display in a UI
	const std::string& getStatus() const { return m_status; }

	// Set logger interface. This will be implemented in a derived class which
	// controls how status messages are displayed (e.g. in a UI).
	void setLogger(Logger* logger);	

	// Request an action from the renderer. The client application might map
	// specific user events (mouse/keys) to actions to be carried out by the 
	// renderer such as selecting or modifying a voxel.
	// At the moment actions do not return any sort of result, but simply modify
	// internal state.
	void requestAction(float x, 
					   float y, 
					   float dx,
					   float dy,
					   Action::PICKING_ACTION action,
					   bool restartAccumulation);

	enum Integrator
	{
		INTEGRATOR_PATHTRACER = 0,
		INTEGRATOR_EDIT_MODE,
		INTEGRATOR_TOTAL,
	};

	std::vector<Material::SerializedData> getMaterials() const;
	void updateMaterialColor(unsigned int dataOffset, const float color[3]);
	void updateMaterialValue(unsigned int dataOffset, float value);

private:
	// Synchronize camera data with the shaders.
	void updateCamera();

	// Declare voxel resources
	void createVoxelDataTexture (const Imath::V3i& resolution,
								 const GLint* voxelMaterials       = NULL,
								 const float* materialData         = NULL,
								 size_t materialDataSize           = 0,
								 const GLint* emissiveVoxelIndices = NULL,
								 size_t numEmissiveVoxels          = 0);

	// reload shader and resources for the screen-space texture drawing shader.
	bool reloadTexturedShader(const std::string& shaderPath);
	// reload shader and resources for the frame accumulation shader.
	bool reloadAverageShader(const std::string& shaderPath);
	// reload shader and resources for each integrator
	bool reloadIntegratorShader(const std::string& shaderPath, 
								const std::string& name,
								const std::string& vsFile,
								const std::string& fsFile,
								IntegratorShaderSettings& settings);

	// Load and process background image
	bool loadBackgroundImage(float&);

	// Utility function to draw a screen-aligned quad
	void drawFullscreenQuad();

	// Declare framebuffer resources
	void createFramebuffer();

	// Specify light direction for the 'edit mode' cheap integrator
	// TODO remove, sample dominant direction from environment map
    Imath::V3f lightDirection() const;

	// clear out list of pending "actions" requested by the user, such as
	// selecting the active voxel, prior to rendering the next visible frame.
	// Each action results in a render pass.
	void processPendingActions();

private:
	// Whether the renderer is initialised yet. No action can be performed till
	// this flag is set to true.
	bool m_initialized;

	// The world-space bounds of the voxel data. This encodes the mapping
	// between the texture (voxels in 0-1 local space) and world space.
	Imath::Box3f m_volumeBounds;

	int m_activeSampleTexture;
	int m_numberSamples;

	Camera m_camera;

	IntegratorShaderSettings        m_settingsIntegrator[INTEGRATOR_TOTAL];
	AccumulationShaderSettings      m_settingsAverage;
	TexturedShaderSettings          m_settingsTextured;

	// All resources declared in OpenGL. This struct is also used to communicate
	// shaders via textures and SSBO
	GLResourceConfiguration m_glResources;

	// Current render settings
	RenderSettings m_renderSettings;

	// Path to shader root folder, specified by client application.
	std::string m_shaderPath;

	AveragedGpuTimer m_frameTimer;

	std::string m_status;

	// Actions which are pending to run since the last frame
	std::vector<Action> m_scheduledActions;

	Integrator m_currentIntegrator;
	// Current background image path, if any. This is used to avoid loading
	// and processing textures (which is expensive) if they're already loaded
	std::string m_currentBackgroundImage;
	// Cached background image radiance integral. Used to calculate the PDF of
	// each environment sample. This integral is computed when the texture is
	// loaded.
	float		m_currentBackgroundRadianceIntegral;

	// Logger (potentially NULL) where error messages -mostly shader
	// compilation- are redirected. This will be implemented in a derived class
	// which handles messages in a specific way (e.g. showing them in a UI).
	Logger* m_logger;

	// List of services offered by the renderer. A service is tightly linked to
	// actions (TODO - possibly unnecesary redundancy, remove actions?), and
	// offfers the functionaltiy required to carry them out. Services include
	// things like "select a voxel through a given pixel", "add/remove a voxel",
	// etc. Services are implemented as standalone chunks of functionality which
	// communicate and share resources with the renderer.
	RendererService* m_services[SERVICE_TOTAL];	
};
