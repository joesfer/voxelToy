#include "src/camera/camera.h"
#include "src/camera/orbitCameraController.h"

Camera::Camera()
{
	m_controller = new OrbitCameraController(&m_parameters);
}
Camera::~Camera()
{
	delete m_controller;
}
void Camera::enableDOF(bool enable)
{
	m_parameters.setLensModel( enable ? 
								CameraParameters::CLM_THIN_LENS :
								CameraParameters::CLM_PINHOLE);
}

void Camera::setFocalLength(float length)          { m_parameters.setFocalLength(length);        }
void Camera::setFocalDistance(float distance)      { m_parameters.setFocalDistance(distance);    }
void Camera::setFilmSize(float filmW, float filmH) { m_parameters.setFilmSize(filmW, filmH);     }
void Camera::setLensRadius(float radius)           { m_parameters.setLensRadius(radius);         }
void Camera::setFStop(float fstop)                 { m_parameters.setFStop(fstop);               }
void Camera::centerAt(const Imath::V3f& target)    { m_parameters.centerAt(target);              }
void Camera::setDistanceToTarget(float distance)   { m_parameters.setDistanceToTarget(distance); }

