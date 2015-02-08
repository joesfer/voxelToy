#pragma once

#include "camera/cameraController.h"

class FlyCameraController : public CameraController
{
public:
	FlyCameraController(CameraParameters* parameters);
	virtual bool onMouseMove(float dx, float dy, int buttons);
	virtual bool onKeyPress(int key);
private:
};




