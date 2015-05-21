#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glu.h>

#include <OpenImageIO/imageio.h>
#include <OpenEXR/ImathMatrixAlgo.h>

#include "renderer/renderer.h"

#include "content.h"
#include "camera/cameraController.h"
#include "renderer/image.h"
#include "shaders/focalDistance/focalDistanceHost.h"
#include "shaders/editVoxels/selectVoxelHost.h"
#include "shaders/shared/constants.h"

#include "renderer/services/serviceAddVoxel.h"
#include "renderer/services/serviceRemoveVoxel.h"
#include "renderer/services/serviceSelectActiveVoxel.h"
#include "renderer/services/serviceSetFocalDistance.h"

#include <memory.h>
#include <algorithm>

#include <Qt> // FIXME used for Qt::Key codes

struct IntegratorSetup
{
	std::string vs;
	std::string fs;
	std::string name;
};

IntegratorSetup integratorSetup[Renderer::INTEGRATOR_TOTAL] = 
{
	{"shared/screenSpace.vs" , "integrator/pathTracer.fs" , "PT"} ,
	{"shared/screenSpace.vs" , "integrator/editMode.fs"   , "EditMode"} ,
};

Renderer::Renderer()
{

	m_camera.controller().lookAt(Imath::V3f(0,0,0));
	m_camera.controller().setDistanceFromTarget(100);
	m_camera.setFStop(16);
	m_activeSampleTexture = 0;
	m_numberSamples = 0;

	m_renderSettings.m_imageResolution.x = 512;
	m_renderSettings.m_imageResolution.y = 512;
	m_renderSettings.m_pathtracerMaxNumBounces = 1;
	m_renderSettings.m_pathtracerMaxSamples = 2048 * 2048;

	m_currentIntegrator = INTEGRATOR_PATHTRACER;

	m_currentBackgroundImage = "";
	m_currentBackgroundRadianceIntegral = 0;

	m_logger = NULL;

	memset(m_services, 0, SERVICE_TOTAL * sizeof(RendererService*));

	m_initialized = false;
}

Renderer::~Renderer()
{
}

void Renderer::setLogger(Logger* logger)
{
	m_logger = logger;
}

void Renderer::initialize(const std::string& shaderPath)
{
	m_shaderPath = shaderPath;

	glewExperimental = true;
	glewInit();
	glClearColor(0.1f, 0.1f, 0.1f, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_3D);
	if (glIsTexture(m_glResources.m_materialOffsetTexture)) glDeleteTextures(1, &m_glResources.m_materialOffsetTexture);
	glGenTextures(1, &m_glResources.m_materialOffsetTexture);
	if (glIsTexture(m_glResources.m_materialDataTexture)) glDeleteTextures(1, &m_glResources.m_materialDataTexture);
	glGenTextures(1, &m_glResources.m_materialDataTexture);
	if (glIsTexture(m_glResources.m_emissiveVoxelIndicesTexture)) glDeleteTextures(1, &m_glResources.m_emissiveVoxelIndicesTexture);
	glGenTextures(1, &m_glResources.m_emissiveVoxelIndicesTexture);

	createFramebuffer();
	reloadShaders(shaderPath);
	createVoxelDataTexture(Imath::V3i(16));
	updateCamera();
	updateRenderSettings();

	glBindImageTexture(0,                                     // image unit
					   m_glResources.m_materialOffsetTexture, // texture
					   0,                                     // level
					   GL_TRUE,                               // layered
					   0,                                     // layer
					   GL_WRITE_ONLY,                         // access
					   GL_R32I                               // format
			);

	glBindImageTexture(1,                                   // image unit
					   m_glResources.m_materialDataTexture, // texture
					   0,                                   // level
					   GL_TRUE,                             // layered
					   0,                                   // layer
					   GL_WRITE_ONLY,                       // access
					   GL_R32F                              // format
			);


	// init services
	m_services[SERVICE_ADD_VOXEL]           = new RendererServiceAddVoxel();
	m_services[SERVICE_REMOVE_VOXEL]        = new RendererServiceRemoveVoxel();
	m_services[SERVICE_SELECT_ACTIVE_VOXEL] = new RendererServiceSelectActiveVoxel();
	m_services[SERVICE_SET_FOCAL_DISTANCE]  = new RendererServiceSetFocalDistance();

	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->reload(shaderPath, m_logger);
		m_services[i]->glResourcesCreated(m_glResources);
		m_services[i]->cameraUpdated(Imath::M44f(),
									 Imath::M44f(),
									 Imath::M44f(),
									 Imath::M44f(),
									 m_camera);
		m_services[i]->frameResized(m_renderSettings.m_viewport);
		m_services[i]->volumeReloaded(m_glResources.m_volumeResolution,
									  m_volumeBounds);
	}

	m_frameTimer.init();

	if (m_logger) (*m_logger)("Renderer initialized.");

	m_initialized = true;
}

Imath::V3f Renderer::lightDirection() const
{
	Imath::V3f d(1, -1, 1);
	return d.normalized();
}

