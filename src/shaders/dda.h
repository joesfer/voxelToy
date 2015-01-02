// Variable naming conventions:
// ws = world space
// vs = voxel space (texture)

bool raymarch(in vec3 wsRayOrigin, in vec3 wsRayDir,
			  in int maxSteps,
			  out vec3 vsHitPos, out vec3 vsHitNormal)
{
	// Traverse the grid using DDA, the code is inspired in iq's Voxel Edges 
	// demo in Shadertoy at https://www.shadertoy.com/view/4dfGzs

	bool isect = false;
	vec3 voxelExtent = vec3(1.0) / (volumeBoundsMax - volumeBoundsMin);
	vec3 voxelOrigin = (wsRayOrigin - volumeBoundsMin) * voxelExtent * voxelResolution;

	vec3 voxelPos = floor(voxelOrigin);
	vec3 wsRayDirIncrement = vec3(1.0f) / wsRayDir;
	vec3 wsRayDirSign = sign(wsRayDir);
	vec3 dis = (voxelPos-voxelOrigin + 0.5 + wsRayDirSign*0.5) * wsRayDirIncrement;

	vec3 mask=vec3(0.0);

	int steps = 0;
	while(steps < maxSteps) 
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

		// break from the traversal if we've gone out of bounds 
		if (any(lessThan(voxelPos, vec3(0.0))) || 
			any(greaterThanEqual(voxelPos,voxelResolution))) break;

		steps++;

	}

	vsHitNormal = -mask*wsRayDirSign;
	vsHitPos = voxelPos;

	return isect;
}

bool traverse(in vec3 wsRayOrigin, in vec3 wsRayDir, in int maxSteps) 
{
	vec3 vsHitPos;
	vec3 vsHitNormal;
	return raymarch(wsRayOrigin, wsRayDir, maxSteps, vsHitPos, vsHitNormal);
}

bool traverse(in vec3 wsRayOrigin, in vec3 wsRayDir,
			  out vec3 vsHitPos, out vec3 vsHitNormal)
{
	vec3 res = vec3(voxelResolution);
	int MAX_STEPS = int(ceil(length(res)));

	return raymarch(wsRayOrigin, wsRayDir, MAX_STEPS, vsHitPos, vsHitNormal);
}


