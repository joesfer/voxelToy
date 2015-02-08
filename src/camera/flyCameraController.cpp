#include "camera/flyCameraController.h"
#include "camera/cameraParameters.h"
#include <stdlib.h>
#include <math.h>

// this is still needed for the mouse buttons, but the dependency with Qt 
// should be removed from the renderer
#include <QtGui> 

FlyCameraController::FlyCameraController(CameraParameters* parameters)
	: CameraController(parameters)
{
}

bool FlyCameraController::onMouseMove(float dx, float dy, int buttons)
{
	if ( m_parameters == NULL ) return false;

    bool change = false;
    if (buttons & Qt::RightButton)
    {
        const float speed = 1.f;
        const float theta = (float)dy * M_PI * speed + m_parameters->rotationTheta();
        const float phi = -(float)dx * 2.0f * M_PI * speed + m_parameters->rotationPhi();

        m_parameters->orbitAroundEye(theta, phi);
		return true;
    }
#ifdef QT5
    else if( buttons & Qt::MiddleButton)
#else
    else if( buttons & Qt::MidButton)
#endif
    {
		const float speed = 100;
		Imath::V3f up = dy * m_parameters->upUnitVector() * speed;
		m_parameters->setEyeTarget(m_parameters->eye() + up, m_parameters->target() + up);	
		return true;
    }
	return false;
}

bool FlyCameraController::onKeyPress(int key)
{
	const float speed = 100;
	switch(key)
	{
		case Qt::Key_W:
		{
			Imath::V3f fwd = m_parameters->forwardUnitVector() * speed;
			m_parameters->setEyeTarget(m_parameters->eye() + fwd, m_parameters->target() + fwd);	
			return true;
		}
		case Qt::Key_S:
		{
			Imath::V3f fwd = m_parameters->forwardUnitVector() * speed;
			m_parameters->setEyeTarget(m_parameters->eye() - fwd, m_parameters->target() - fwd);	
			return true;
		}
		case Qt::Key_D:
		{
			Imath::V3f right = m_parameters->rightUnitVector() * speed;
			m_parameters->setEyeTarget(m_parameters->eye() + right, m_parameters->target() + right);	
			return true;
		}
		case Qt::Key_A:
		{
			Imath::V3f right = m_parameters->rightUnitVector() * speed;
			m_parameters->setEyeTarget(m_parameters->eye() - right, m_parameters->target() - right);	
			return true;
		}
		default: return false;
	}
}

