#version 130

uniform sampler3D occupancyTexture;
uniform sampler3D voxelColorTexture;
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

	return tminf; 
}

vec4 shade(vec3 n)
{
	// Basic dot lighting
	vec4 ambient = vec4(0.1);
	return vec4(max(0, dot(n, -wsLightDir))) + ambient;
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

void main()
{
	vec3 rayOrigin = (cameraInverseModelView * vec4(0,0,1,1)).xyz;
	vec3 rayDir = normalize(screenToWorldSpace(vec3(gl_FragCoord.xy, cameraNear)) - rayOrigin);
	rayOrigin.z *=-1;
	rayDir.z *=-1;

	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(rayOrigin, rayDir); 
	if (aabbIsectDist < 0)
	{
		gl_FragColor = vec4(0);
		return;
	}
	
	// We have a potential intersection. Traverse the grid using DDA, the code
	// is inspired in iq's Voxel Edges demo in Shadertoy at https://www.shadertoy.com/view/4dfGzs
	vec3 res = vec3(voxelResolution);
	int MAX_STEPS = int(ceil(sqrt(res.x*res.x + res.y*res.y + res.z*res.z)));

	float rayLength = aabbIsectDist;
	bool isect = false;
	vec3 rayPoint = rayOrigin + rayLength * rayDir;
	vec3 voxelExtent = vec3(1.0) / (volumeBoundsMax - volumeBoundsMin);
	vec3 voxelOrigin = (rayPoint - volumeBoundsMin) * voxelExtent * voxelResolution;

	vec3 voxelPos = floor(voxelOrigin);
	vec3 rayDirIncrement = vec3(1.0f) / rayDir;
	vec3 rayDirSign = sign(rayDir);
	vec3 dis = (voxelPos-voxelOrigin + 0.5 + rayDirSign*0.5) * rayDirIncrement;

	vec3 mask=vec3(0.0);

	float voxelOccupancy = 0.0;
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
		dis += mask * rayDirSign * rayDirIncrement;
		voxelPos += mask * rayDirSign;

		// break the traversal if we've gone out of bounds 
		if (any(lessThan(voxelPos, vec3(0.0))) || 
			any(greaterThanEqual(voxelPos,voxelResolution))) break;

		steps++;

	}

	if ( isect )
	{
		vec3 hitNormal = -mask*rayDirSign;
		vec4 voxelColor = texelFetch(voxelColorTexture,
								     ivec3(voxelPos.x, voxelPos.y, voxelPos.z), 0);

		float ao = ambientOcclusion(voxelPos, hitNormal);

		gl_FragColor = ao * voxelColor * shade(hitNormal);
	}
	else
	{
		gl_FragColor = vec4(0.0);
	}

}


