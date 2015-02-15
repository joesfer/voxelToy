#version 430

#include <editVoxels/selectVoxelDevice.h>

uniform sampler3D   occupancyTexture;
uniform ivec3       voxelResolution;
uniform vec3        volumeBoundsMin;
uniform vec3        volumeBoundsMax;

uniform vec4        viewport;
uniform float       cameraNear;
uniform float       cameraFar;
uniform mat4        cameraProj;
uniform mat4        cameraInverseProj;
uniform mat4        cameraInverseModelView;
uniform float       cameraFocalLength;
uniform vec2        cameraFilmSize;

uniform vec2        sampledFragment;

#include <shared/constants.h>
#include <shared/aabb.h>
#include <shared/coordinates.h>
#include <shared/dda.h>
#include <shared/sampling.h>
#include <shared/generateRay.h>

// This vertex shader should run for a single vertex, and calculates the
// voxel index of the closest intersection.

void main()
{
	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay_Pinhole(vec3(sampledFragment, 0), wsRayOrigin, wsRayDir);
	
	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(wsRayOrigin, wsRayDir,
											  volumeBoundsMin, volumeBoundsMax); 
	SelectVoxelData.index = ivec4(0);

	if (aabbIsectDist < 0)
	{
		return;
	}

	float rayLength = aabbIsectDist;
	vec3 rayPoint = wsRayOrigin + rayLength * wsRayDir;
	vec3 vsHitPos;

	bool hitGround;
	if ( !traverse(rayPoint, wsRayDir, vsHitPos, hitGround) )
	{
		return;
	}

	Basis wsHitBasis;
	voxelSpaceToWorldSpace(vsHitPos, 
						   wsRayOrigin, wsRayDir,
						   wsHitBasis);

	SelectVoxelData.index = ivec4(vsHitPos.xyz,0);
	SelectVoxelData.normal = vec4(wsHitBasis.normal, 0);
}
