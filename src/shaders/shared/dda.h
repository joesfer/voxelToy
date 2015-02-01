// Variable naming conventions:
// ws = world space
// vs = voxel space (texture)

bool raymarch(in vec3 wsRayOrigin, 
			  in vec3 wsRayDir,
			  in int maxSteps,
			  out vec3 vsHitPos)
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
							   ivec3(voxelPos), 0).r > 0);
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

	vsHitPos = voxelPos;

	return isect;
}

bool traverse(in vec3 wsRayOrigin, 
			  in vec3 wsRayDir, 
			  in int maxSteps,
			  out vec3 vsHitPos, 
			  out bool hitGround)
{
	if (raymarch(wsRayOrigin, wsRayDir, maxSteps, vsHitPos))
	{
		hitGround = false;
		return true;
	}
	
	// we didn't hit the voxel model, but check whether we hit the optional
	// ground?
	hitGround = (vsHitPos.y < 0);
	return hitGround;
}

bool traverse(in vec3 wsRayOrigin, 
			  in vec3 wsRayDir,
			  out vec3 vsHitPos, 
			  out bool hitGround)
{
	// In theory I should not need to enforce any clamping because we test on
	// the raymarching function whether we've gone out of the volume bounds and
	// bail out there. But I can see very noticeable slowing downs the larger I
	// make this threshold (fragment divergence? the compiler no longer
	// unrolling the while loop?, or simply a bug somewhere). So better to make
	// the maxsteps limit as small as we can. 
	//
	// I haven't found any formal proof for this, but the heuristic of
	// multiplying by a factor of 2 the number of voxels in a straight line
	// appears to be correct from my tests (worst case is a diagonal line, where 
	// DDA would take one extra step for every advanced voxel as it jumps through 
	// adjacent faces and not vertices).  
	int MAX_STEPS = int(2 * ceil(length(vec3(voxelResolution))));
	return traverse(wsRayOrigin, wsRayDir, MAX_STEPS, vsHitPos, hitGround);
}