bool Renderer::reloadTexturedShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("shared/screenSpace.vs");
	std::string fs = shaderPath + std::string("shared/textureMap.fs");

	if ( !Shader::compileProgramFromFile("textured",
										shaderPath,
										vs, "",
										fs, "",
										m_settingsTextured.m_program,
										m_logger) )
	{
		return false;
	}
	
	glUseProgram(m_settingsTextured.m_program);

	m_settingsTextured.m_uniformTexture  = glGetUniformLocation(m_settingsTextured.m_program, "texture");
	m_settingsTextured.m_uniformViewport = glGetUniformLocation(m_settingsTextured.m_program, "viewport");

	glUniform4f(m_settingsTextured.m_uniformViewport, 
				(float)m_renderSettings.m_viewport[0],
				(float)m_renderSettings.m_viewport[1],
				(float)m_renderSettings.m_viewport[2],
				(float)m_renderSettings.m_viewport[3]);

	glUseProgram(0);

	return true;
}

bool Renderer::reloadAverageShader(const std::string& shaderPath)
{
	std::string vs = shaderPath + std::string("shared/screenSpace.vs");
	std::string fs = shaderPath + std::string("shared/accumulation.fs");

	if ( !Shader::compileProgramFromFile("accumulation",
										shaderPath,
										vs, "",
										fs, "",
										m_settingsAverage.m_program,
										m_logger) )
	{
		return false;
	}
	
	glUseProgram(m_settingsAverage.m_program);

	m_settingsAverage.m_uniformSampleTexture  = glGetUniformLocation(m_settingsAverage.m_program, "sampleTexture");
	m_settingsAverage.m_uniformAverageTexture = glGetUniformLocation(m_settingsAverage.m_program, "averageTexture");
	m_settingsAverage.m_uniformSampleCount    = glGetUniformLocation(m_settingsAverage.m_program, "sampleCount");
	m_settingsAverage.m_uniformViewport       = glGetUniformLocation(m_settingsAverage.m_program, "viewport");

	glUniform4f(m_settingsAverage.m_uniformViewport, 0, 0, (float)m_renderSettings.m_imageResolution.x, (float)m_renderSettings.m_imageResolution.y);

	glUseProgram(0);

	return true;
}

