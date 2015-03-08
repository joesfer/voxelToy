#pragma once

#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathBox.h>
#include <string>

class Renderer;
class Logger;
class Camera;
struct GLResourceConfiguration;

class RendererService
{
public:
	virtual ~RendererService();

	virtual bool reload(const std::string& shaderPath, Logger* logger) = 0;

	// inform services that the gl resources have been created. This information
	// may be required to fully initialize the shaders, but it's optional and
	// the default implementation does nothing.
	virtual void glResourcesCreated(const GLResourceConfiguration& /*glResources*/) {};

	// inform services that the camera has changed. Only some implementations
	// may care about this, so by default the method does nothing
	virtual void cameraUpdated(const Imath::M44f& /*modelViewMatrix*/,
	                           const Imath::M44f& /*modelViewInverse*/,
	                           const Imath::M44f& /*projectionMatrix*/,
	                           const Imath::M44f& /*projectionInverse*/,
	                           const Camera& /*camera*/) {}

	// inform services that the frame has been resized. Only some
	// implementations may care about this, so by default the method does
	// nothing.
	virtual void frameResized(int /*viewport*/[4]) {}

	// inform services that the voxel data has been reloaded. 
	virtual void volumeReloaded(const Imath::V3i& /*volumeResolution*/,
								const Imath::Box3f& /*volumeBounds*/) {}

	virtual void setMouseParameters(Imath::V2f& point,
									Imath::V2f& velocity)
	{
		m_point = point;
		m_velocity = velocity;
	}

	// execute service functionality. 
	virtual void execute() = 0;

protected:
	// Utility function used to run the vertex shader associated to each
	// service. This is a simple 1-vertex draw call with no rasterization.
	void runVertexShader();

protected:
	// shader program
	GLuint m_program;

	// Normalized coordinates in screen space from where the requested picking
	// action is originated (the actual pixel coordinates are calculated as
	// m_pickingActionPoint * viewportSize;
	Imath::V2f m_point;
	// screen-space normalized movement 
	Imath::V2f m_velocity;
};

// TODO: should have a proper registration mechanism
enum RendererServiceType
{
	SERVICE_ADD_VOXEL = 0,
	SERVICE_REMOVE_VOXEL,
	SERVICE_SELECT_ACTIVE_VOXEL,
	SERVICE_SET_FOCAL_DISTANCE,
	SERVICE_TOTAL // keep last
};
