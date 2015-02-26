#include "camera/cameraParameters.h"
#include "camera/camera.h"

// shorthand for a 35 milimiter (36 * 24mm) frame sensor
float CameraParameters::FILM_SIZE_35MM = 36.0f;

CameraParameters::CameraParameters() :
    m_target(Imath::V3f(0,0,0)),
    m_eye(Imath::V3f(0,0,-1)),
	m_near(0.1f),
	m_far(10000),
	m_focalDistance(100),
	m_lensRadius(0),
	m_filmSize(36)
{
	this->setFocalLength(50);
}

const Imath::V3f& CameraParameters::eye() const
{
    return m_eye;
}

const Imath::V3f& CameraParameters::target() const
{
	return m_target;
}
void CameraParameters::lookAt(const Imath::V3f &target)
{
    m_target = target;
}

float CameraParameters::distanceToTarget() const
{
    return (m_target - m_eye).length();
}

void CameraParameters::setDistanceFromTarget(float distance)
{
	m_eye = m_target - forwardUnitVector() * distance;
}

void CameraParameters::getBasis(Imath::V3f& forwardUnitVector,
							    Imath::V3f& rightUnitVector,
							    Imath::V3f& upUnitVector) const
{
	forwardUnitVector = (m_target - m_eye).normalized();
	rightUnitVector = Imath::V3f(0,1,0).cross(forwardUnitVector).normalized();
	upUnitVector = forwardUnitVector.cross(rightUnitVector);
}

Imath::V3f CameraParameters::forwardUnitVector() const
{
	Imath::V3f right, up, forward;
	getBasis(forward, right, up);
	return forward;
}

Imath::V3f CameraParameters::rightUnitVector() const
{
	Imath::V3f right, up, forward;
	getBasis(forward, right, up);
	return right;
}

Imath::V3f CameraParameters::upUnitVector() const
{
	Imath::V3f right, up, forward;
	getBasis(forward, right, up);
	return up;
}

float CameraParameters::rotationTheta() const
{
	return acos(forwardUnitVector().y);
}

float CameraParameters::rotationPhi() const
{
	const Imath::V3f fwd = forwardUnitVector();
	return atan2(fwd.z, fwd.x);
}

Imath::V3f sphericalToCartesian(float theta, float phi)
{
	const float sinTheta = sin(theta);
	return Imath::V3f(sinTheta * cos(phi), cos(theta), sinTheta * sin(phi));
}

void CameraParameters::orbitAroundTarget(float theta, float phi)
{
	const float r = distanceToTarget();
	m_eye = m_target - r * sphericalToCartesian(theta, phi);
}

void CameraParameters::orbitAroundEye(float theta, float phi)
{
	const float r = distanceToTarget();
	m_target = m_eye + r * sphericalToCartesian(theta, phi);
}

void CameraParameters::setEyeTarget(const Imath::V3f& eye, const Imath::V3f& target)
{
	m_eye = eye;
	m_target = target;
}

float CameraParameters::fovY() const
{ 
	return m_fovY;
}

void CameraParameters::setFovY(float fov)
{
	m_fovY = fov;
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
	return filmSize().y / (2.0f * tan(0.5f * m_fovY));
}
void CameraParameters::setFocalLength(float focalLength)
{
	m_fovY = atan2(this->filmSize().y * 0.5f, focalLength) * 2.0f;
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
	m_fovY = atan2(this->filmSize().y * 0.5f, focalLength()) * 2.0f;
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
	m_lensRadius = (focalLength() / std::max(1e-4f, fstop)) * 0.5f;
}

CameraParameters::CameraLensModel CameraParameters::lensModel() const
{
	return m_lensModel;
}
void CameraParameters::setLensModel(CameraParameters::CameraLensModel model)
{
	m_lensModel = model;
}


