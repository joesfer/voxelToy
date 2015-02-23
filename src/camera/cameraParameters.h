#pragma once

#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathMatrix.h>

//                target
//               ,
//              ,
//      (up)
//        y   z (forward)
//        |  /
//        | /
//        |/
//        ------x (right)
//     eye 
//

class CameraParameters
{
public:
	CameraParameters();

	enum CameraLensModel
	{
		CLM_PINHOLE,
		CLM_THIN_LENS,
		CLM_ORTHOGRAPHIC
	};
	
    // get eye position
	const Imath::V3f& eye() const;

    // get target position
	const Imath::V3f& target() const;

    // get and set eye-target distance
    float distanceToTarget() const;

	// retrieve camera orthonormal basis
	void getBasis(Imath::V3f& forwardUnitVector,
				  Imath::V3f& rightUnitVector,
				  Imath::V3f& upUnitVector) const;

	Imath::V3f forwardUnitVector() const;
	Imath::V3f rightUnitVector() const;
	Imath::V3f upUnitVector() const;
	
	// polar angle: angle with respect to the vertical Y axis. 0 <= theta <= PI
	float rotationTheta() const;
	// azimutal angle: angle with respect to the Z axis. 0 <= phi < 2*PI
	float rotationPhi() const;
	// TODO roll

	// get vertical fov, in radians
	float fovY() const; 

	// get focal length, which is proportional to the current FOV and the sensor
	// size.
	float focalLength() const;

	float nearDistance() const;

	float farDistance() const;

	// get distance to the focal plane
	float focalDistance() const;

	Imath::V2f filmSize() const;

	float lensRadius() const;

	CameraLensModel lensModel() const;

    static float FILM_SIZE_35MM;

private:
	friend class CameraController;
	friend class Camera;

	// FIXME
	friend class OrbitCameraController;
	friend class FlyCameraController;

	// move target, leaving the eye fixed.
    void lookAt(const Imath::V3f& target);
	// move eye away from target, which remains fixed.
    void setDistanceFromTarget(float distance);

	
	// Set eye position from spherical coordinates centered on the target point.
	// Parameters:
	// - theta: polar angle. Angle with respect to the vertical axis 0 <= theta <= PI. 
	// At theta = 0 degrees, the camera looks down directly along the Y axis.
	// - phi: angle with respect to the Z axis. 0 <= phi < 2*PI.
	// At phi = 0 degrees, the projection of the (eye-target) vector falls onto
	// the Z axis.
	void orbitAroundTarget(float theta, float phi);
	// Set target position from spherical coordinates centered on the eye point.
	void orbitAroundEye(float theta, float phi);

	// Set the camera direction explicitly from eye and target points.
	void setEyeTarget(const Imath::V3f& eye, const Imath::V3f& target);

	// set vertical FOV in radians. This affects focal distance.
	void setFovY(float fov);
	void setNearDistance(float d);
	void setFarDistance(float d);
	// set focal length. This affects the FOV.
	void setFocalLength(float length);
	// set distance to focal plane
	void setFocalDistance(float distance);
	void setFilmSize(float filmW, float filmH);
	void setLensRadius(float radius);
	void setFStop(float fstop);
	void setLensModel(CameraLensModel model);

private:
	Imath::V3f	    m_target;
	Imath::V3f	    m_eye;
    //float		    m_phi;
	//float		    m_theta;
	float		    m_fovY;
	float		    m_near;
	float		    m_far;
	float		    m_focalDistance;
	float		    m_lensRadius;
	Imath::V2f		m_filmSize;
	CameraLensModel m_lensModel;
};
