#include "camera/orbitCameraController.h"
#include "camera/cameraParameters.h"
#include <stdlib.h>
#include <math.h>

// this is still needed for the mouse buttons, but the dependency with Qt 
// should be removed from the renderer
#include <QtGui> 

OrbitCameraController::OrbitCameraController(CameraParameters* parameters)
	: CameraController(parameters)
{
}

bool OrbitCameraController::onMouseMove(float dx, float dy, int buttons)
{
	if ( m_parameters == NULL ) return false;

    bool change = false;
    if (buttons & Qt::RightButton)
    {
        const float speed = 1.f;
        const float theta = (float)dy * M_PI * speed + m_parameters->rotationTheta();
        const float phi = -(float)dx * 2.0f * M_PI * speed + m_parameters->rotationPhi();

        m_parameters->setOrbitRotation(theta, phi);
		change = true;
    }
    else if( buttons & Qt::MiddleButton)
    {
        const float speed = 1.05f;
        m_parameters->setDistanceFromTarget( dy > 0 ? m_parameters->distanceToTarget() * speed :
												    m_parameters->distanceToTarget() / speed );
		change = true;
    }
	return change;
}

bool OrbitCameraController::onKeyPress(int key)
{
	return false;
}

