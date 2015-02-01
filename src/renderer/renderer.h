#pragma once

#include "camera/camera.h"
#include "shaders/shader.h"
#include "timer/gpuTimer.h"

#include <GL/gl.h>

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>

#include <vector>

class Mesh;

struct RenderSettings
{
	// maximum path length allowed in the path tracer (1 = direct
	// illumination only).
	int m_pathtracerMaxPathLength;

    // Max number of accumulated samples before the render finishes
    int m_pathtracerMaxSamples;

	// rendered image resolution in pixels
	Imath::V2i m_imageResolution;

	// Viewport within which to render the image. This may not match the
	// resolution of the rendered image, in which case stretching or squashing
	// will occur.
	int	m_viewport[4];

	float m_wireframeOpacity;
	float m_wireframeThickness;

};


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
    void resetRender();

	bool onMouseMove(int dx, int dy, int buttons);
	bool onKeyPress(int key);

	Camera& camera() { return m_camera; }
	RenderSettings& renderSettings() { return m_renderSettings; }
	void updateRenderSettings();

	const std::string& getStatus() const { return m_status; }

	enum PICKING_ACTION
	{
		PA_SELECT_FOCAL_POINT,
		PA_SELECT_ACTIVE_VOXEL,
	};
	
	void requestAction(float x, 
					   float y, 
					   PICKING_ACTION action,
					   bool restartAccumulation);

private:
	void updateCamera();
	void createVoxelDataTexture (const Imath::V3i& resolution,
								 const GLubyte* occupancyTexels = NULL,
								 const GLubyte* colorTexels = NULL);
	bool reloadFocalDistanceShader(const std::string& shaderPath);
	bool reloadSelectActiveVoxelShader(const std::string& shaderPath);
	bool reloadTexturedShader(const std::string& shaderPath);
	bool reloadAverageShader(const std::string& shaderPath);
	bool reloadPathtracerShader(const std::string& shaderPath);
	bool reloadVoxelizeShader(const std::string& shaderPath);
	void drawFullscreenQuad();
	void drawSingleVertex();
	void createFramebuffer();
    Imath::V3f lightDirection() const;

	void voxelizeCPU(const Imath::V3f* vertices, 
				     const unsigned int* indices,
				     unsigned int numTriangles);
	void voxelizeGPU(const Mesh* mesh);
	// clear out list of pending "actions" requested by the user, such as
	// selecting the active voxel, prior to rendering the next visible frame.
	// Each action results in a render pass.
	void processPendingActions();

private:
	Imath::Box3f m_volumeBounds;
	Imath::V3i	 m_volumeResolution;

	int m_activeSampleTexture;
	int m_numberSamples;

	Camera m_camera;

	GLuint m_mainFBO;
	GLuint m_mainRBO;
    GLuint m_sampleTexture;
    GLuint m_averageTexture[2];
    GLuint m_noiseTexture;
	GLint m_textureDimensions[2];

    GLuint m_focalDistanceSSBO;
    GLuint m_selectedVoxelSSBO;

    GLuint m_occupancyTexture;
    GLuint m_voxelColorTexture;

	PathtracerShaderSettings        m_settingsPathtracer;
	AccumulationShaderSettings      m_settingsAverage;
	TexturedShaderSettings          m_settingsTextured;
	FocalDistanceShaderSettings     m_settingsFocalDistance;
	VoxelizeShaderSettings          m_settingsVoxelize;
	SelectActiveVoxelShaderSettings m_settingsSelectActiveVoxel;

	enum TextureUnits
	{
		TEXTURE_UNIT_OCCUPANCY = 0,
		TEXTURE_UNIT_COLOR,
		TEXTURE_UNIT_SAMPLE,
		TEXTURE_UNIT_AVERAGE0,
		TEXTURE_UNIT_AVERAGE1,
		TEXTURE_UNIT_NOISE
    };
	
	RenderSettings m_renderSettings;

	Imath::M44f m_meshTransform;
    Mesh* m_mesh;

	AveragedGpuTimer m_frameTimer;

	std::string m_status;

	struct Action
	{
		PICKING_ACTION m_type;
		// Normalized coordinates in screen space from where the requested picking
		// action is originated (the actual pixel coordinates are calculated as
		// m_pickingActionPoint * viewportSize;
		Imath::V2f m_point;
		bool m_invalidatesRender;
	};
	
	std::vector<Action> m_scheduledActions;

};
