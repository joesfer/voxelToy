#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/services/serviceSelectActiveVoxel.h"
#include "renderer/glResources.h"

bool RendererServiceSelectActiveVoxel::reload(const std::string& shaderPath, Logger* logger)
{
	return reloadShader(shaderPath, 
						"SelectActiveVoxel",
						"editVoxels/selectVoxel.vs",
						"shared/trivial.fs",
						logger);
}

void RendererServiceSelectActiveVoxel::glResourcesCreated(const GLResourceConfiguration& glResources)
{
	glUseProgram(m_program);

	m_uniformSSBOStorageBlock = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SelectVoxelData");
	glShaderStorageBlockBinding(m_program, m_uniformSSBOStorageBlock, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_selectedVoxelSSBOBindingPointIndex, glResources.m_selectedVoxelSSBO);

	glUseProgram(0);
}


