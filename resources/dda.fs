#version 130

uniform sampler3D   occupancyTexture;
uniform sampler3D   voxelColorTexture;
uniform sampler2D   noiseTexture;
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
uniform float       cameraFocalDistance;
uniform float       cameraLensRadius;
uniform float		cameraFilmSize;
uniform vec3        wsLightDir;
uniform vec4        backgroundColor = vec4(0.2,   0.2,   0.2,   1);
uniform int         sampleCount;
uniform int			enableDOF;

#include <coordinates.h>
#include <dda.h>
#include <ao.h>
#include <sampling.h>
#include <aabb.h>

vec4 shade(vec3 n)
{
	// Basic dot lighting
	return vec4(max(0, dot(n, -wsLightDir)));
}

void generateRay(out vec3 wsRayOrigin, out vec3 wsRayDir)
{
	vec3 esLensSamplePoint;
	if (enableDOF != 0)
	{
		// thin lens model
		vec4 uniformRandomSample = texelFetch(noiseTexture, ivec2(sampleCount, 0), 0);
		vec2 unitDiskSample = sampleDisk(uniformRandomSample.xy);
		esLensSamplePoint = vec3(unitDiskSample * cameraLensRadius, 0);
	}
	else
	{
		// pinhole
		esLensSamplePoint = vec3(0,0,0);
	}

	vec3 esImagePlanePoint = (vec3((gl_FragCoord.xy/viewport.zw * vec2(2) - vec2(1)) * cameraFilmSize, 0)).xyz;
	esImagePlanePoint.z = -cameraFocalLength;

	vec3 esFilmToPinhole = normalize(-esImagePlanePoint);
	float t = (-cameraFocalDistance - esImagePlanePoint.z) / esFilmToPinhole.z;
	// intersection of ray originating at film plane, passing through an ideal 
	// pinhole (i.e. completely sharp)
	vec3 esFocalPlanePoint = esImagePlanePoint + t * esFilmToPinhole;
	vec3 wsFocalPlanePoint = (cameraInverseModelView * vec4(esFocalPlanePoint, 1)).xyz;

	wsRayOrigin = (cameraInverseModelView * vec4(esLensSamplePoint,1)).xyz;
	wsRayDir =  normalize(wsFocalPlanePoint - wsRayOrigin);
}

void main()
{
	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay(wsRayOrigin, wsRayDir);

	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(wsRayOrigin, wsRayDir); 

	if (aabbIsectDist < 0)
	{
		gl_FragColor = backgroundColor;
		return;
	}

	vec3 voxelExtent = vec3(1.0) / (volumeBoundsMax - volumeBoundsMin);

	float rayLength = aabbIsectDist;
	vec3 rayPoint = wsRayOrigin + rayLength * wsRayDir;
	vec3 vsHitPos, vsHitNormal;
	if ( !raymarch(rayPoint, wsRayDir, vsHitPos, vsHitNormal) )
	{
		gl_FragColor = backgroundColor;
		return;
	}

	vec4 voxelColor = texelFetch(voxelColorTexture,
							     ivec3(vsHitPos.x, vsHitPos.y, vsHitPos.z), 0);

	float ao = ambientOcclusion(vsHitPos, vsHitNormal); 

	float lighting = shade(vsHitNormal);
	vsHitPos += vsHitNormal;
	vec3 wsHitPos = (vsHitPos + vec3(0.5))/(voxelResolution*voxelExtent) + volumeBoundsMin; 
	vec3 toLight = normalize(wsLightDir);
	if ( raymarch(wsHitPos, -wsLightDir, vsHitPos, vsHitNormal) )
	{
		// we hit something between us and the light - thus this voxel is in
		// shadow.
		lighting = vec3(0);
	}

	vec3 ambient = vec3(0.2);
	lighting += ambient;

	lighting *= ao;

	gl_FragColor = voxelColor * lighting;

}


