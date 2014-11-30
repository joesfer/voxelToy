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


class Camera
{
public:
	Camera();
	
    // get eye position
	Imath::V3f eye() const;

    // get target position
	Imath::V3f target() const;

    // set target at supplied location, and move eye position proportionally to
    // the current orientation and distance to camera.
    void centerAt(const Imath::V3f& target);

    // get and set eye-target distance
    float distanceToTarget() const;
    void setDistanceToTarget(float distance);

	// retrieve camera orthonormal basis
	Imath::V3f forwardUnitVector() const;
	Imath::V3f rightUnitVector() const;
	Imath::V3f upUnitVector() const;
	
	// polar angle: angle with respect to the vertical Y axis. 0 <= theta <= PI
	float rotationTheta() const;
	// azimutal angle: angle with respect to the Z axis. 0 <= phi < 2*PI
	float rotationPhi() const;
	// TODO roll

	// Set eye position from spherical coordinates centered around the target
	// position. Parameters:
	// - theta: polar angle. Angle with respect to the vertical axis 0 <= theta <= PI. 
	// At theta = 0 degrees, the camera looks down directly along the Y axis.
	// - phi: angle with respect to the Z axis. 0 <= phi < 2*PI.
	// At phi = 0 degrees, the projection of the (eye-target) vector falls onto
	// the Z axis.
	void setOrbitRotation(float theta, float phi);

	// Set the camera direction explicitly from eye and target points.
	void setEyeTarget(const Imath::V3f& eye, const Imath::V3f& target);

	float fovY() const;
	void setFovY(float fov);

private:
	Imath::V3f	 m_target;
    float        m_targetDistance;
    float		 m_phi;
	float		 m_theta;
	float		 m_fovY;
};
