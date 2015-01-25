#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <GL/glu.h>

#include "renderer.h"
#include "mesh.h"
#include "content.h"
#include "meshLoader.h"
#include "voxLoader.h"
#include "camera/cameraController.h"

#define FOCAL_DISTANCE_TEXTURE_RESOLUTION 1 

Renderer::Renderer()
{
    m_camera.controller().lookAt(Imath::V3f(0,0,0));
    m_camera.controller().setDistanceFromTarget(100);
    m_camera.setFStop(16);
	m_activeSampleTexture = 0;
	m_numberSamples = 0;
	m_screenFocalPoint = Imath::V2f(0.5f, 0.5f);

	m_renderSettings.m_imageResolution.x = 512;
	m_renderSettings.m_imageResolution.y = 512;
    m_renderSettings.m_pathtracerMaxPathLength = 1;
    m_renderSettings.m_pathtracerMaxSamples = 128;
	m_mesh = NULL;
}

Renderer::~Renderer()
{
}

void Renderer::initialize(const std::string& shaderPath)
{
	glClearColor(0.1f, 0.1f, 0.1f, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_3D);
    if (glIsTexture(m_occupancyTexture)) glDeleteTextures(1, &m_occupancyTexture);
    glGenTextures(1, &m_occupancyTexture);
    if (glIsTexture(m_voxelColorTexture)) glDeleteTextures(1, &m_voxelColorTexture);
    glGenTextures(1, &m_voxelColorTexture);

	createVoxelDataTexture(Imath::V3i(16));
    createFramebuffer();
	reloadShaders(shaderPath);
	updateCamera();
	updateRenderSettings();
}

Imath::V3f Renderer::lightDirection() const
{
	Imath::V3f d(1, -1, 1);
	return d.normalized();
}

bool Renderer::reloadFocalDistanceShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("screenSpace.vs");
	std::string fs = shaderPath + std::string("focalDistance.fs");

    if ( !Shader::compileProgramFromFile("FocalDistance",
                                        vs, "",
                                        fs, "#define PINHOLE\n",
                                        m_settingsFocalDistance.m_program) )
	{
		return false;
	}
    
	glUseProgram(m_settingsFocalDistance.m_program);

	m_settingsFocalDistance.m_uniformVoxelOccupancyTexture  = glGetUniformLocation(m_settingsFocalDistance.m_program, "occupancyTexture");
	m_settingsFocalDistance.m_uniformVoxelDataResolution    = glGetUniformLocation(m_settingsFocalDistance.m_program, "voxelResolution");
	m_settingsFocalDistance.m_uniformVolumeBoundsMin        = glGetUniformLocation(m_settingsFocalDistance.m_program, "volumeBoundsMin");
	m_settingsFocalDistance.m_uniformVolumeBoundsMax        = glGetUniformLocation(m_settingsFocalDistance.m_program, "volumeBoundsMax");
	m_settingsFocalDistance.m_uniformViewport               = glGetUniformLocation(m_settingsFocalDistance.m_program, "viewport");
	m_settingsFocalDistance.m_uniformCameraNear             = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraNear");
	m_settingsFocalDistance.m_uniformCameraFar              = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraFar");
	m_settingsFocalDistance.m_uniformCameraProj             = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraProj");
	m_settingsFocalDistance.m_uniformCameraInverseProj      = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraInverseProj");
	m_settingsFocalDistance.m_uniformCameraInverseModelView = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraInverseModelView");
	m_settingsFocalDistance.m_uniformCameraFocalLength      = glGetUniformLocation(m_settingsFocalDistance.m_program, "cameraFocalLength");             
	m_settingsFocalDistance.m_uniformSampledFragment        = glGetUniformLocation(m_settingsFocalDistance.m_program, "sampledFragment");             

	glViewport(0,0,m_renderSettings.m_imageResolution.x, m_renderSettings.m_imageResolution.y);
	glUniform4f(m_settingsFocalDistance.m_uniformViewport, 0, 0, (float)m_renderSettings.m_imageResolution.x, (float)m_renderSettings.m_imageResolution.y);

	glUniform1i(m_settingsFocalDistance.m_uniformVoxelOccupancyTexture, TEXTURE_UNIT_OCCUPANCY);
	
	glUniform3i(m_settingsFocalDistance.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

	return true;
}

