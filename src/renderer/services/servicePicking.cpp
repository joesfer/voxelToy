#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/glResources.h"
#include "renderer/services/servicePicking.h"
#include "renderer/renderer.h"
#include "log/logger.h"
#include "shaders/shader.h"
#include "camera/camera.h"

bool RendererServicePicking::reloadShader(const std::string& shaderPath, 
						  const std::string& shaderName,
						  const std::string& vertexShaderName,
						  const std::string& fragmentShaderName,
						  Logger* logger)
{
	std::string vs = shaderPath + vertexShaderName;
	std::string fs = shaderPath + fragmentShaderName;

    if ( !Shader::compileProgramFromFile(shaderName,
										shaderPath,
                                        vs, "#define PINHOLE\n",
                                        fs, "",
                                        m_program,
										logger) )
	{
		return false;
	}
    
	glUseProgram(m_program);

	m_uniformVoxelOccupancyTexture  = glGetUniformLocation(m_program, "occupancyTexture");
	m_uniformVoxelDataResolution    = glGetUniformLocation(m_program, "voxelResolution");
	m_uniformVolumeBoundsMin        = glGetUniformLocation(m_program, "volumeBoundsMin");
	m_uniformVolumeBoundsMax        = glGetUniformLocation(m_program, "volumeBoundsMax");
	m_uniformViewport               = glGetUniformLocation(m_program, "viewport");
	m_uniformCameraNear             = glGetUniformLocation(m_program, "cameraNear");
	m_uniformCameraFar              = glGetUniformLocation(m_program, "cameraFar");
	m_uniformCameraProj             = glGetUniformLocation(m_program, "cameraProj");
	m_uniformCameraInverseProj      = glGetUniformLocation(m_program, "cameraInverseProj");
	m_uniformCameraInverseModelView = glGetUniformLocation(m_program, "cameraInverseModelView");
	m_uniformCameraFocalLength      = glGetUniformLocation(m_program, "cameraFocalLength");             
	m_uniformSampledFragment        = glGetUniformLocation(m_program, "sampledFragment");             

	glUniform1i(m_uniformVoxelOccupancyTexture, GLResourceConfiguration::TEXTURE_UNIT_OCCUPANCY);
	
	glUseProgram(0);

	return true;
}

void RendererServicePicking::cameraUpdated(const Imath::M44f& /*modelViewMatrix*/,
										   const Imath::M44f& modelViewInverse,
										   const Imath::M44f& projectionMatrix,
										   const Imath::M44f& projectionInverse,
										   const Camera& camera)
{
    glUseProgram(m_program);

    glUniform1f(m_uniformCameraNear, camera.parameters().nearDistance());
	glUniform1f(m_uniformCameraFar, camera.parameters().farDistance());

    glUniformMatrix4fv(m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &modelViewInverse.x[0][0]);

    glUniformMatrix4fv(m_uniformCameraProj,
					   1,
                       GL_TRUE,
					   &projectionMatrix.x[0][0]);

    glUniformMatrix4fv(m_uniformCameraInverseProj,
					   1,
                       GL_TRUE,
					   &projectionInverse.x[0][0]);
	
	glUniform1f(m_uniformCameraFocalLength, camera.parameters().focalLength());

    glUseProgram(0);
}

void RendererServicePicking::frameResized(int viewport[4])
{
    glUseProgram(m_program);

	glUniform4f(m_uniformViewport, 
				(float)viewport[0], 
				(float)viewport[1], 
				(float)viewport[2],
				(float)viewport[3]);

    glUseProgram(0);
}

void RendererServicePicking::volumeReloaded(const Imath::V3i& volumeResolution,
											const Imath::Box3f& volumeBounds)
{
    glUseProgram(m_program);

	glUniform3i(m_uniformVoxelDataResolution, 
				volumeResolution.x, 
				volumeResolution.y, 
				volumeResolution.z);

	glUniform3f(m_uniformVolumeBoundsMin,
				volumeBounds.min.x,
				volumeBounds.min.y,
				volumeBounds.min.z);

	glUniform3f(m_uniformVolumeBoundsMax,
				volumeBounds.max.x,
				volumeBounds.max.y,
				volumeBounds.max.z);

    glUseProgram(0);
}

void RendererServicePicking::execute()
{
    glUseProgram(m_program);
	
	glUniform2f(m_uniformSampledFragment, m_point.x, m_point.y);

	runVertexShader();	

    glUseProgram(0);
}

