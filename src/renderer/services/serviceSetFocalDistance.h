#pragma once

#include "renderer/services/servicePicking.h"

class RendererServiceSetFocalDistance : public RendererServicePicking
{
public:
	RendererServiceSetFocalDistance(Renderer* renderer) : RendererServicePicking(renderer) {}

	virtual bool reload(const std::string& shaderPath, Logger* logger);

	virtual void glResourcesCreated(const GLResourceConfiguration& glResources);

};

