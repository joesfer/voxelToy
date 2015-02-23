#include "camera/camera.h"
#include "camera/orbitCameraController.h"
#include "camera/flyCameraController.h"
#include <assert.h>

Camera::Camera()
{
	m_controller = new OrbitCameraController(&m_parameters);
	m_controllerMode = CCM_ORBIT;
}
Camera::~Camera()
{
	delete m_controller;
}
void Camera::setLensModel(CameraParameters::CameraLensModel model)
{
	m_parameters.setLensModel(model); 
}

void Camera::setFocalLength(float length)          { m_parameters.setFocalLength(length);        }
void Camera::setFocalDistance(float distance)      { m_parameters.setFocalDistance(distance);    }
void Camera::setFilmSize(float filmW, float filmH) { m_parameters.setFilmSize(filmW, filmH);     }
void Camera::setLensRadius(float radius)           { m_parameters.setLensRadius(radius);         }
void Camera::setFStop(float fstop)                 { m_parameters.setFStop(fstop);               }

void Camera::setCameraController(CameraControllerMode mode)
{
	if (m_controllerMode == mode) return;
	m_controllerMode = mode;
	switch(m_controllerMode)
	{
		case CCM_ORBIT: m_controller = new OrbitCameraController(&m_parameters); break;
		case CCM_FLY:   m_controller = new FlyCameraController(&m_parameters);   break;
		default: assert(false); break;
	}
}


