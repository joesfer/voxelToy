#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/glResources.h"
#include "renderer/services/serviceRemoveVoxel.h"
#include "renderer/renderer.h"
#include "log/logger.h"
#include "shaders/shader.h"

bool RendererServiceRemoveVoxel::reload(const std::string& shaderPath, Logger* logger)
{
	std::string vs = shaderPath + std::string("editVoxels/removeVoxel.vs");
	std::string fs = shaderPath + std::string("shared/trivial.fs");

    if ( !Shader::compileProgramFromFile("RemoveVoxel",
										 shaderPath,
                                         vs, "#version 430\n",
                                         fs, "#version 430\n",
                                         m_program,
										 logger) )
	{
		return false;
	}
    
	glUseProgram(m_program);

	m_uniformMaterialOffsetTexture   = glGetUniformLocation(m_program, "materialOffsetTexture");
	
	glUniform1i(m_uniformMaterialOffsetTexture, GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_OFFSET);

	glUseProgram(0);

	return true;
}

void RendererServiceRemoveVoxel::glResourcesCreated(const GLResourceConfiguration& glResources)
{
	glUseProgram(m_program);

	m_uniformSelectedVoxelSSBOStorageBlock = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SelectVoxelData");
	glShaderStorageBlockBinding(m_program, m_uniformSelectedVoxelSSBOStorageBlock, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex, glResources.m_selectedVoxelSSBO);

	glUseProgram(0);
}

void RendererServiceRemoveVoxel::execute()
{
    glUseProgram(m_program);
	
	runVertexShader();

    glUseProgram(0);
}

