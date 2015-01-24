#pragma once

#include <OpenEXR/ImathVec.h>

class CameraParameters;
class CameraController
{
public:
	CameraController(CameraParameters* parameters) : m_parameters(parameters) {}
	virtual bool onMouseMove(float dx, float dy, int buttons) = 0;

    // set target at supplied location, and move eye position proportionally to
    // the current orientation and distance to camera.
    void centerAt(const Imath::V3f& target);

    void setDistanceToTarget(float distance);

protected:
	CameraParameters* m_parameters;
};
