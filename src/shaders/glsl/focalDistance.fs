#version 130

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

out float wsDistance;

#include <aabb.h>
#include <coordinates.h>
#include <dda.h>
#include <sampling.h>
#include <generateRay.h>

// This fragment shader should run for a single fragment, and calculates the
// distance to the closest intersection, in world-space. The distance is then
// fed to another shader down the line to be used as the camera focal distance.

void main()
{
	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay_Pinhole(vec3(sampledFragment, gl_FragCoord.z), wsRayOrigin, wsRayDir);
	
	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(wsRayOrigin, wsRayDir,
											  volumeBoundsMin, volumeBoundsMax); 

	if (aabbIsectDist < 0)
	{
		wsDistance = 99999999.0;	
		return;
	}

	float rayLength = aabbIsectDist;
	vec3 rayPoint = wsRayOrigin + rayLength * wsRayDir;
	vec3 vsHitPos, vsHitNormal;

	if ( !traverse(rayPoint, wsRayDir, vsHitPos, vsHitNormal) )
	{
		wsDistance = 99999999.0;	
		return;
	}

	// vsHitPos marks the lower-left corner of the voxel. Calculate the
	// precise ray/voxel intersection in world-space
	vec3 wsVoxelSize = (volumeBoundsMax - volumeBoundsMin) / voxelResolution;
	vec3 wsVoxelMin = vsHitPos * wsVoxelSize + volumeBoundsMin; 
	vec3 wsVoxelMax = wsVoxelMin + wsVoxelSize; 
	float voxelHitDistance = rayAABBIntersection(wsRayOrigin, wsRayDir, wsVoxelMin, wsVoxelMax);
	wsDistance = voxelHitDistance;
}
