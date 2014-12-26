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
uniform vec3        wsLightDir;
uniform vec4        backgroundColor = vec4(0.2,   0.2,   0.2,   1);
uniform int         sampleCount;

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

{
	{
	}
	else
	{
	}

// Variable naming conventions:
// ws = world space
// vs = voxel space (texture)

bool raymarch(vec3 wsRayOrigin, vec3 wsRayDir,
			  out vec3 vsHitPos, out vec3 vsHitNormal)
{
	// We have a potential intersection. Traverse the grid using DDA, the code
	// is inspired in iq's Voxel Edges demo in Shadertoy at https://www.shadertoy.com/view/4dfGzs
	vec3 res = vec3(voxelResolution);
	int MAX_STEPS = 2 * int(ceil(sqrt(res.x*res.x + res.y*res.y + res.z*res.z)));

	bool isect = false;
	vec3 voxelExtent = vec3(1.0) / (volumeBoundsMax - volumeBoundsMin);
	vec3 voxelOrigin = (wsRayOrigin - volumeBoundsMin) * voxelExtent * voxelResolution;

	vec3 voxelPos = floor(voxelOrigin);
	vec3 wsRayDirIncrement = vec3(1.0f) / wsRayDir;
	vec3 wsRayDirSign = sign(wsRayDir);
	vec3 dis = (voxelPos-voxelOrigin + 0.5 + wsRayDirSign*0.5) * wsRayDirIncrement;

	vec3 mask=vec3(0.0);

	int steps = 0;
	while(steps < MAX_STEPS) 
	{
		bool hit = (texelFetch(occupancyTexture, 
							   ivec3(voxelPos.x, voxelPos.y, voxelPos.z), 0).r > 0);
		if (hit)
		{
			isect = true;
			break;
		}
		mask = step(dis.xyz, dis.yxy) * step(dis.xyz, dis.zzx);
		dis += mask * wsRayDirSign * wsRayDirIncrement;
		voxelPos += mask * wsRayDirSign;

		// break the traversal if we've gone out of bounds 
		if (any(lessThan(voxelPos, vec3(0.0))) || 
			any(greaterThanEqual(voxelPos,voxelResolution))) break;

		steps++;

	}

	vsHitNormal = -mask*wsRayDirSign;
	vsHitPos = voxelPos;

	return isect;
}
			  

void main()
{
	vec3 wsRayOrigin = (cameraInverseModelView * vec4(0,0,0,1)).xyz;
	vec2 jittering = texelFetch(noiseTexture, ivec2(sampleCount, 0), 0).xy;
	vec3 wsRayDir = normalize(screenToWorldSpace(vec3(gl_FragCoord.xy + jittering, cameraNear)) - wsRayOrigin);
	wsRayOrigin.z *=-1;
	wsRayDir.z *=-1;

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


