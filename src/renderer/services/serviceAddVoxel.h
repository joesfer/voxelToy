#pragma once

#include <GL/gl.h>
#include "renderer/services/service.h"

#include <OpenEXR/ImathMatrix.h>

class RendererServiceAddVoxel : public RendererService
{
public:
	RendererServiceAddVoxel(Renderer* renderer) : RendererService(renderer) {}

	virtual bool reload(const std::string& shaderPath, Logger* logger);

	virtual void glResourcesCreated(const GLResourceConfiguration& glResources);

	virtual void cameraUpdated(const Imath::M44f& /*modelViewMatrix*/,
							   const Imath::M44f& /*modelViewInverse*/,
							   const Imath::M44f& /*projectionMatrix*/,
							   const Imath::M44f& /*projectionInverse*/,
							   const Camera& /*camera*/);

	virtual void execute();

private:
	// shader program
	GLuint m_program;

	// uniforms
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformScreenSpaceMotion;
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformVoxelColorTexture;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
	GLuint m_uniformNewVoxelColor;
};
