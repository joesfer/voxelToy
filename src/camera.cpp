#include <camera.h>

#include <OpenEXR/ImathMatrixAlgo.h>
#include <math.h>

// shorthand for a 35 milimiter (36 * 24mm) frame sensor
float Camera::FILM_SIZE_35MM = 36.0f;

Camera::Camera() :
    m_target(Imath::V3f(0,0,0)),
    m_targetDistance(1.0f),
    m_phi(-M_PI/2),
    m_theta(M_PI/2),
	m_near(0.1f),
	m_far(10000),
	m_focalDistance(100),
	m_lensRadius(0),
	m_filmSize(36)
{
	this->setFocalLength(50);
}

Imath::V3f Camera::eye() const
{
    return m_target - forwardUnitVector() * m_targetDistance;
}

Imath::V3f Camera::target() const
{
	return m_target;
}
void Camera::centerAt(const Imath::V3f &target)
{
    m_target = target;
}

float Camera::distanceToTarget() const
{
    return m_targetDistance;
}

void Camera::setDistanceToTarget(float distance)
{
    m_targetDistance = std::max(0.f, distance);
}

Imath::V3f Camera::rightUnitVector() const
{
	return Imath::V3f(cos(m_phi + M_PI/2),
					  0,
					  sin(m_phi + M_PI/2));
} 

Imath::V3f Camera::upUnitVector() const
{
    return forwardUnitVector().cross(rightUnitVector());
}

Imath::V3f Camera::forwardUnitVector() const
{
	using namespace Imath;
	const float sinTheta = sin(m_theta);
	V3f targetToEye( sinTheta * cos(m_phi),
					 cos(m_theta),
					 sinTheta * sin(m_phi) );
	V3f forward = -targetToEye;
    return forward.normalized();
}

float Camera::rotationTheta() const
{
	return m_theta;
}

float Camera::rotationPhi() const
{
	return m_phi;
}

void Camera::setOrbitRotation(float theta, float phi)
{
	m_phi = phi;
	m_theta = theta;
}

float Camera::fovY() const
{ 
	return m_fovY;
}

void Camera::setFovY(float fov)
{
	m_fovY = fov;
	m_focalLength = filmSize().y / (2.0f * tan(0.5f * m_fovY));
}

float Camera::nearDistance() const
{
	return m_near;	
}
void Camera::setNearDistance(float d)
{
	m_near = std::max(0.0f, d);
}
float Camera::farDistance() const
{
	return m_far;	
}
void Camera::setFarDistance(float d)
{
	m_far = std::max(m_near, d);
}

float Camera::focalLength() const
{
	return m_focalLength;
}
void Camera::setFocalLength(float length)
{
	m_focalLength = std::max(0.0f, length);
	m_fovY = atan2(this->filmSize().y * 0.5f, m_focalLength) * 2.0f;
}

float Camera::focalDistance() const
{
	return m_focalDistance;
}
void Camera::setFocalDistance(float distance)
{
    m_focalDistance = std::max(0.0f, distance);
}

Imath::V2f Camera::filmSize() const
{
	return m_filmSize;
}
void Camera::setFilmSize(float filmW, float filmH)
{
	m_filmSize = Imath::V2f(std::max(0.0f, filmW),
							std::max(0.0f, filmH));
	m_fovY = atan2(this->filmSize().y * 0.5f, m_focalLength) * 2.0f;
}

float Camera::lensRadius() const
{
	return m_lensRadius;
}
void Camera::setLensRadius(float radius)
{
	m_lensRadius = std::max(0.0f, radius);
}
void Camera::setFStop(float fstop)
{
	m_lensRadius = (m_focalLength / std::max(1e-4f, fstop)) * 0.5f;
}

Camera::CameraLensModel Camera::lensModel() const
{
	return m_lensModel;
}
void Camera::setLensModel(Camera::CameraLensModel model)
{
	m_lensModel = model;
}

