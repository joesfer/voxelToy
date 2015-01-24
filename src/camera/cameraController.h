#pragma once

class CameraParameters;
class CameraController
{
public:
	CameraController(CameraParameters* parameters) : m_parameters(parameters) {}
	virtual bool onMouseMove(float dx, float dy, int buttons) = 0;
protected:
	CameraParameters* m_parameters;
};