bool Renderer::reloadIntegratorShader(const std::string& shaderPath, 
									  const std::string& name,
									  const std::string& vsFile,
									  const std::string& fsFile,
									  IntegratorShaderSettings& settings)
{
	std::string vs = shaderPath + vsFile;
	std::string fs = shaderPath + fsFile; 

	if ( !Shader::compileProgramFromFile(name,
										shaderPath,
										vs, "",
										fs, "#define PINHOLE\n#define THINLENS\n",
										settings.m_program,
										m_logger) )
	{
		return false;
	}
	
	glUseProgram(settings.m_program);

	settings.m_uniformMaterialOffsetTexture     = glGetUniformLocation(settings.m_program, "materialOffsetTexture");
	settings.m_uniformMaterialDataTexture       = glGetUniformLocation(settings.m_program, "materialDataTexture");
	settings.m_emissiveVoxelIndicesTexture      = glGetUniformLocation(settings.m_program, "emissiveVoxelIndicesTexture");
	settings.m_uniformNoiseTexture              = glGetUniformLocation(settings.m_program, "noiseTexture");
	settings.m_uniformVoxelDataResolution       = glGetUniformLocation(settings.m_program, "voxelResolution");
	settings.m_uniformVolumeBoundsMin           = glGetUniformLocation(settings.m_program, "volumeBoundsMin");
	settings.m_uniformVolumeBoundsMax           = glGetUniformLocation(settings.m_program, "volumeBoundsMax");
	settings.m_uniformVoxelSize                 = glGetUniformLocation(settings.m_program, "wsVoxelSize");
	settings.m_uniformViewport                  = glGetUniformLocation(settings.m_program, "viewport");
	settings.m_uniformCameraNear                = glGetUniformLocation(settings.m_program, "cameraNear");
	settings.m_uniformCameraFar                 = glGetUniformLocation(settings.m_program, "cameraFar");
	settings.m_uniformCameraProj                = glGetUniformLocation(settings.m_program, "cameraProj");
	settings.m_uniformCameraInverseProj         = glGetUniformLocation(settings.m_program, "cameraInverseProj");
	settings.m_uniformCameraInverseModelView    = glGetUniformLocation(settings.m_program, "cameraInverseModelView");
	settings.m_uniformCameraFocalLength         = glGetUniformLocation(settings.m_program, "cameraFocalLength");
	settings.m_uniformCameraLensRadius          = glGetUniformLocation(settings.m_program, "cameraLensRadius");
	settings.m_uniformCameraFilmSize            = glGetUniformLocation(settings.m_program, "cameraFilmSize");
	settings.m_uniformCameraLensModel           = glGetUniformLocation(settings.m_program, "cameraLensModel");
	settings.m_uniformLightDir                  = glGetUniformLocation(settings.m_program, "wsLightDir");
	settings.m_uniformSampleCount               = glGetUniformLocation(settings.m_program, "sampleCount");
	settings.m_uniformPathtracerMaxPathBounces  = glGetUniformLocation(settings.m_program, "pathtracerMaxNumBounces");
	settings.m_uniformWireframeOpacity          = glGetUniformLocation(settings.m_program, "wireframeOpacity");
	settings.m_uniformWireframeThickness        = glGetUniformLocation(settings.m_program, "wireframeThickness");
	settings.m_uniformBackgroundColorTop        = glGetUniformLocation(settings.m_program, "backgroundColorTop");
	settings.m_uniformBackgroundColorBottom     = glGetUniformLocation(settings.m_program, "backgroundColorBottom");
	settings.m_uniformBackgroundUseImage        = glGetUniformLocation(settings.m_program, "backgroundUseImage");
	settings.m_uniformBackgroundTexture         = glGetUniformLocation(settings.m_program, "backgroundTexture");
	settings.m_uniformBackgroundCDFUTexture     = glGetUniformLocation(settings.m_program, "backgroundCDFUTexture");
	settings.m_uniformBackgroundCDFVTexture     = glGetUniformLocation(settings.m_program, "backgroundCDFVTexture");
	settings.m_uniformBackgroundIntegral        = glGetUniformLocation(settings.m_program, "backgroundIntegral");
	settings.m_uniformBackgroundRotationRadians = glGetUniformLocation(settings.m_program, "backgroundRotationRadians");

	settings.m_uniformFocalDistanceSSBOStorageBlock = glGetProgramResourceIndex(settings.m_program, GL_SHADER_STORAGE_BLOCK, "FocalDistanceData");
	glShaderStorageBlockBinding(settings.m_program, settings.m_uniformFocalDistanceSSBOStorageBlock, GLResourceConfiguration::m_focalDistanceSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_focalDistanceSSBOBindingPointIndex, m_glResources.m_focalDistanceSSBO);

	settings.m_uniformSelectedVoxelSSBOStorageBlock = glGetProgramResourceIndex(settings.m_program, GL_SHADER_STORAGE_BLOCK, "SelectVoxelData");
	glShaderStorageBlockBinding(settings.m_program, settings.m_uniformSelectedVoxelSSBOStorageBlock, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex, m_glResources.m_selectedVoxelSSBO);

	glViewport(0,0,m_renderSettings.m_imageResolution.x, m_renderSettings.m_imageResolution.y);
	glUniform4f(settings.m_uniformViewport, 0, 0, (float)m_renderSettings.m_imageResolution.x, (float)m_renderSettings.m_imageResolution.y);

	Imath::V3f lightDir = lightDirection();
	glUniform3f(settings.m_uniformLightDir, lightDir.x, lightDir.y, -lightDir.z);

	glUniform1i(settings.m_uniformMaterialOffsetTexture, GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_OFFSET);
	glUniform1i(settings.m_uniformMaterialDataTexture, GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_DATA);
	glUniform1i(settings.m_uniformNoiseTexture, GLResourceConfiguration::TEXTURE_UNIT_NOISE);
	glUniform1i(settings.m_emissiveVoxelIndicesTexture, GLResourceConfiguration::TEXTURE_UNIT_EMISSIVE_VOXEL_INDICES);
	
	glUniform3i(settings.m_uniformVoxelDataResolution, 
				m_glResources.m_volumeResolution.x, 
				m_glResources.m_volumeResolution.y, 
				m_glResources.m_volumeResolution.z);

	glUniform3f(settings.m_uniformVolumeBoundsMin,
				m_volumeBounds.min.x,
				m_volumeBounds.min.y,
				m_volumeBounds.min.z);

	glUniform3f(settings.m_uniformVolumeBoundsMax,
				m_volumeBounds.max.x,
				m_volumeBounds.max.y,
				m_volumeBounds.max.z);

	glUniform3f(settings.m_uniformVoxelSize,
				(m_volumeBounds.max.x-m_volumeBounds.min.x) / m_glResources.m_volumeResolution.x,
				(m_volumeBounds.max.y-m_volumeBounds.min.y) / m_glResources.m_volumeResolution.y,
				(m_volumeBounds.max.z-m_volumeBounds.min.z) / m_glResources.m_volumeResolution.z);

	updateCamera();

	glUseProgram(0);

	return true;
}

void Renderer::reloadShaders(const std::string& shaderPath)
{
	if (!reloadAverageShader(shaderPath) ||
		!reloadTexturedShader(shaderPath))
	{
		m_status = "Shader loading failed";
		return;
	}
	for( int i = 0; i < INTEGRATOR_TOTAL; ++i )
	{
		if ( !reloadIntegratorShader(shaderPath, 
									 integratorSetup[i].name,
									 integratorSetup[i].vs,
									 integratorSetup[i].fs,
									 m_settingsIntegrator[i]) )
		{
			m_status = integratorSetup[i].name + " loading failed";
		}
	}
	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->reload(shaderPath, m_logger);
		m_services[i]->volumeReloaded(m_glResources.m_volumeResolution,
									  m_volumeBounds);
	}

	updateCamera();
	updateRenderSettings();
}

