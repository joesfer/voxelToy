#include "camera/cameraParameters.h"
#include "camera/camera.h"

// shorthand for a 35 milimiter (36 * 24mm) frame sensor
float CameraParameters::FILM_SIZE_35MM = 36.0f;

CameraParameters::CameraParameters() :
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

Imath::V3f CameraParameters::eye() const
{
    return m_target - forwardUnitVector() * m_targetDistance;
}

Imath::V3f CameraParameters::target() const
{
	return m_target;
}
void CameraParameters::centerAt(const Imath::V3f &target)
{
    m_target = target;
}

float CameraParameters::distanceToTarget() const
{
    return m_targetDistance;
}

void CameraParameters::setDistanceToTarget(float distance)
{
    m_targetDistance = std::max(0.f, distance);
}

Imath::V3f CameraParameters::rightUnitVector() const
{
	return Imath::V3f(cos(m_phi + M_PI/2),
					  0,
					  sin(m_phi + M_PI/2));
} 

Imath::V3f CameraParameters::upUnitVector() const
{
    return forwardUnitVector().cross(rightUnitVector());
}

Imath::V3f CameraParameters::forwardUnitVector() const
{
	using namespace Imath;
	const float sinTheta = sin(m_theta);
	V3f targetToEye( sinTheta * cos(m_phi),
					 cos(m_theta),
					 sinTheta * sin(m_phi) );
	V3f forward = -targetToEye;
    return forward.normalized();
}

float CameraParameters::rotationTheta() const
{
	return m_theta;
}

float CameraParameters::rotationPhi() const
{
	return m_phi;
}

void CameraParameters::setOrbitRotation(float theta, float phi)
{
	m_phi = phi;
	m_theta = theta;
}

float CameraParameters::fovY() const
{ 
	return m_fovY;
}

void CameraParameters::setFovY(float fov)
{
	m_fovY = fov;
	m_focalLength = filmSize().y / (2.0f * tan(0.5f * m_fovY));
}

float CameraParameters::nearDistance() const
{
	return m_near;	
}
void CameraParameters::setNearDistance(float d)
{
	m_near = std::max(0.0f, d);
}
float CameraParameters::farDistance() const
{
	return m_far;	
}
void CameraParameters::setFarDistance(float d)
{
	m_far = std::max(m_near, d);
}

float CameraParameters::focalLength() const
{
	return m_focalLength;
}
void CameraParameters::setFocalLength(float length)
{
	m_focalLength = std::max(0.0f, length);
	m_fovY = atan2(this->filmSize().y * 0.5f, m_focalLength) * 2.0f;
}

float CameraParameters::focalDistance() const
{
	return m_focalDistance;
}
void CameraParameters::setFocalDistance(float distance)
{
    m_focalDistance = std::max(0.0f, distance);
}

Imath::V2f CameraParameters::filmSize() const
{
	return m_filmSize;
}
void CameraParameters::setFilmSize(float filmW, float filmH)
{
	m_filmSize = Imath::V2f(std::max(0.0f, filmW),
							std::max(0.0f, filmH));
	m_fovY = atan2(this->filmSize().y * 0.5f, m_focalLength) * 2.0f;
}

float CameraParameters::lensRadius() const
{
	return m_lensRadius;
}
void CameraParameters::setLensRadius(float radius)
{
	m_lensRadius = std::max(0.0f, radius);
}
void CameraParameters::setFStop(float fstop)
{
	m_lensRadius = (m_focalLength / std::max(1e-4f, fstop)) * 0.5f;
}

CameraParameters::CameraLensModel CameraParameters::lensModel() const
{
	return m_lensModel;
}
void CameraParameters::setLensModel(CameraParameters::CameraLensModel model)
{
	m_lensModel = model;
}


