#version 130

uniform sampler3D   occupancyTexture;
uniform sampler3D   voxelColorTexture;
uniform sampler2D   noiseTexture;
uniform sampler2D   focalDistanceTexture;
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

uniform float       cameraLensRadius;
uniform vec2        cameraFilmSize;
uniform vec3        wsLightDir;
uniform vec4        backgroundColor = vec4(0.2,   0.2,   0.2,   1);
uniform int         sampleCount;
uniform int         enableDOF;

out vec4 outColor;

#include <coordinates.h>
#include <dda.h>
#include <ao.h>
#include <sampling.h>
#include <generateRay.h>
#include <aabb.h>

vec4 shade(vec3 n)
{
	// Basic dot lighting
	return vec4(max(0, dot(n, -wsLightDir)));
}

void generateRay(out vec3 wsRayOrigin, out vec3 wsRayDir)
{
	if (enableDOF != 0)
	{
		generateRay_ThinLens(gl_FragCoord.xyz, wsRayOrigin, wsRayDir);
	}
	else
	{
		generateRay_Pinhole(gl_FragCoord.xyz, wsRayOrigin, wsRayDir);
	}

}

void main()
{
	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay(wsRayOrigin, wsRayDir);

	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(wsRayOrigin, wsRayDir,
											  volumeBoundsMin, volumeBoundsMax); 

	if (aabbIsectDist < 0)
	{
		outColor = backgroundColor;
		return;
	}

	float rayLength = aabbIsectDist;
	vec3 rayPoint = wsRayOrigin + rayLength * wsRayDir;
	vec3 vsHitPos, vsHitNormal;
	if ( !traverse(rayPoint, wsRayDir, vsHitPos, vsHitNormal) )
	{
		outColor = backgroundColor;
		return;
	}

	//vec4 voxelColor = texelFetch(voxelColorTexture,
	//						     ivec3(vsHitPos.x, vsHitPos.y, vsHitPos.z), 0);
	vec4 voxelColor = vec4(1); 

	float ao = ambientOcclusion(vsHitPos, vsHitNormal); 

	// vsHitPos marks the lower-left corner of the voxel. Calculate the
	// precise ray/voxel intersection in world-space
	vec3 wsVoxelSize = (volumeBoundsMax - volumeBoundsMin) / voxelResolution;
	vec3 wsVoxelMin = vsHitPos * wsVoxelSize + volumeBoundsMin; 
	vec3 wsVoxelMax = wsVoxelMin + wsVoxelSize; 
	float voxelHitDistance = rayAABBIntersection(wsRayOrigin, wsRayDir, wsVoxelMin, wsVoxelMax);
	vec3 wsHitPos = wsRayOrigin + wsRayDir * voxelHitDistance; 

	vec3 ambient = vec3(0.2);
	vec3 lighting = ambient;
	float EPSILON = 0.1; // avoid self-intersection
	vec3 primaryRayHitNormal = vsHitNormal;

vec3 tangent = cross(primaryRayHitNormal, wsRayDir);
vec3 bitangent = cross(tangent, primaryRayHitNormal);

	vec4 uniformRandomSample = texelFetch(noiseTexture, ivec2(sampleCount, 0), 0);
	float phi = uniformRandomSample.x * 2.0 * PI;
	float theta = uniformRandomSample.y * PI * 0.4; // just one hemisphere
	vec3 toLight = polarToVector(phi, theta); 
	toLight = toLight.x * tangent +
			  toLight.z * bitangent +
			  toLight.y * primaryRayHitNormal;

	vec3 res = vec3(voxelResolution);
	int maxReach = int(ceil(length(res) * 0.1)); // in voxels
	if ( !traverse(wsHitPos + EPSILON * primaryRayHitNormal, 
				   toLight, 
				   maxReach) )
	{
		// we didn't hit anything between us and the light - thus this voxel receives 
		// contribution from this light
		lighting += vec3(1);//vec3(shade(primaryRayHitNormal));
	}

	//lighting *= ao;

	outColor = voxelColor * vec4(lighting,1);
}