bool Renderer::reloadTexturedShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("screenSpace.vs");
	std::string fs = shaderPath + std::string("textureMap.fs");

    if ( !Shader::compileProgramFromFile("textured",
                                        vs, "",
                                        fs, "",
                                        m_settingsTextured.m_program) )
	{
		return false;
	}
    
	glUseProgram(m_settingsTextured.m_program);

	m_settingsTextured.m_uniformTexture  = glGetUniformLocation(m_settingsTextured.m_program, "texture");
	m_settingsTextured.m_uniformViewport = glGetUniformLocation(m_settingsTextured.m_program, "viewport");

    glUniform4f(m_settingsTextured.m_uniformViewport, 
                m_renderSettings.m_viewport[0],
                m_renderSettings.m_viewport[1],
                m_renderSettings.m_viewport[2],
                m_renderSettings.m_viewport[3]);

	return true;
}

bool Renderer::reloadAverageShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("screenSpace.vs");
	std::string fs = shaderPath + std::string("accumulation.fs");

    if ( !Shader::compileProgramFromFile("accumulation",
                                        vs, "",
                                        fs, "",
                                        m_settingsAverage.m_program) )
	{
		return false;
	}
    
	glUseProgram(m_settingsAverage.m_program);

	m_settingsAverage.m_uniformSampleTexture  = glGetUniformLocation(m_settingsAverage.m_program, "sampleTexture");
	m_settingsAverage.m_uniformAverageTexture = glGetUniformLocation(m_settingsAverage.m_program, "averageTexture");
	m_settingsAverage.m_uniformSampleCount    = glGetUniformLocation(m_settingsAverage.m_program, "sampleCount");
	m_settingsAverage.m_uniformViewport       = glGetUniformLocation(m_settingsAverage.m_program, "viewport");

    glUniform4f(m_settingsAverage.m_uniformViewport, 0, 0, (float)m_renderSettings.m_imageResolution.x, (float)m_renderSettings.m_imageResolution.y);

	return true;
}

bool Renderer::reloadPathtracerShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("screenSpace.vs");
	std::string fs = shaderPath + std::string("pathTracer.fs");

    if ( !Shader::compileProgramFromFile("PT",
                                        vs, "",
                                        fs, "#define PINHOLE\n#define THINLENS\n",
                                        m_settingsPathtracer.m_program) )
	{
		return false;
	}
    
	glUseProgram(m_settingsPathtracer.m_program);

	m_settingsPathtracer.m_uniformVoxelOccupancyTexture   = glGetUniformLocation(m_settingsPathtracer.m_program, "occupancyTexture");
	m_settingsPathtracer.m_uniformVoxelColorTexture       = glGetUniformLocation(m_settingsPathtracer.m_program, "voxelColorTexture");
	m_settingsPathtracer.m_uniformNoiseTexture            = glGetUniformLocation(m_settingsPathtracer.m_program, "noiseTexture");
	m_settingsPathtracer.m_uniformFocalDistanceTexture    = glGetUniformLocation(m_settingsPathtracer.m_program, "focalDistanceTexture");
	m_settingsPathtracer.m_uniformVoxelDataResolution     = glGetUniformLocation(m_settingsPathtracer.m_program, "voxelResolution");
	m_settingsPathtracer.m_uniformVolumeBoundsMin         = glGetUniformLocation(m_settingsPathtracer.m_program, "volumeBoundsMin");
	m_settingsPathtracer.m_uniformVolumeBoundsMax         = glGetUniformLocation(m_settingsPathtracer.m_program, "volumeBoundsMax");
	m_settingsPathtracer.m_uniformViewport                = glGetUniformLocation(m_settingsPathtracer.m_program, "viewport");
	m_settingsPathtracer.m_uniformCameraNear              = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraNear");
	m_settingsPathtracer.m_uniformCameraFar               = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraFar");
	m_settingsPathtracer.m_uniformCameraProj              = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraProj");
	m_settingsPathtracer.m_uniformCameraInverseProj       = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraInverseProj");
	m_settingsPathtracer.m_uniformCameraInverseModelView  = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraInverseModelView");
	m_settingsPathtracer.m_uniformCameraFocalLength       = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraFocalLength");
	m_settingsPathtracer.m_uniformCameraLensRadius        = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraLensRadius");
	m_settingsPathtracer.m_uniformCameraFilmSize          = glGetUniformLocation(m_settingsPathtracer.m_program, "cameraFilmSize");
	m_settingsPathtracer.m_uniformLightDir                = glGetUniformLocation(m_settingsPathtracer.m_program, "wsLightDir");
	m_settingsPathtracer.m_uniformSampleCount             = glGetUniformLocation(m_settingsPathtracer.m_program, "sampleCount");
	m_settingsPathtracer.m_uniformEnableDOF               = glGetUniformLocation(m_settingsPathtracer.m_program, "enableDOF");
	m_settingsPathtracer.m_uniformPathtracerMaxPathLength = glGetUniformLocation(m_settingsPathtracer.m_program, "pathtracerMaxPathLength");
	m_settingsPathtracer.m_uniformWireframeOpacity        = glGetUniformLocation(m_settingsPathtracer.m_program, "wireframeOpacity");
	m_settingsPathtracer.m_uniformWireframeThickness      = glGetUniformLocation(m_settingsPathtracer.m_program, "wireframeThickness");

	glViewport(0,0,m_renderSettings.m_imageResolution.x, m_renderSettings.m_imageResolution.y);
	glUniform4f(m_settingsPathtracer.m_uniformViewport, 0, 0, (float)m_renderSettings.m_imageResolution.x, (float)m_renderSettings.m_imageResolution.y);

	Imath::V3f lightDir = lightDirection();
	glUniform3f(m_settingsPathtracer.m_uniformLightDir, lightDir.x, lightDir.y, -lightDir.z);

	glUniform1i(m_settingsPathtracer.m_uniformVoxelOccupancyTexture, TEXTURE_UNIT_OCCUPANCY);
	glUniform1i(m_settingsPathtracer.m_uniformVoxelColorTexture, TEXTURE_UNIT_COLOR);
	glUniform1i(m_settingsPathtracer.m_uniformNoiseTexture, TEXTURE_UNIT_NOISE);
	glUniform1i(m_settingsPathtracer.m_uniformFocalDistanceTexture, TEXTURE_UNIT_FOCAL_DISTANCE);
	
	glUniform3i(m_settingsPathtracer.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsPathtracer.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsPathtracer.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

    updateCamera();

	return true;
}

