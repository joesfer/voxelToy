#pragma once

#include "src/camera/cameraController.h"

class OrbitCameraController : public CameraController
{
public:
	OrbitCameraController(CameraParameters* parameters);
	virtual bool onMouseMove(float dx, float dy, int buttons);
private:
};



