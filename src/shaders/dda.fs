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

// 0 or 1, toggles ambient occlusion
uniform int         ambientOcclusionEnable;
// from 0.0 to 1.0, how long does the shadow ray traverse for the AO
// calculations, as a fraction of the number of voxels
uniform float		ambientOcclusionReach;
// from 0.0 to 1.0, cone spread used to generate shadow rays. This value is
// multiplied by PI when mapped to the polar theta angle.
uniform float		ambientOcclusionSpread;

out vec4 outColor;

#include <coordinates.h>
#include <dda.h>
#include <sampling.h>
#include <ao.h>
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

	vec4 voxelColor = texelFetch(voxelColorTexture,
							     ivec3(vsHitPos.x, vsHitPos.y, vsHitPos.z), 0);

	// vsHitPos marks the lower-left corner of the voxel. Calculate the
	// precise ray/voxel intersection in world-space
	vec3 wsVoxelSize = (volumeBoundsMax - volumeBoundsMin) / voxelResolution;
	vec3 wsVoxelMin = vsHitPos * wsVoxelSize + volumeBoundsMin; 
	vec3 wsVoxelMax = wsVoxelMin + wsVoxelSize; 
	float voxelHitDistance = rayAABBIntersection(wsRayOrigin, wsRayDir, wsVoxelMin, wsVoxelMax);
	vec3 wsHitPos = wsRayOrigin + wsRayDir * voxelHitDistance; 

	vec3 ambient = vec3(0.1);
	vec3 lighting = ambient;
	
	// note since we don't yet allow for transformation on the voxels, the 
	// voxel-space and world-space normals coincide.
	vec3 wsHitNormal = vsHitNormal; 

	// TODO it will probably be better to turn all these (and future) toggles
	// into #defines and generate shader variations given the desired set of
	// rendering traits. There's currently no divergence on the if statements,
	// but still.
	float ao = ambientOcclusionEnable != 0 ? ambientOcclusion(wsHitPos, vsHitPos, wsHitNormal) : 1.0;
	
	lighting = vec3(ao);

	outColor = voxelColor * vec4(lighting,1);
}