bool Renderer::reloadVoxelizeShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("voxelize.vs");
	std::string gs = shaderPath + std::string("voxelize.gs");
	std::string fs = shaderPath + std::string("trivial.fs");

    if ( !Shader::compileProgramFromFile("Voxelize",
                                         vs, "",
                                         gs, "",
                                         fs, "",
                                         m_settingsVoxelize.m_program) )
	{
		return false;
	}
    
	glUseProgram(m_settingsVoxelize.m_program);

	m_settingsVoxelize.m_uniformVoxelOccupancyTexture   = glGetUniformLocation(m_settingsVoxelize.m_program, "occupancyTexture");
	m_settingsVoxelize.m_uniformVoxelDataResolution     = glGetUniformLocation(m_settingsVoxelize.m_program, "voxelResolution");
	m_settingsVoxelize.m_uniformModelTransform          = glGetUniformLocation(m_settingsVoxelize.m_program, "modelTransform");

	glUniform1i(m_settingsVoxelize.m_uniformVoxelOccupancyTexture, TEXTURE_UNIT_OCCUPANCY);
	
	glUniform3i(m_settingsVoxelize.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);


	return true;
}

void Renderer::reloadShaders(const std::string& shaderPath)
{
	if (!reloadPathtracerShader(shaderPath)    ||
		!reloadAverageShader(shaderPath)       ||
		!reloadTexturedShader(shaderPath)      ||
		!reloadFocalDistanceShader(shaderPath) ||
		!reloadVoxelizeShader(shaderPath))
	{
		std::cout << "Shader loading failed" << std::endl;
    }
	updateCamera();
	updateRenderSettings();
}