void Renderer::updateCamera()
{
	if (!m_initialized) return;

	using namespace Imath;
	V3f eye	 = m_camera.parameters().eye();
	V3f right, up, forward;
	m_camera.parameters().getBasis(forward, right, up);

	M44f mvm; // modelViewMatrix
	M44f pm; // projectionMatrix

	{ // gluLookAt
		mvm.makeIdentity()        ;
		mvm.x[0][0] = right[0]    ; mvm.x[0][1] = right[1]    ; mvm.x[0][2] = right[2]    ; mvm.x[0][3] = -eye.dot(right)  ;
		mvm.x[1][0] = up[0]       ; mvm.x[1][1] = up[1]       ; mvm.x[1][2] = up[2]       ; mvm.x[1][3] = -eye.dot(up)     ;
		mvm.x[2][0] = -forward[0] ; mvm.x[2][1] = -forward[1] ; mvm.x[2][2] = -forward[2] ; mvm.x[2][3] = eye.dot(forward) ;
		mvm.x[3][0] = 0           ; mvm.x[3][1] = 0           ; mvm.x[3][2] = 0           ; mvm.x[3][3] = 1                ;
	}

	if (m_camera.parameters().lensModel() == CameraParameters::CLM_ORTHOGRAPHIC)
	{ // gluOrtho2D
	
		const float a = (float)m_renderSettings.m_imageResolution.x / m_renderSettings.m_imageResolution.y;
		const float left = -tan(m_camera.parameters().fovY() / 2) * m_camera.parameters().distanceToTarget();
		const float right = -left;
		const float bottom = -tan(m_camera.parameters().fovY() / 2) * m_camera.parameters().distanceToTarget() / a;
		const float top = -bottom;
		const float near = -m_camera.parameters().nearDistance();
		const float far = -m_camera.parameters().farDistance();
		const float tx = -(right+left)/(right-left);
		const float ty = -(top+bottom)/(top-bottom);
		const float tz = -(far+near)/(far-near);

		pm.x[0][0] = 2.0f / (right-left) ; pm.x[0][1] = 0                   ; pm.x[0][2] = 0                  ; pm.x[0][3] = tx ;
		pm.x[1][0] = 0                   ; pm.x[1][1] = 2.0f / (top-bottom) ; pm.x[1][2] = 0                  ; pm.x[1][3] = ty ;
		pm.x[2][0] = 0                   ; pm.x[2][1] = 0                   ; pm.x[2][2] = -2.0f / (far-near) ; pm.x[2][3] = tz ;
		pm.x[3][0] = 0                   ; pm.x[3][1] = 0                   ; pm.x[3][2] = 0                  ; pm.x[3][3] = 1  ;

	}
	else
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

	// Inform services 
	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->cameraUpdated(mvm, 
									 invModelView, 
									 pm, 
									 invProj,
									 m_camera);
	}

	// integrator shaders
	
	for( int i = 0; i < INTEGRATOR_TOTAL; ++i )
	{
		const IntegratorShaderSettings& integratorSettings = m_settingsIntegrator[i];
		glUseProgram(integratorSettings.m_program);

		glUniform1f(integratorSettings.m_uniformCameraNear, m_camera.parameters().nearDistance());
		glUniform1f(integratorSettings.m_uniformCameraFar, m_camera.parameters().farDistance());

		glUniformMatrix4fv(integratorSettings.m_uniformCameraInverseModelView,
						   1,
						   GL_TRUE,
						   &invModelView.x[0][0]);

		glUniformMatrix4fv(integratorSettings.m_uniformCameraProj,
						   1,
						   GL_TRUE,
						   &pm.x[0][0]);

		glUniformMatrix4fv(integratorSettings.m_uniformCameraInverseProj,
						   1,
						   GL_TRUE,
						   &invProj.x[0][0]);

		// TODO the focal length can be extracted from the perspective matrix (FOV)
		// so we should choose either method, but not both.
		glUseProgram(integratorSettings.m_program);
		glUniform1f(integratorSettings.m_uniformCameraFocalLength  , m_camera.parameters().focalLength());
		glUniform1f(integratorSettings.m_uniformCameraLensRadius   , m_camera.parameters().lensRadius());
		glUniform1i(integratorSettings.m_uniformCameraLensModel	, (int)m_camera.parameters().lensModel());
		glUniform2f(integratorSettings.m_uniformCameraFilmSize	 , m_camera.parameters().filmSize().x, 
																	   m_camera.parameters().filmSize().y);
		
	}
	glUseProgram(0);
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

	for( int i = 0; i < INTEGRATOR_TOTAL; ++i )
	{
		const IntegratorShaderSettings& integratorSettings = m_settingsIntegrator[i];

		glUseProgram(integratorSettings.m_program);
		glUniform4f(integratorSettings.m_uniformViewport, 
					0, 
					0, 
					m_renderSettings.m_imageResolution.x, 
					m_renderSettings.m_imageResolution.y);
	}

	glUseProgram(m_settingsAverage.m_program);
	glUniform4f(m_settingsAverage.m_uniformViewport,
				0, 
				0, 
				m_renderSettings.m_imageResolution.x, 
				m_renderSettings.m_imageResolution.y);

	glUseProgram(m_settingsTextured.m_program);
	glUniform4f(m_settingsTextured.m_uniformViewport,
				(float)m_renderSettings.m_viewport[0],
				(float)m_renderSettings.m_viewport[1],
				(float)m_renderSettings.m_viewport[2],
				(float)m_renderSettings.m_viewport[3]);

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
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_sampleTexture);
				break;
			case 1: 
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_averageTexture[0]);
				break;
			case 2: 
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_averageTexture[1]);
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
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_glResources.m_textureDimensions[0]);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_glResources.m_textureDimensions[1]);
	
	glBindRenderbuffer(GL_RENDERBUFFER, m_glResources.m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  m_renderSettings.m_imageResolution.x,
						  m_renderSettings.m_imageResolution.y);

	// Inform services 
	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->frameResized(m_renderSettings.m_viewport);
	}
}

