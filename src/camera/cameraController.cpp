#include "camera/cameraController.h"
#include "camera/cameraParameters.h"

void CameraController::lookAt(const Imath::V3f& target)    
{ 
	if (m_parameters == NULL) return;
	m_parameters->lookAt(target);              
}

void CameraController::setDistanceFromTarget(float distance)
{ 
	if (m_parameters == NULL) return;
	m_parameters->setDistanceFromTarget(distance);
}

void CameraController::focusOnBounds(const Imath::Box3f& bounds)
{
	using namespace Imath;
	V3f fwd = m_parameters->forwardUnitVector();
	V3f center = bounds.center();
	float distance = bounds.size().length() / (2.0f * tan(m_parameters->fovY() / 2));
	m_parameters->setEyeTarget(center - fwd * distance, center);
}

