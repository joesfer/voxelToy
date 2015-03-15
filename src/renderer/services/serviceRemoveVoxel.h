#pragma once

#include <GL/gl.h>
#include "renderer/services/service.h"

#include <OpenEXR/ImathMatrix.h>

class RendererServiceRemoveVoxel : public RendererService
{
public:
	virtual bool reload(const std::string& shaderPath, Logger* logger);

	virtual void glResourcesCreated(const GLResourceConfiguration& glResources);

	virtual void execute();

private:
	// uniforms
	GLuint m_uniformMaterialOffsetTexture;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
};