Renderer::RenderResult Renderer::render()
{
	if (!m_initialized) return RR_FINISHED_RENDERING;

	m_frameTimer.sampleBegin();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	processPendingActions();

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
	
	// render frame into m_glResources.m_sampleTexture

	glBindFramebuffer(GL_FRAMEBUFFER, m_glResources.m_mainFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT0,
						   GL_TEXTURE_2D,
						   m_glResources.m_sampleTexture, // where we'll write to
						   0);

	const IntegratorShaderSettings& integratorSettings = m_settingsIntegrator[m_currentIntegrator];
	glUseProgram(integratorSettings.m_program);
	glUniform1i(integratorSettings.m_uniformSampleCount, std::min(m_numberSamples, m_renderSettings.m_pathtracerMaxSamples - 1));

	glViewport(0,0,m_glResources.m_textureDimensions[0], m_glResources.m_textureDimensions[1]);
	drawFullscreenQuad();

	// m_glResources.m_sampleTexture and m_average[active] ---> m_average[active^1]

	glFramebufferTexture2D(GL_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT0,
						   GL_TEXTURE_2D,
						   m_glResources.m_averageTexture[m_activeSampleTexture ^ 1], // where we'll write to
						   0);

	glUseProgram(m_settingsAverage.m_program);
	glUniform1i(m_settingsAverage.m_uniformSampleTexture, GLResourceConfiguration::TEXTURE_UNIT_SAMPLE);
	glUniform1i(m_settingsAverage.m_uniformAverageTexture, GLResourceConfiguration::TEXTURE_UNIT_AVERAGE0 + m_activeSampleTexture);
	glUniform1i(m_settingsAverage.m_uniformSampleCount, m_numberSamples);
	drawFullscreenQuad();
	
	// finally draw to screen

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(m_settingsTextured.m_program);
	glUniform1i(m_settingsTextured.m_uniformTexture, GLResourceConfiguration::TEXTURE_UNIT_AVERAGE0 + (m_activeSampleTexture^1));
	glViewport(m_renderSettings.m_viewport[0],
			   m_renderSettings.m_viewport[1],
			   m_renderSettings.m_viewport[2],
			   m_renderSettings.m_viewport[3]);
	drawFullscreenQuad();

	m_frameTimer.sampleEnd();
	{
		const float averageFrameSeconds = m_frameTimer.averageSampleTime();
		if ( averageFrameSeconds > 0.f)
		{
			const float fps = 1.0f / averageFrameSeconds;
			std::stringstream ss;
			ss << "fps " << fps;
			m_status = ss.str();
		}
	}

	glUseProgram(0);
	
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
	if (key == Qt::Key_Space)
	{
		m_currentIntegrator = (Integrator)(!(int)m_currentIntegrator);
		m_numberSamples = 0;
		return true;
	}
	else if (key == Qt::Key_F)
	{
		m_camera.controller().focusOnBounds(this->m_volumeBounds);
		m_numberSamples = 0;
		updateCamera();
		return true;
	}
	return false;
}

