#pragma once

#include <GL/gl.h>
#include "renderer/services/service.h"

#include <OpenEXR/ImathMatrix.h>

class RendererServiceRemoveVoxel : public RendererService
{
public:
	RendererServiceRemoveVoxel(Renderer* renderer) : RendererService(renderer) {}

	virtual bool reload(const std::string& shaderPath, Logger* logger);

	virtual void glResourcesCreated(const GLResourceConfiguration& glResources);

	virtual void execute();

private:
	// shader program
	GLuint m_program;

	// uniforms
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
};

