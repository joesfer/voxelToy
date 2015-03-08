#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/services/serviceSetFocalDistance.h"
#include "renderer/glResources.h"

bool RendererServiceSetFocalDistance::reload(const std::string& shaderPath, Logger* logger)
{
	return reloadShader(shaderPath, 
						"FocalDistance", 
						"focalDistance/focalDistance.vs", 
						"shared/trivial.fs", 
						logger);
}

void RendererServiceSetFocalDistance::glResourcesCreated(const GLResourceConfiguration& glResources)
{
	glUseProgram(m_program);

	m_uniformSSBOStorageBlock = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "FocalDistanceData");
	glShaderStorageBlockBinding(m_program, m_uniformSSBOStorageBlock, GLResourceConfiguration::m_focalDistanceSSBOBindingPointIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLResourceConfiguration::m_focalDistanceSSBOBindingPointIndex, glResources.m_focalDistanceSSBO);

	glUseProgram(0);
}


