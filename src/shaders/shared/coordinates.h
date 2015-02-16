vec4 screenToEyeSpace(vec3 windowSpace)
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 ndcPos;
	ndcPos.xy = ((2.0 * windowSpace.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * windowSpace.z - cameraNear - cameraFar) / (cameraFar - cameraNear);
 
	vec4 clipPos;
	clipPos.w = cameraProj[3][2] / (ndcPos.z - (cameraProj[2][2] / cameraProj[2][3]));
	clipPos.xyz = ndcPos * clipPos.w;
 
	vec4 eyePos = cameraInverseProj * clipPos;
	return eyePos;
}

vec3 screenToWorldSpace(vec3 windowSpace)
{
	vec4 eyePos = screenToEyeSpace(windowSpace);
	return (cameraInverseModelView * eyePos).xyz;
}

struct Basis
{
	vec3 position;
	vec3 tangent;
	vec3 normal;
	vec3 binormal;
};

vec3 localToWorld(in vec3 lsV, 
				  in Basis basis)
{
	return lsV.x * basis.tangent + 
		   lsV.y * basis.normal + 
		   lsV.z * basis.binormal;
}

vec3 worldToLocal(in vec3 wsV,
				  in Basis basis)
{
	return vec3(dot(wsV, basis.tangent),
				dot(wsV, basis.normal),
				dot(wsV, basis.binormal));
}

void voxelSpaceToWorldSpace(in vec3 vsP, 
							in vec3 wsRayOrigin, in vec3 wsRayDir,
							out Basis wsHitBasis)
{
	// vsP marks the lower-left corner of the voxel. Calculate the
	// precise ray/voxel intersection in world-space
	vec3 wsVoxelSize = (volumeBoundsMax - volumeBoundsMin) / voxelResolution;
	vec3 wsVoxelMin = vsP * wsVoxelSize + volumeBoundsMin; 
	vec3 wsVoxelMax = wsVoxelMin + wsVoxelSize; 
	float voxelHitDistance = rayAABBIntersection(wsRayOrigin, wsRayDir, wsVoxelMin, wsVoxelMax);

	wsHitBasis.position = wsRayOrigin + wsRayDir * voxelHitDistance; 
	
	vec3 wsVoxelCenter = wsVoxelMin + wsVoxelSize * 0.5;
	vec3 centerToHit = wsHitBasis.position - wsVoxelCenter; 
	vec3 absCenterToHit = abs(centerToHit); 
	vec3 mask = step(absCenterToHit.yxx, absCenterToHit.xyz) * step(absCenterToHit.zzy, absCenterToHit.xyz);
	wsHitBasis.normal = mask * sign(centerToHit);
	
	// since we're dealing with axis-aligned voxels, a diagonal unit vector is a
	// safe choice for the cross product. 1/sqrt(3) = 0.57735026919 
	vec3 tangent = cross(wsHitBasis.normal, vec3(0.57735026919));
	wsHitBasis.binormal = normalize(cross(tangent, wsHitBasis.normal));
	wsHitBasis.tangent = normalize(cross(wsHitBasis.binormal, wsHitBasis.normal));
}

// pole at +Y (theta = 0)
vec3 sphericalToCartesian(float phi, float theta)
{
	float sinTheta = sin(theta);
	return vec3( sinTheta * cos(phi), cos(theta), sinTheta * sin(phi) );
}

vec2 uvCoordFromVector(in vec3 vec, float rotation)
{
	// cartesian to spherical coordinates
	// pole at Y, v is assumed to be normalized
	const float theta = acos(vec.y);
	float phi = atan(vec.z, vec.x) + PI + rotation;
	//phi = modf(phi + rotation, PI);
	// map spherical coordinates to unit square (uv)
	const float u = mod(phi / TWO_PI, 1.0);
	const float v = theta / PI;

	return vec2(u, v);
}

vec3 directionFromUVCoord(in vec2 uv, float rotation)
{
	const float phi = uv.x * TWO_PI - PI - rotation;
	const float theta = uv.y * PI;
	return sphericalToCartesian(phi, theta);
}