void Renderer::createFramebuffer()
{
	if (glIsTexture(m_glResources.m_sampleTexture)) glDeleteTextures(1, &m_glResources.m_sampleTexture);
	glGenTextures(1, &m_glResources.m_sampleTexture);

	if (glIsTexture(m_glResources.m_noiseTexture)) glDeleteTextures(1, &m_glResources.m_noiseTexture);
	glGenTextures(1, &m_glResources.m_noiseTexture);

	if (glIsTexture(m_glResources.m_averageTexture[0])) glDeleteTextures(2, m_glResources.m_averageTexture);
	glGenTextures(2, m_glResources.m_averageTexture);

	if (glIsBuffer(m_glResources.m_focalDistanceSSBO)) glDeleteBuffers(1, &m_glResources.m_focalDistanceSSBO);
	glGenBuffers(1, &m_glResources.m_focalDistanceSSBO);

	if (glIsBuffer(m_glResources.m_selectedVoxelSSBO)) glDeleteBuffers(1, &m_glResources.m_selectedVoxelSSBO);
	glGenBuffers(1, &m_glResources.m_selectedVoxelSSBO);

	// create focal distance shader storage buffer object 
	{
		FocalDistanceData data;
		const float Infinity = 99999999.0;	
		data.focalDistance = Infinity;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glResources.m_focalDistanceSSBO);	
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(FocalDistanceData), &data, GL_DYNAMIC_COPY);	
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// create selected voxel shader storage buffer object 
	{
		SelectVoxelData data;
		data.index[0] = 0;
		data.index[1] = 0;
		data.index[2] = 0;
		data.index[3] = 0;
		data.normal[0] = 1.f;
		data.normal[1] = 0.f;
		data.normal[2] = 0.f;
		data.normal[3] = 0.f;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_glResources.m_selectedVoxelSSBO);	
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SelectVoxelData), &data, GL_DYNAMIC_COPY);	
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	
	// create noise texture
	{
		const unsigned int noiseTextureSize = 2048;
		float* noise = (float*)malloc(noiseTextureSize * noiseTextureSize * 4 * sizeof(float));
		for( unsigned int i = 0; i < 4 * noiseTextureSize * noiseTextureSize; ++i ) noise[i] = (float)rand()/RAND_MAX;
		glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_NOISE);
		glBindTexture(GL_TEXTURE_2D, m_glResources.m_noiseTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA32F,
					 noiseTextureSize, 
					 noiseTextureSize,
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
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_SAMPLE);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_sampleTexture);
				break;
			case 1: 
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_AVERAGE0);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_averageTexture[0]);
				break;
			case 2: 
				glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_AVERAGE1);
				glBindTexture(GL_TEXTURE_2D, m_glResources.m_averageTexture[1]);
				break;
			default: break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D,
					 0,
					 GL_RGBA32F,
					 m_renderSettings.m_imageResolution.x,
					 m_renderSettings.m_imageResolution.y,
					 0,
					 GL_RGBA,
					 GL_FLOAT,
					 NULL);
	}

	// generate main fbo/rbo

	if (glIsRenderbuffer(m_glResources.m_mainRBO)) glDeleteRenderbuffers(1, &m_glResources.m_mainRBO);
	glGenRenderbuffers(1, &m_glResources.m_mainRBO);

	glBindRenderbuffer(GL_RENDERBUFFER, m_glResources.m_mainRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, 
						  GL_DEPTH_COMPONENT,
						  m_renderSettings.m_imageResolution.x,
						  m_renderSettings.m_imageResolution.y);

	//
	if (glIsFramebuffer(m_glResources.m_mainFBO)) glDeleteFramebuffers(1, &m_glResources.m_mainFBO);
	glGenFramebuffers(1, &m_glResources.m_mainFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, m_glResources.m_mainFBO);
	
	// attach a texture to depth attachment point

	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_DEPTH_ATTACHMENT, 
							  GL_RENDERBUFFER,
							  m_glResources.m_mainRBO);

	// note the main fbo is not complete yet
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// Inform services 
	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->glResourcesCreated(m_glResources);
	}

}

