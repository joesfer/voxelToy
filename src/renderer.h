#pragma once

#include "camera.h"
#include "shader.h"

#include <GL/gl.h>

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>

class Mesh;

struct RenderSettings
{
	// maximum path length allowed in the path tracer (1 = direct
	// illumination only).
	int m_pathtracerMaxPathLength;
	
	// rendered image resolution in pixels
	Imath::V2i m_imageResolution;

	// Viewport within which to render the image. This may not match the
	// resolution of the rendered image, in which case stretching or squashing
	// will occur.
	int	m_viewport[4];

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
	void setScreenFocalPoint(float x, float y);
    void resetRender();

	void onMouseMove(int dx, int dy, int buttons);

	Camera& camera() { return m_camera; }
	RenderSettings& renderSettings() { return m_renderSettings; }
	void updateRenderSettings();

private:
	void updateCamera();
	void createVoxelDataTexture();
	bool reloadFocalDistanceShader(const std::string& shaderPath);
	bool reloadTexturedShader(const std::string& shaderPath);
	bool reloadAverageShader(const std::string& shaderPath);
	bool reloadPathtracerShader(const std::string& shaderPath);
	bool reloadVoxelizeShader(const std::string& shaderPath);
	void drawFullscreenQuad();
	void createFramebuffer();
    Imath::V3f lightDirection() const;

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

	// Normalized coordinates in screen space from where the focal distance is
	// retrieved (the actual pixel coordinates are calculated as
	// m_screenFocalPoint * viewportSize;
	Imath::V2f m_screenFocalPoint;
	GLuint m_focalDistanceFBO;
	GLuint m_focalDistanceRBO;
    GLuint m_focalDistanceTexture;

    GLuint m_occupancyTexture;
    GLuint m_voxelColorTexture;

	PathtracerShaderSettings     m_settingsPathtracer;
	AccumulationShaderSettings   m_settingsAverage;
	TexturedShaderSettings       m_settingsTextured;
	FocalDistanceShaderSettings  m_settingsFocalDistance;
	VoxelizeShaderSettings       m_settingsVoxelize;

	enum TextureUnits
	{
		TEXTURE_UNIT_OCCUPANCY = 0,
		TEXTURE_UNIT_COLOR,
		TEXTURE_UNIT_SAMPLE,
		TEXTURE_UNIT_AVERAGE0,
		TEXTURE_UNIT_AVERAGE1,
		TEXTURE_UNIT_NOISE,
		TEXTURE_UNIT_FOCAL_DISTANCE
    };
	
	RenderSettings m_renderSettings;

    static const unsigned int MAX_FRAME_SAMPLES = 256;

	Imath::M44f m_meshTransform;
    Mesh* m_mesh;

};
