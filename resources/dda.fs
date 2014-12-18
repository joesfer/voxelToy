#version 130

uniform sampler3D occupancyTexture;
uniform sampler3D voxelColorTexture;
uniform sampler2D noiseTexture;
uniform ivec3 voxelResolution;
uniform vec3 volumeBoundsMin;
uniform vec3 volumeBoundsMax;

uniform vec4 viewport;
uniform float cameraNear;
uniform float cameraFar;
uniform mat4 cameraProj;
uniform mat4 cameraInverseProj;
uniform mat4 cameraInverseModelView;
uniform vec3 wsLightDir;
uniform vec4 backgroundColor = vec4(0.2, 0.2, 0.2, 1);
uniform int sampleCount;

float rayAABBIntersection(vec3 o, vec3 d)
{
	vec3 invDir = vec3(1.0f) / d; 

	vec3 tMin3 = (volumeBoundsMin - o) * invDir;
	vec3 tMax3 = (volumeBoundsMax - o) * invDir;

	vec3 tmin = min(tMin3, tMax3);
	float tminf = max(tmin.x, max(tmin.y, tmin.z));
	vec3 tmax = max(tMin3, tMax3);
	float tmaxf = min(tmax.x, min(tmax.y, tmax.z));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
	if (tmaxf < 0)
	{
		return -1; 
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tminf > tmaxf)
	{
		return -1; 
	}

	return tminf >= 0 ? tminf : tmaxf;
}

vec4 shade(vec3 n)
{
	// Basic dot lighting
	return vec4(max(0, dot(n, -wsLightDir)));
}

vec3 offset[8] = vec3[8](vec3(-1,-1,0),
						 vec3( 0,-1,0),
				  		 vec3(+1,-1,0),
				  		 vec3(-1, 0,0),
				  		 //vec3( 0, 0,0),
				  		 vec3(+1, 0,0),
				  		 vec3(-1,+1,0),
				  		 vec3( 0,+1,0),
				  		 vec3(+1,+1,0));

// approximate ambient occlusion by checking voxel adjacency on the top 2 out 
// of 3 possible slabs of 9x9 voxels around the central voxel p. This is a very
// rough approximation of a hemisphere which requires 17 instead of 26 samples
// for the full cube.
float ambientOcclusion(vec3 p, vec3 n)
{
	/*
	 *          ____
	 *         /   /|
	 *        /___/ | p + n
	 *        |   | /
	 *        |___|/  
	 *          n   / offset
	 *       ___|__/______ 
	 *      /   |   /   /|
	 *     /___/|__/___/ |  second slab
	 *    /   /|| /   /| /
	 *   /___/_|_/___/ |/
	 *  /   /   /   /| /____ offset
	 * /___/___/___/ |/
	 * |   |   |   | /
	 * |___|___|___|/ 
	 *       ___|________
	 *      /   |   /   /|
	 *     /___/|__/___/ |  first slab
	 *    /   /|P /   /| /
	 *   /___/_|_/___/ |/
	 *  /   /   /   /| / 
	 * /___/___/___/ |/
	 * |   |   |   | /
	 * |___|___|___|/ 
	 * 
	 *
	 */

	// n is expected to be {x, 0, 0}, {0, y, 0} or {0, 0, z} for
	// x,y,z e [-1,1]
	bvec3 mask = greaterThan(abs(n), vec3(0));

	float hit = 0.0;
	if(mask.z)
	{
		// first slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+offset[i].xyz),
													   0).r > 0 ? 1.0 : 0.0;
		// second slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+n+offset[i].xyz),
													   0).r > 0 ? 1.0 : 0.0;
	}
	else if(mask.y)
	{
		// first slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+offset[i].xzy),
													   0).r > 0 ? 1.0 : 0.0;
		// second slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+n+offset[i].xzy),
													   0).r > 0 ? 1.0 : 0.0;
	}
	else
	{
		// first slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+offset[i].yzx),
													   0).r > 0 ? 1.0 : 0.0;
		// second slab
		for( int i = 0; i < 8; ++i ) hit += texelFetch(occupancyTexture, 
													   ivec3(p+n+offset[i].yzx),
													   0).r > 0 ? 1.0 : 0.0;
	}

	// fill central gap on second slab
	hit += texelFetch(occupancyTexture, 
				   ivec3(p+n),
				   0).r > 0 ? 1.0 : 0.0;

	return 1.0f - hit / 17;

}

vec3 screenToWorldSpace(vec3 windowSpace)
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 ndcPos;
	ndcPos.xy = ((2.0 * windowSpace.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * windowSpace.z - cameraNear - cameraFar) / (cameraFar - cameraNear);
 
	vec4 clipPos;
	clipPos.w = cameraProj[3][2] / (ndcPos.z - (cameraProj[2][2] / cameraProj[2][3]));
	clipPos.xyz = ndcPos * clipPos.w;
 
	vec4 eyePos = cameraInverseProj * clipPos;
	return (cameraInverseModelView * eyePos).xyz;
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


