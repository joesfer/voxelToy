#pragma once

#include <GL/gl.h>
#include "renderer/services/service.h"

#include <OpenEXR/ImathMatrix.h>

class RendererServicePicking : public RendererService
{
public:
	virtual void cameraUpdated(const Imath::M44f& /*modelViewMatrix*/,
							   const Imath::M44f& /*modelViewInverse*/,
							   const Imath::M44f& /*projectionMatrix*/,
							   const Imath::M44f& /*projectionInverse*/,
							   const Camera& /*camera*/);

	virtual void frameResized(int viewport[4]);
	
	// inform services that the voxel data has been reloaded. 
	virtual void volumeReloaded(const Imath::V3i& volumeResolution,
								const Imath::Box3f& volumeBounds);

	virtual void execute();

protected:
	virtual bool reloadShader(const std::string& shaderPath, 
							  const std::string& shaderName,
							  const std::string& vertexShaderName,
							  const std::string& fragmentShaderName,
							  Logger* logger);
protected:
	// uniforms
	GLuint m_uniformMaterialOffsetTexture;
	GLuint m_uniformVoxelDataResolution;
	GLuint m_uniformVolumeBoundsMin;
	GLuint m_uniformVolumeBoundsMax;
	GLuint m_uniformViewport;
	GLuint m_uniformCameraNear;
	GLuint m_uniformCameraFar;
	GLuint m_uniformCameraProj;
	GLuint m_uniformCameraInverseProj;
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformCameraFocalLength;
	GLuint m_uniformSampledFragment;
	GLuint m_uniformSSBOStorageBlock;
};


