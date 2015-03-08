#pragma once

#include "camera/camera.h"
#include "shaders/shader.h"
#include "timer/gpuTimer.h"
#include "log/logger.h"
#include "renderer/renderSettings.h"
#include "renderer/glResources.h"
#include "renderer/services/service.h"
#include "renderer/actions.h"

#include <GL/gl.h>

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>

#include <vector>
#include <string>

class Mesh;

class Renderer
{
public:
	enum RenderResult
	{
		RR_SAMPLES_PENDING,
		RR_FINISHED_RENDERING,
	};

	Renderer();
	~Renderer();
	void initialize(const std::string& shaderPath);
	void resizeFrame(int frameBufferWidth, 
				     int frameBufferHeight,
				     int viewportX, int viewportY,
				     int viewportW, int viewportH);
	RenderResult render();
	void reloadShaders(const std::string& shaderPath);
    void loadMesh(const std::string& file);
    void loadVoxFile(const std::string& file);
    void saveImage(const std::string& file);
    void resetRender();
	void drawSingleVertex();

	bool onMouseMove(int dx, int dy, int buttons);
	bool onKeyPress(int key);

	Camera& camera() { return m_camera; }
	RenderSettings& renderSettings() { return m_renderSettings; }
	void updateRenderSettings();

	const std::string& getStatus() const { return m_status; }

	void setLogger(Logger* logger);	

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

private:
	struct IntegratorSetup;

	void updateCamera();
	void createVoxelDataTexture (const Imath::V3i& resolution,
								 const GLubyte* occupancyTexels = NULL,
								 const GLubyte* colorTexels = NULL);
	bool reloadTexturedShader(const std::string& shaderPath);
	bool reloadAverageShader(const std::string& shaderPath);
	bool reloadIntegratorShader(const std::string& shaderPath, 
								const std::string& name,
								const std::string& vsFile,
								const std::string& fsFile,
								IntegratorShaderSettings& settings);
	bool loadBackgroundImage(float&);
	void drawFullscreenQuad();
	void createFramebuffer();
    Imath::V3f lightDirection() const;

	// clear out list of pending "actions" requested by the user, such as
	// selecting the active voxel, prior to rendering the next visible frame.
	// Each action results in a render pass.
	void processPendingActions();

private:
	bool m_initialized;

	Imath::Box3f m_volumeBounds;

	int m_activeSampleTexture;
	int m_numberSamples;

	Camera m_camera;

	IntegratorShaderSettings        m_settingsIntegrator[INTEGRATOR_TOTAL];
	AccumulationShaderSettings      m_settingsAverage;
	TexturedShaderSettings          m_settingsTextured;

	GLResourceConfiguration m_glResources;

	RenderSettings m_renderSettings;
	std::string m_shaderPath;

	AveragedGpuTimer m_frameTimer;

	std::string m_status;

	std::vector<Action> m_scheduledActions;

	Integrator m_currentIntegrator;
	std::string m_currentBackgroundImage;
	float		m_currentBackgroundRadianceIntegral;

	Logger* m_logger;

	RendererService* m_services[SERVICE_TOTAL];	
};
