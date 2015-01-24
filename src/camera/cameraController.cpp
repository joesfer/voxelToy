#include "camera/cameraController.h"
#include "camera/cameraParameters.h"

void CameraController::centerAt(const Imath::V3f& target)    
{ 
	if (m_parameters == NULL) return;
	m_parameters->centerAt(target);              
}

void CameraController::setDistanceToTarget(float distance)
{ 
	if (m_parameters == NULL) return;
	m_parameters->setDistanceToTarget(distance);
}

