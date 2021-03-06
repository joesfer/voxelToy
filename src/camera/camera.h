#pragma once

#include "camera/cameraParameters.h"
#include "camera/cameraParameters.h"

class CameraController;

class Camera
{
public:
	Camera();
	~Camera();

	const CameraParameters& parameters() const { return m_parameters; }
	CameraController& controller() const { return *m_controller; }

	void setLensModel(CameraParameters::CameraLensModel);
	void setFocalLength(float length);
	void setFocalDistance(float distance);
	void setFilmSize(float filmW, float filmH);
	void setLensRadius(float radius);
	void setFStop(float fstop);

	enum CameraControllerMode
	{
		CCM_ORBIT,
		CCM_FLY
	};
	void setCameraController(CameraControllerMode);

private:
	CameraParameters  m_parameters;
	CameraController* m_controller;
	CameraControllerMode m_controllerMode;
};
