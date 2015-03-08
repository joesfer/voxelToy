#pragma once

#include "renderer/services/servicePicking.h"

class RendererServiceSelectActiveVoxel : public RendererServicePicking
{
public:
	RendererServiceSelectActiveVoxel(Renderer* renderer) : RendererServicePicking(renderer) {}

	virtual bool reload(const std::string& shaderPath, Logger* logger);

	virtual void glResourcesCreated(const GLResourceConfiguration& glResources);

};


