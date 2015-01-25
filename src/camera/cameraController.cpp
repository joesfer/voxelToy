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

