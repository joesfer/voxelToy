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

Imath::V3f CameraParameters::rightUnitVector() const
{
	const float phi = rotationPhi();
	const float theta0 = rotationTheta();
	const float theta1 = rotationTheta() - 0.1f;
	const float sinTheta0 = sin(theta0);
	const float sinTheta1 = sin(theta1);

	const Imath::V3f v0(sinTheta0 * cos(phi), cos(theta0), sinTheta0 * sin(phi));
	const Imath::V3f v1(sinTheta1 * cos(phi), cos(theta1), sinTheta1 * sin(phi));

    return v1.cross(v0).normalized();
} 

Imath::V3f CameraParameters::upUnitVector() const
{
	const float phi0 = rotationPhi();
	const float phi1 = rotationPhi() + 0.1f;
	const float theta = rotationTheta();
	const float sinTheta = sin(theta);

	const Imath::V3f v0(sinTheta * cos(phi0), cos(theta), sinTheta * sin(phi0));
	const Imath::V3f v1(sinTheta * cos(phi1), cos(theta), sinTheta * sin(phi1));

    return v1.cross(v0).normalized();
}

Imath::V3f CameraParameters::forwardUnitVector() const
{
	return (m_target - m_eye).normalized();
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

void CameraParameters::setOrbitRotation(float theta, float phi)
{
	const float r = distanceToTarget();
	const float sinTheta = sin(theta);
	Imath::V3f fwd(r * sinTheta * cos(phi),
				   r * cos(theta),
				   r * sinTheta * sin(phi));
	m_eye = m_target - fwd;
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
	std::cout << "FOV " << m_fovY << std::endl;
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