void Renderer::updateCamera()
{
	using namespace Imath;
	V3f eye     = m_camera.parameters().eye();
	V3f right   = m_camera.parameters().rightUnitVector();
	V3f up      = m_camera.parameters().upUnitVector();
	V3f forward = m_camera.parameters().forwardUnitVector();

	M44f mvm; // modelViewMatrix
	M44f pm; // projectionMatrix

	{ // gluLookAt
		mvm.makeIdentity();
        mvm.x[0][0] = right[0]    ; mvm.x[0][1] = right[1]    ; mvm.x[0][2] = right[2]    ; mvm.x[0][3] = -eye.dot(right) ;
        mvm.x[1][0] = up[0]       ; mvm.x[1][1] = up[1]       ; mvm.x[1][2] = up[2]       ; mvm.x[1][3] = -eye.dot(up);
        mvm.x[2][0] = -forward[0] ; mvm.x[2][1] = -forward[1] ; mvm.x[2][2] = -forward[2] ; mvm.x[2][3] = eye.dot(forward) ;
        mvm.x[3][0] = 0           ; mvm.x[3][1] = 0           ; mvm.x[3][2] = 0           ; mvm.x[3][3] = 1 ;
	}

	{ // gluPerspective
		const float a = (float)m_renderSettings.m_imageResolution.x / m_renderSettings.m_imageResolution.y;
		const float n = m_camera.parameters().nearDistance();
		const float f = m_camera.parameters().farDistance();
		const float e = 1.0f / tan(m_camera.parameters().fovY()/2);

		pm.x[0][0] = e/a ; pm.x[0][1] = 0 ; pm.x[0][2] = 0           ; pm.x[0][3] = 0              ;
		pm.x[1][0] = 0   ; pm.x[1][1] = e ; pm.x[1][2] = 0           ; pm.x[1][3] = 0              ;
		pm.x[2][0] = 0   ; pm.x[2][1] = 0 ; pm.x[2][2] = (f+n)/(n-f) ; pm.x[2][3] = 2.0f*f*n/(n-f) ;
		pm.x[3][0] = 0   ; pm.x[3][1] = 0 ; pm.x[3][2] = -1          ; pm.x[3][3] = 0              ;
	}

    M44f invModelView = mvm.inverse();
    M44f invProj = pm.inverse();
	
	// FocalDistance shader

    glUseProgram(m_settingsFocalDistance.m_program);

    glUniform1f(m_settingsFocalDistance.m_uniformCameraNear, m_camera.parameters().nearDistance());
	glUniform1f(m_settingsFocalDistance.m_uniformCameraFar, m_camera.parameters().farDistance());

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &invModelView.x[0][0]);

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &pm.x[0][0]);

    glUniformMatrix4fv(m_settingsFocalDistance.m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &invProj.x[0][0]);
	
	// Path tracer shader 
	
    glUseProgram(m_settingsPathtracer.m_program);

    glUniform1f(m_settingsPathtracer.m_uniformCameraNear, m_camera.parameters().nearDistance());
	glUniform1f(m_settingsPathtracer.m_uniformCameraFar, m_camera.parameters().farDistance());

    glUniformMatrix4fv(m_settingsPathtracer.m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &invModelView.x[0][0]);

    glUniformMatrix4fv(m_settingsPathtracer.m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &pm.x[0][0]);

    glUniformMatrix4fv(m_settingsPathtracer.m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &invProj.x[0][0]);

	bool enableDOF = m_camera.parameters().lensModel() == CameraParameters::CLM_THIN_LENS; 
	enableDOF &= (m_numberSamples > 0); // Disable DOF while tumbling around

	// TODO the focal length can be extracted from the perspective matrix (FOV)
	// so we should choose either method, but not both.
    glUseProgram(m_settingsPathtracer.m_program);
    glUniform1f(m_settingsPathtracer.m_uniformCameraFocalLength  , m_camera.parameters().focalLength());
    glUniform1f(m_settingsPathtracer.m_uniformCameraLensRadius   , m_camera.parameters().lensRadius());
    glUniform1i(m_settingsPathtracer.m_uniformEnableDOF          , enableDOF ? 1 : 0 );
    glUniform2f(m_settingsPathtracer.m_uniformCameraFilmSize     , m_camera.parameters().filmSize().x, 
															       m_camera.parameters().filmSize().y);
    glUseProgram(m_settingsFocalDistance.m_program);
    glUniform1f(m_settingsFocalDistance.m_uniformCameraFocalLength  , m_camera.parameters().focalLength());
	glUniform2f(m_settingsFocalDistance.m_uniformSampledFragment,
				m_screenFocalPoint[0] * m_renderSettings.m_imageResolution.x, 
				(1.0f - m_screenFocalPoint[1]) * m_renderSettings.m_imageResolution.y); // m_screenFocal point origin is at top-left, whilst glsl is bottom-left
		
}

