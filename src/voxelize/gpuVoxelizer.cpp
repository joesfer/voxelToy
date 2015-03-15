#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>

#include "voxelize/gpuVoxelizer.h"
#include "log/logger.h"
#include "shaders/shader.h"
#include "mesh/mesh.h"

GPUVoxelizer::GPUVoxelizer(const std::string& shaderPath,
						   Logger* logger)
{
	m_initialized = false;

	std::string vs = shaderPath + std::string("shared/voxelize.vs");
	std::string gs = shaderPath + std::string("shared/voxelize.gs");
	std::string fs = shaderPath + std::string("shared/trivial.fs");

	if ( !Shader::compileProgramFromFile("Voxelize",
										 shaderPath,
										 vs, "",
										 gs, "",
										 fs, "",
										 m_program,
										 logger) )
	{
		return;
	}
	
	glUseProgram(m_program);

	m_uniformMaterialOffsetTexture = glGetUniformLocation(m_program, "materialOffsetTexture");
	m_uniformVoxelDataResolution   = glGetUniformLocation(m_program, "voxelResolution");
	m_uniformModelTransform        = glGetUniformLocation(m_program, "modelTransform");

	glUseProgram(0);

	m_initialized = true;
}

GPUVoxelizer::~GPUVoxelizer()
{
	if (!m_initialized) return;
	glDeleteProgram(m_program);
}

bool GPUVoxelizer::voxelizeMesh(const Mesh* mesh,
								const Imath::M44f& meshTransform,
								const Imath::V3i& resolution,
								GLuint textureUnit)
{
	if (!m_initialized) return false;

	glUseProgram(m_program);

	glUniform1i(m_uniformMaterialOffsetTexture, textureUnit);
	
	glUniform3i(m_uniformVoxelDataResolution, 
				resolution.x, 
				resolution.y, 
				resolution.z);

	glUniformMatrix4fv(m_uniformModelTransform,
					   1,
					   GL_TRUE,
					   &meshTransform.x[0][0]);

	mesh->draw();

	glUseProgram(0);

	return true;
}

