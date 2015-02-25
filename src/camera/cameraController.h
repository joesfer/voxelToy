#pragma once

#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathBox.h>

class CameraParameters;
class CameraController
{
public:
	virtual ~CameraController() {}

	CameraController(CameraParameters* parameters) : m_parameters(parameters) {}
	virtual bool onMouseMove(float dx, float dy, int buttons) = 0;
	virtual bool onKeyPress(int key) = 0;

    // set target at supplied location, and move eye position proportionally to
    // the current orientation and distance to camera.
    void lookAt(const Imath::V3f& target);

    void setDistanceFromTarget(float distance);

	void focusOnBounds(const Imath::Box3f& bounds);

protected:
	CameraParameters* m_parameters;
};