void Renderer::resizeFrame(int frameBufferWidth, 
						   int frameBufferHeight,
						   int viewportX, int viewportY,
						   int viewportW, int viewportH)
{
    m_numberSamples = 0;

	m_renderSettings.m_imageResolution.x = frameBufferWidth;
	m_renderSettings.m_imageResolution.y = frameBufferHeight;

	m_renderSettings.m_viewport[0] = viewportX;
	m_renderSettings.m_viewport[1] = viewportY;
	m_renderSettings.m_viewport[2] = viewportW;
	m_renderSettings.m_viewport[3] = viewportH;

    glUseProgram(m_settingsPathtracer.m_program);
	glUniform4f(m_settingsPathtracer.m_uniformViewport, 
				0, 
				0, 
				m_renderSettings.m_imageResolution.x, 
				m_renderSettings.m_imageResolution.y);

    glUseProgram(m_settingsAverage.m_program);
    glUniform4f(m_settingsAverage.m_uniformViewport,
				0, 
				0, 
				m_renderSettings.m_imageResolution.x, 
				m_renderSettings.m_imageResolution.y);

    glUseProgram(m_settingsTextured.m_program);
    glUniform4f(m_settingsTextured.m_uniformViewport,
                m_renderSettings.m_viewport[0],
                m_renderSettings.m_viewport[1],
                m_renderSettings.m_viewport[2],
                m_renderSettings.m_viewport[3]);

    glUseProgram(m_settingsFocalDistance.m_program);
    glUniform4f(m_settingsFocalDistance.m_uniformViewport,
				0, 
				0, 
				m_renderSettings.m_imageResolution.x, 
				m_renderSettings.m_imageResolution.y);

    glUseProgram(0);
	
    const float viewportAspectRatio = (float)m_renderSettings.m_imageResolution.x / m_renderSettings.m_imageResolution.y;

	if (viewportAspectRatio >= 1.0f) 
	{
		m_camera.setFilmSize(CameraParameters::FILM_SIZE_35MM, CameraParameters::FILM_SIZE_35MM / viewportAspectRatio);
	}
	else 
	{
		m_camera.setFilmSize(CameraParameters::FILM_SIZE_35MM * viewportAspectRatio, CameraParameters::FILM_SIZE_35MM);
	}
	updateCamera();


	// create texture
	for( int i = 0; i < 3; ++i )
	{
		switch(i)
		{
			case 0: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_sampleTexture);
				break;
			case 1: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[0]);
				break;
			case 2: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[1]);
				break;
			default: break;
		}

		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 m_renderSettings.m_imageResolution.x,
					 m_renderSettings.m_imageResolution.y,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     NULL);

	}

	// these should match the viewport resolution, but maybe some older hardware
	// still performs some power-of-two rounding; so we'll store the actual
	// values to match the viewport when rendering to texture.
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_textureDimensions[0]);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_textureDimensions[1]);
	
    glBindRenderbuffer(GL_RENDERBUFFER, m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  m_renderSettings.m_imageResolution.x,
						  m_renderSettings.m_imageResolution.y);

}

Renderer::RenderResult Renderer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

	// calculate focal distance
	if (m_numberSamples == 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_focalDistanceFBO);

		glViewport(0,0,FOCAL_DISTANCE_TEXTURE_RESOLUTION,FOCAL_DISTANCE_TEXTURE_RESOLUTION);

		glUseProgram(m_settingsFocalDistance.m_program);

		drawFullscreenQuad();
	}

	const float viewportAspectRatio = (float)m_renderSettings.m_imageResolution.x / m_renderSettings.m_imageResolution.y;
	if (viewportAspectRatio >= 1.0f) 
	{
		m_camera.setFilmSize(CameraParameters::FILM_SIZE_35MM, CameraParameters::FILM_SIZE_35MM / viewportAspectRatio);
	}
	else 
	{
		m_camera.setFilmSize(CameraParameters::FILM_SIZE_35MM * viewportAspectRatio, CameraParameters::FILM_SIZE_35MM);
	}
	updateCamera();
	
    // render frame into m_sampleTexture

    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_sampleTexture, // where we'll write to
                           0);

    glUseProgram(m_settingsPathtracer.m_program);
    glUniform1i(m_settingsPathtracer.m_uniformSampleCount, std::min(m_numberSamples, m_renderSettings.m_pathtracerMaxSamples - 1));

	glViewport(0,0,m_textureDimensions[0], m_textureDimensions[1]);
    drawFullscreenQuad();

    // m_sampleTexture and m_average[active] ---> m_average[active^1]

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_averageTexture[m_activeSampleTexture ^ 1], // where we'll write to
                           0);

	glUseProgram(m_settingsAverage.m_program);
	glUniform1i(m_settingsAverage.m_uniformSampleTexture, TEXTURE_UNIT_SAMPLE);
	glUniform1i(m_settingsAverage.m_uniformAverageTexture, TEXTURE_UNIT_AVERAGE0 + m_activeSampleTexture);
	glUniform1i(m_settingsAverage.m_uniformSampleCount, m_numberSamples);
	drawFullscreenQuad();
	
    // finally draw to screen

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(m_settingsTextured.m_program);
	glUniform1i(m_settingsTextured.m_uniformTexture, TEXTURE_UNIT_AVERAGE0 + (m_activeSampleTexture^1));
    glViewport(m_renderSettings.m_viewport[0],
               m_renderSettings.m_viewport[1],
               m_renderSettings.m_viewport[2],
               m_renderSettings.m_viewport[3]);
	drawFullscreenQuad();
	
	// run continuously?
    if ( m_numberSamples++ < m_renderSettings.m_pathtracerMaxSamples)
    {
        m_activeSampleTexture ^= 1;
		return RR_SAMPLES_PENDING; 
    }
	return RR_FINISHED_RENDERING;
}