void Renderer::createVoxelDataTexture(const Imath::V3i& resolution,
									  const GLint* voxelMaterials,
									  const float* materialData,
									  size_t materialDataSize,
									  const GLint* emissiveVoxelIndices,
									  size_t numEmissiveVoxels)
{
	using namespace Imath;
	
	// Texture resolution 
	m_glResources.m_volumeResolution = resolution;

	float sizeMultiplier = 1000;
	float voxelSize = sizeMultiplier / std::max(m_glResources.m_volumeResolution.x, std::max(m_glResources.m_volumeResolution.y, m_glResources.m_volumeResolution.z));
	V3f boundsSize(voxelSize * m_glResources.m_volumeResolution.x, 
	               voxelSize * m_glResources.m_volumeResolution.y, 
	               voxelSize * m_glResources.m_volumeResolution.z);
	m_volumeBounds = Box3f( -boundsSize * 0.5f, boundsSize * 0.5f);

	// Upload texture data to card

	glActiveTexture( GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_OFFSET);
	glBindTexture(GL_TEXTURE_3D, m_glResources.m_materialOffsetTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexImage3D(GL_TEXTURE_3D,
	             0,
	             GL_R32I,
	             m_glResources.m_volumeResolution.x,
	             m_glResources.m_volumeResolution.y,
	             m_glResources.m_volumeResolution.z,
	             0,
	             GL_RED_INTEGER,
	             GL_INT,
	             voxelMaterials);

	glActiveTexture( GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_DATA);
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_materialDataTexture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexImage1D(GL_TEXTURE_1D,
	             0,
	             GL_R32F,
				 std::max(size_t(1), materialDataSize),
	             0,
	             GL_RED,
	             GL_FLOAT,
	             materialData);

	glActiveTexture( GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_EMISSIVE_VOXEL_INDICES);
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_emissiveVoxelIndicesTexture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexImage1D(GL_TEXTURE_1D,
	             0,
	             GL_R32I,
				 numEmissiveVoxels, 
	             0,
	             GL_RED_INTEGER,
	             GL_INT,
	             emissiveVoxelIndices); 

	// Set new resolution and volume bounds in all shaders

	for( int i = 0; i < INTEGRATOR_TOTAL; ++i )
	{
		const IntegratorShaderSettings& integratorSettings = m_settingsIntegrator[i];
		glUseProgram(integratorSettings.m_program);
		glUniform3i(integratorSettings.m_uniformVoxelDataResolution, 
		            m_glResources.m_volumeResolution.x, 
		            m_glResources.m_volumeResolution.y, 
		            m_glResources.m_volumeResolution.z);

		glUniform3f(integratorSettings.m_uniformVolumeBoundsMin,
		            m_volumeBounds.min.x,
		            m_volumeBounds.min.y,
		            m_volumeBounds.min.z);

		glUniform3f(integratorSettings.m_uniformVolumeBoundsMax,
		            m_volumeBounds.max.x,
		            m_volumeBounds.max.y,
		            m_volumeBounds.max.z);

		glUniform3f(integratorSettings.m_uniformVoxelSize,
					(m_volumeBounds.max.x-m_volumeBounds.min.x) / m_glResources.m_volumeResolution.x,
					(m_volumeBounds.max.y-m_volumeBounds.min.y) / m_glResources.m_volumeResolution.y,
					(m_volumeBounds.max.z-m_volumeBounds.min.z) / m_glResources.m_volumeResolution.z);
	}

	glUseProgram(0);

	// inform services
	for( int i = 0; i < SERVICE_TOTAL; ++i)
	{
		if (m_services[i] == NULL) continue;
		m_services[i]->volumeReloaded(m_glResources.m_volumeResolution, 
									  m_volumeBounds);
	}
}
void Renderer::resetRender()
{
	updateCamera();
	m_numberSamples = 0;
}

bool Renderer::loadBackgroundImage( float& mapIntegralTimesSin )
{
	if ( m_renderSettings.m_backgroundImage.empty() ) 
	{
		// no background iamge set
		mapIntegralTimesSin = 0;
		return true;
	}
	else if ( m_renderSettings.m_backgroundImage == m_currentBackgroundImage )
	{
		// we've already loaded and processed this image
		mapIntegralTimesSin = m_currentBackgroundRadianceIntegral;
		return true;
	}
	
	// new image
	
	mapIntegralTimesSin = 1.0f;
	if (glIsTexture(m_glResources.m_backgroundTexture))
	{
		glDeleteTextures(1, &m_glResources.m_backgroundTexture);
		m_glResources.m_backgroundTexture = 0;
	}
	if (glIsTexture(m_glResources.m_backgroundCDFUTexture))
	{
		glDeleteTextures(1, &m_glResources.m_backgroundCDFUTexture);
		m_glResources.m_backgroundCDFUTexture = 0;
	}
	if (glIsTexture(m_glResources.m_backgroundCDFVTexture))
	{
		glDeleteTextures(1, &m_glResources.m_backgroundCDFVTexture);
		m_glResources.m_backgroundCDFVTexture = 0;
	}
	const int useBackgroundImage = m_renderSettings.m_backgroundImage.empty() ? 0 : 1;
	if(useBackgroundImage == 0) return true; // we're done

	unsigned int w, h;
	std::vector<float> pixels;
	if (!loadImage(m_renderSettings.m_backgroundImage, w, h, pixels)) return false;

	glGenTextures(1, &m_glResources.m_backgroundTexture);
	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND);
	glBindTexture(GL_TEXTURE_2D, m_glResources.m_backgroundTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             GL_RGBA32F,
	             w,
	             h,
	             0,
	             GL_RGB,
	             GL_FLOAT,
	             &pixels[0]);

	// Calculate data required for light importance sampling

	unsigned int cdfUDataWidth, cdfUDataHeight;
	std::vector<float> cdfUData, cdfVData;
	float integralTimesSin;
	calculateCDF(&pixels[0], w, h,
	             cdfUData, cdfUDataWidth, cdfUDataHeight,
	             cdfVData, 
	             integralTimesSin);
	const unsigned int cdfVDataHeight = cdfVData.size();

	glGenTextures(1, &m_glResources.m_backgroundCDFUTexture);
	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND_CDF_U);
	glBindTexture(GL_TEXTURE_2D, m_glResources.m_backgroundCDFUTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             GL_R32F,
	             cdfUDataWidth,
	             cdfUDataHeight,
	             0,
	             GL_RED,
	             GL_FLOAT,
	             &cdfUData[0]);

	glGenTextures(1, &m_glResources.m_backgroundCDFVTexture);
	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND_CDF_V);
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_backgroundCDFVTexture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage1D(GL_TEXTURE_1D,
	             0,
	             GL_R32F,
	             cdfVDataHeight,
	             0,
	             GL_RED,
	             GL_FLOAT,
	             &cdfVData[0]);

	mapIntegralTimesSin = integralTimesSin;

	// store the current settings
	m_currentBackgroundImage			= m_renderSettings.m_backgroundImage;
	m_currentBackgroundRadianceIntegral = integralTimesSin;

	return true;
}

void Renderer::updateRenderSettings()
{
	if (!m_initialized) return;

	const int useBackgroundImage = m_renderSettings.m_backgroundImage.empty() ? 0 : 1;

	float mapIntegralTimesSin = 0;
	if (!loadBackgroundImage(mapIntegralTimesSin)) return;

	for( int i = 0; i < INTEGRATOR_TOTAL; ++i )
	{
		const IntegratorShaderSettings& integratorSettings = m_settingsIntegrator[i];
		glUseProgram(integratorSettings.m_program);

		glUniform1i(integratorSettings.m_uniformPathtracerMaxPathBounces , m_renderSettings.m_pathtracerMaxNumBounces);
		glUniform1f(integratorSettings.m_uniformWireframeOpacity		, m_renderSettings.m_wireframeOpacity);
		glUniform1f(integratorSettings.m_uniformWireframeThickness	  , m_renderSettings.m_wireframeThickness);

		glUniform3f(integratorSettings.m_uniformBackgroundColorTop, 
					m_renderSettings.m_backgroundColor[0].x,
					m_renderSettings.m_backgroundColor[0].y, 
					m_renderSettings.m_backgroundColor[0].z );

		glUniform3f(integratorSettings.m_uniformBackgroundColorBottom, 
					m_renderSettings.m_backgroundColor[1].x,
					m_renderSettings.m_backgroundColor[1].y, 
					m_renderSettings.m_backgroundColor[1].z );

		glUniform1i(integratorSettings.m_uniformBackgroundUseImage, 
					useBackgroundImage);	

		glUniform1i(integratorSettings.m_uniformBackgroundTexture, 
					GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND);	

		glUniform1i(integratorSettings.m_uniformBackgroundCDFUTexture, 
					GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND_CDF_U);	

		glUniform1i(integratorSettings.m_uniformBackgroundCDFVTexture, 
					GLResourceConfiguration::TEXTURE_UNIT_BACKGROUND_CDF_V);	

		glUniform1f(integratorSettings.m_uniformBackgroundIntegral, 
					mapIntegralTimesSin);	

		glUniform1f(integratorSettings.m_uniformBackgroundRotationRadians, 
					m_renderSettings.m_backgroundRotationDegrees * M_PI / 180.0f);	
	}

	glUseProgram(0);
	resetRender();
}

