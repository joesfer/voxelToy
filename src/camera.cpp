#include <camera.h>

#include <OpenEXR/ImathMatrixAlgo.h>
#include <math.h>

Camera::Camera() :
    m_target(Imath::V3f(0,0,0)),
    m_targetDistance(1.0f),
    m_phi(-M_PI/2),
    m_theta(M_PI/2),
    m_fovY(M_PI/2),
	m_near(0.1f),
	m_far(10000)
{
}

Imath::V3f Camera::eye() const
{
    return m_target - forwardUnitVector() * m_targetDistance;
}

Imath::V3f Camera::target() const
{
	return m_target;
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