void Renderer::drawFullscreenQuad()
{
	// draw quad
	glBegin(GL_QUADS);
		glVertex3f(  1.0f,  1.0f, m_camera.parameters().nearDistance());
		glVertex3f( -1.0f,  1.0f, m_camera.parameters().nearDistance());
		glVertex3f( -1.0f, -1.0f, m_camera.parameters().nearDistance());
		glVertex3f(  1.0f, -1.0f, m_camera.parameters().nearDistance());
	glEnd();
}

void Renderer::setScreenFocalPoint(float x, float y)
{
        m_screenFocalPoint.x = x;
        m_screenFocalPoint.y = y;
        updateCamera();
        m_numberSamples = 0;
}

bool Renderer::onMouseMove(int dx, int dy, int buttons)
{
	const float ndx = (float)dx / this->m_renderSettings.m_imageResolution.x;
	const float ndy = (float)dy / this->m_renderSettings.m_imageResolution.y;
	if ( m_camera.controller().onMouseMove(ndx, ndy, buttons) )
	{
		updateCamera();
		m_numberSamples = 0;
		return true;
	}
	return false;
}
bool Renderer::onKeyPress(int key)
{
	if (m_camera.controller().onKeyPress(key))
	{
		updateCamera();
		m_numberSamples = 0;
		return true;
	}
	return false;
}

void Renderer::createFramebuffer()
{
    glEnable(GL_TEXTURE_2D);

    if (glIsTexture(m_sampleTexture)) glDeleteTextures(1, &m_sampleTexture);
    glGenTextures(1, &m_sampleTexture);

    if (glIsTexture(m_noiseTexture)) glDeleteTextures(1, &m_noiseTexture);
    glGenTextures(1, &m_noiseTexture);

    if (glIsTexture(m_averageTexture[0])) glDeleteTextures(2, m_averageTexture);
    glGenTextures(2, m_averageTexture);

    if (glIsTexture(m_focalDistanceTexture)) glDeleteTextures(1, &m_focalDistanceTexture);
    glGenTextures(1, &m_focalDistanceTexture);

	// create focal distance texture
	{
		// this is a 1x1 px texture
		glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_FOCAL_DISTANCE);
		glBindTexture(GL_TEXTURE_2D, m_focalDistanceTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_R16F,
					 FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
					 FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
					 0,
					 GL_RED,
					 GL_FLOAT,
                     NULL);
	}
	
	// create noise texture
	{
		const unsigned int noiseTextureSize = 1024;
		float* noise = (float*)malloc(noiseTextureSize * noiseTextureSize * 4 * sizeof(float));
        for( unsigned int i = 0; i < 4 * noiseTextureSize * noiseTextureSize; ++i ) noise[i] = (float)rand()/RAND_MAX;
		glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_NOISE);
		glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 1024, 
					 1024,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     noise);
		free(noise);
	}

	// create sample texture
	for( int i = 0; i < 3; ++i )
	{
		switch(i)
		{
			case 0: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_sampleTexture);
				break;
			case 1: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[0]);
				break;
			case 2: 
                glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_averageTexture[1]);
				break;
			default: break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA,
					 m_renderSettings.m_imageResolution.x,
					 m_renderSettings.m_imageResolution.y,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
                     NULL);
	}

	// generate focal distance shader's fbo/rbo
	
    if (glIsRenderbuffer(m_focalDistanceRBO)) glDeleteRenderbuffers(1, &m_focalDistanceRBO);
    glGenRenderbuffers(1, &m_focalDistanceRBO);

    glBindRenderbuffer(GL_RENDERBUFFER, m_focalDistanceRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 
						  FOCAL_DISTANCE_TEXTURE_RESOLUTION, 
						  FOCAL_DISTANCE_TEXTURE_RESOLUTION);

    //
    if (glIsFramebuffer(m_focalDistanceFBO)) glDeleteFramebuffers(1, &m_focalDistanceFBO);
    glGenFramebuffers(1, &m_focalDistanceFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_focalDistanceFBO);
	
	// attach texture to color0
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           m_focalDistanceTexture, // where we'll write to
                           0);
	
	// attach a texture to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_DEPTH_ATTACHMENT, 
							  GL_RENDERBUFFER,
                              m_focalDistanceRBO);
	
	// generate main fbo/rbo

    if (glIsRenderbuffer(m_mainRBO)) glDeleteRenderbuffers(1, &m_mainRBO);
    glGenRenderbuffers(1, &m_mainRBO);

    glBindRenderbuffer(GL_RENDERBUFFER, m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  m_renderSettings.m_imageResolution.x,
						  m_renderSettings.m_imageResolution.y);

    //
    if (glIsFramebuffer(m_mainFBO)) glDeleteFramebuffers(1, &m_mainFBO);
    glGenFramebuffers(1, &m_mainFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFBO);
	
	// attach a texture to depth attachment point

    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_DEPTH_ATTACHMENT, 
							  GL_RENDERBUFFER,
                              m_mainRBO);

	// note the main fbo is not complete yet
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