void Renderer::saveImage(const std::string& file)
{
	if (!m_initialized)  return;

	OIIO_NAMESPACE_USING 

	const int xres = m_renderSettings.m_imageResolution.x, 
			  yres = m_renderSettings.m_imageResolution.y;
	const int channels = 4; // RGBA
	float* pixels = new float[xres*yres*channels];

	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, m_glResources.m_averageTexture[m_activeSampleTexture]);
	glGetTexImage(GL_TEXTURE_2D,
				  0,
				  GL_RGBA,
				  GL_FLOAT,
				  pixels);

	ImageOutput* out = ImageOutput::create(file.c_str());
	if (!out) return;
	ImageSpec spec (xres, yres, channels, TypeDesc::FLOAT);
	out->open(file.c_str(), spec);
	const size_t scanlineSize = xres * channels * sizeof(float);
	out->write_image(TypeDesc::FLOAT, 
					 (const char*)pixels + (yres - 1) * scanlineSize, // offset to last
					 AutoStride, 
					 -(int)scanlineSize,  // negative stride = flip image vertically
					 AutoStride);
	delete[] pixels;
	out->close();
	delete out;
}

std::vector<Material::SerializedData> Renderer::getMaterials() const
{
	using namespace std;
	vector<Material::SerializedData> result;

	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_DATA); 
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_materialDataTexture);
	GLint width;
	glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, &width); // get width of GL texture
	glPixelStorei(GL_PACK_ALIGNMENT,4);
	glPixelStorei(GL_UNPACK_ALIGNMENT,4);

	float* storage = new float[width];

	glGetTexImage(GL_TEXTURE_1D,
				  0,
				  GL_RED,
				  GL_FLOAT,
				  storage);
	
	size_t offset = 0;
	while(offset < (size_t)width)
	{
		Material::MaterialType mt = (Material::MaterialType)(storage[offset]);

		size_t dataSize = 0;
		switch(mt)
		{
			case Material::MT_LAMBERT:
			{
				const Material::LambertMaterialData* materialData = reinterpret_cast<const Material::LambertMaterialData*>(storage + offset + 1);
				result.push_back(Material::serializeLambert(*materialData, offset));
				dataSize = sizeof(Material::LambertMaterialData) / sizeof(float);
			} break;

			case Material::MT_METAL:
			{
				const Material::MetalMaterialData* materialData = reinterpret_cast<const Material::MetalMaterialData*>(storage + offset + 1);
				result.push_back(Material::serializeMetal(*materialData, offset));
				dataSize = sizeof(Material::MetalMaterialData) / sizeof(float);
			} break;

			case Material::MT_PLASTIC:
			{
				const Material::PlasticMaterialData* materialData = reinterpret_cast<const Material::PlasticMaterialData*>(storage + offset + 1);
				result.push_back(Material::serializePlastic(*materialData, offset));
				dataSize = sizeof(Material::PlasticMaterialData) / sizeof(float);
			} break;

			default: 
			{
				offset = width;
				if (m_logger) (*m_logger)("Unrecognized material type. Aborting.");
				break;
			}
		}
		offset += 1 + dataSize; // Type (1) + dataSize
	};

	delete storage;
	return result;
}

void Renderer::updateMaterialColor(unsigned int dataOffset, const float color[3])
{
	if (!m_initialized) return;

	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_DATA); 
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_materialDataTexture);

	glTexSubImage1D(GL_TEXTURE_1D,
					0, // GLint level,
					dataOffset, // GLint xoffset,
					3, // GLsizei width,
					GL_RED, // GLenum format,
					GL_FLOAT, // GLenum type,
					color); // const GLvoid * pixels
}

void Renderer::updateMaterialValue(unsigned int dataOffset, float value)
{
	if (!m_initialized) return;

	glActiveTexture(GL_TEXTURE0 + GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_DATA); 
	glBindTexture(GL_TEXTURE_1D, m_glResources.m_materialDataTexture);

	glTexSubImage1D(GL_TEXTURE_1D,
					0, // GLint level,
					dataOffset, // GLint xoffset,
					1, // GLsizei width,
					GL_RED, // GLenum format,
					GL_FLOAT, // GLenum type,
					&value ); // const GLvoid * pixels
}

