#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/glResources.h"
#include "renderer/services/serviceAddVoxel.h"
#include "renderer/renderer.h"
#include "log/logger.h"
#include "shaders/shader.h"

bool RendererServiceAddVoxel::reload(const std::string& shaderPath, Logger* logger)
{
	std::string vs = shaderPath + std::string("editVoxels/addVoxel.vs");
	std::string fs = shaderPath + std::string("shared/trivial.fs");

    if ( !Shader::compileProgramFromFile("AddVoxel",
										 shaderPath,
                                         vs, "",
                                         fs, "",
                                         m_program,
										 logger) )
	{
		return false;
	}
    
	glUseProgram(m_program);

	m_uniformCameraInverseModelView  = glGetUniformLocation(m_program, "cameraInverseModelView");
	m_uniformScreenSpaceMotion       = glGetUniformLocation(m_program, "screenSpaceMotion");
	m_uniformVoxelOccupancyTexture   = glGetUniformLocation(m_program, "voxelOccupancy");
	m_uniformVoxelColorTexture       = glGetUniformLocation(m_program, "voxelColor");
	m_uniformNewVoxelColor           = glGetUniformLocation(m_program, "newVoxelColor");
	
	glUniform1i(m_uniformVoxelOccupancyTexture, GLResourceConfiguration::TEXTURE_UNIT_OCCUPANCY);
	glUniform1i(m_uniformVoxelColorTexture, GLResourceConfiguration::TEXTURE_UNIT_COLOR);

	glUseProgram(0);

	return true;
}

void RendererServiceAddVoxel::glResourcesCreated(const GLResourceConfiguration& glResources)
{
	glUseProgram(m_program);

	m_uniformSelectedVoxelSSBOStorageBlock = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SelectVoxelData");
	glShaderStorageBlockBinding(m_program, m_uniformSelectedVoxelSSBOStorageBlock, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex, glResources.m_selectedVoxelSSBO);

	glUseProgram(0);
}


void RendererServiceAddVoxel::cameraUpdated(const Imath::M44f& /*modelViewMatrix*/,
										    const Imath::M44f& modelViewInverse,
										    const Imath::M44f& /*projectionMatrix*/,
										    const Imath::M44f& /*projectionInverse*/,
										    const Camera& /*camera*/)
{
    glUseProgram(m_program);

    glUniformMatrix4fv(m_uniformCameraInverseModelView,
                       1,
                       GL_TRUE,
                       &modelViewInverse.x[0][0]);
	
    glUseProgram(0);
}

void RendererServiceAddVoxel::execute()
{
    glUseProgram(m_program);
	
	// m_screenFocal point origin is at top-left, whilst glsl is bottom-left
	glUniform2f(m_uniformScreenSpaceMotion,
				m_velocity.x , -m_velocity.y ); 

	runVertexShader();	

    glUseProgram(0);
}