struct V4f
{
	float x,y,z,w;
	V4f() {}
	V4f(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};


void Renderer::createVoxelDataTexture(const Imath::V3i& resolution,
									  const GLubyte* occupancyTexels,
									  const GLubyte* colorTexels)
{
    using namespace Imath;
	
	// Texture resolution 
    m_volumeResolution = resolution;

    const size_t numVoxels = (size_t)(m_volumeResolution.x * m_volumeResolution.y * m_volumeResolution.z);

    float sizeMultiplier = 1000;
	float voxelSize = sizeMultiplier / std::max(m_volumeResolution.x, std::max(m_volumeResolution.y, m_volumeResolution.z));
	V3f boundsSize(voxelSize * m_volumeResolution.x, 
				   voxelSize * m_volumeResolution.y, 
				   voxelSize * m_volumeResolution.z);
    m_volumeBounds = Box3f( -boundsSize * 0.5f, boundsSize * 0.5f);

	GLubyte* occupancyStorage = occupancyTexels != NULL ? NULL : new GLubyte[numVoxels];
	GLubyte* colorStorage = colorTexels != NULL ? NULL : new GLubyte[4*numVoxels];
	if (occupancyStorage != NULL) memset(occupancyStorage, 0, numVoxels * sizeof(GLubyte));
	if (colorStorage != NULL) memset(colorStorage, 0, 4 * numVoxels * sizeof(GLubyte));

	const GLubyte* occupancy = occupancyTexels != NULL ? occupancyTexels : occupancyStorage;
	const GLubyte* color = colorTexels != NULL ? colorTexels : colorStorage;

	// Upload texture data to card

	glActiveTexture( GL_TEXTURE0 + TEXTURE_UNIT_OCCUPANCY);
    glBindTexture(GL_TEXTURE_3D, m_occupancyTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage3D(GL_TEXTURE_3D,
				 0,
				 GL_R8,
                 m_volumeResolution.x,
                 m_volumeResolution.y,
                 m_volumeResolution.z,
				 0,
				 GL_RED,
				 GL_UNSIGNED_BYTE,
                 occupancy);

	glActiveTexture( GL_TEXTURE0 + TEXTURE_UNIT_COLOR);
    glBindTexture(GL_TEXTURE_3D, m_voxelColorTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage3D(GL_TEXTURE_3D,
				 0,
				 GL_RGBA8,
                 m_volumeResolution.x,
                 m_volumeResolution.y,
                 m_volumeResolution.z,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
                 color);

	delete[] occupancyStorage;
	delete[] colorStorage;

	// Set new resolution and volume bounds in all shaders

	glUseProgram(m_settingsPathtracer.m_program);
	glUniform3i(m_settingsPathtracer.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsPathtracer.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsPathtracer.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

	glUseProgram(m_settingsVoxelize.m_program);
	glUniform3i(m_settingsVoxelize.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUseProgram(m_settingsFocalDistance.m_program);
	glUniform3i(m_settingsFocalDistance.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(m_settingsFocalDistance.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

}
void Renderer::resetRender()
{
    updateCamera();
    m_numberSamples = 0;
}

void Renderer::updateRenderSettings()
{
	glUseProgram(m_settingsPathtracer.m_program);
	glUniform1i(m_settingsPathtracer.m_uniformPathtracerMaxPathLength , m_renderSettings.m_pathtracerMaxPathLength);
	glUniform1f(m_settingsPathtracer.m_uniformWireframeOpacity        , m_renderSettings.m_wireframeOpacity);
	glUniform1f(m_settingsPathtracer.m_uniformWireframeThickness      , m_renderSettings.m_wireframeThickness);

	glUseProgram(0);
    resetRender();
}

void Renderer::loadMesh(const std::string& file)
{
	createVoxelDataTexture(Imath::V3i(256));

	m_camera.controller().setDistanceFromTarget(m_volumeBounds.size().length() * 0.5f);

	m_mesh = MeshLoader::loadFromOBJ(file.c_str());
	if (m_mesh == NULL) return;

	// set mesh transform so that the mesh fits within the unit cube. This will
	// be changed later when we let the user manipulate the mesh transform and
	// the mesh/volume intersection.
	using namespace Imath;
    V3f voxelMargin = V3f(1.0f) / m_volumeResolution; // 1 voxel
	int majorAxis = m_mesh->bounds().majorAxis();
    float s = (1.0f - 2.0 * voxelMargin[majorAxis] ) / m_mesh->bounds().size()[majorAxis];
    V3f t = -m_mesh->bounds().min + voxelMargin / s;
    m_meshTransform.x[0][0] = s ; m_meshTransform.x[0][1] = 0 ; m_meshTransform.x[0][2] = 0 ; m_meshTransform.x[0][3] = t.x  * s ;
    m_meshTransform.x[1][0] = 0 ; m_meshTransform.x[1][1] = s ; m_meshTransform.x[1][2] = 0 ; m_meshTransform.x[1][3] = t.y  * s ;
    m_meshTransform.x[2][0] = 0 ; m_meshTransform.x[2][1] = 0 ; m_meshTransform.x[2][2] = s ; m_meshTransform.x[2][3] = t.z  * s ;
    m_meshTransform.x[3][0] = 0 ; m_meshTransform.x[3][1] = 0 ; m_meshTransform.x[3][2] = 0 ; m_meshTransform.x[3][3] = 1.0f     ;

	glUseProgram(m_settingsVoxelize.m_program);

	glUniform3i(m_settingsVoxelize.m_uniformVoxelDataResolution, 
				m_volumeResolution.x, 
				m_volumeResolution.y, 
				m_volumeResolution.z);

	glUniformMatrix4fv(m_settingsVoxelize.m_uniformModelTransform,
					   1,
					   GL_TRUE,
					   &m_meshTransform.x[0][0]);

	// Connect output image textures for the geometry shader
	
	glBindImageTexture(0,                   // image unit
					   m_occupancyTexture,  // texture
					   0,                   // level
					   GL_TRUE,             // layered
					   0,                   // layer
					   GL_WRITE_ONLY,       // access
					   GL_R8UI              // format
			);

	glBindImageTexture(1,                   // image unit
					   m_voxelColorTexture, // texture
					   0,                   // level
					   GL_TRUE,             // layered
					   0,                   // layer
					   GL_WRITE_ONLY,       // access
					   GL_RGBA8             // format
			);

	m_mesh->draw();

	resetRender();
}

void Renderer::loadVoxFile(const std::string& file)
{
    GLubyte* occupancyTexels = NULL;
    GLubyte* colorTexels = NULL;
	MagicaVoxelLoader loader;

	if (!loader.load(file, 
					 occupancyTexels, 
					 colorTexels, 
					 m_volumeResolution))
	{
		return;
	}
							
	createVoxelDataTexture(m_volumeResolution, occupancyTexels, colorTexels);
	m_camera.controller().setDistanceFromTarget(m_volumeBounds.size().length() * 0.5f);

	free(occupancyTexels);
	free(colorTexels);

	resetRender();
}

