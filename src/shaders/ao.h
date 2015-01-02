vec3 offset[8] = vec3[8](vec3(-1,-1,0),
						 vec3( 0,-1,0),
				  		 vec3(+1,-1,0),
				  		 vec3(-1, 0,0),
				  		 //vec3( 0, 0,0),
				  		 vec3(+1, 0,0),
				  		 vec3(-1,+1,0),
				  		 vec3( 0,+1,0),
				  		 vec3(+1,+1,0));

////////////////////////////////////////////////////////////////////////////////
// approximate ambient occlusion by checking voxel adjacency on the top 2 out 
// of 3 possible slabs of 9x9 voxels around the central voxel p. This is a very
// rough approximation of a hemisphere which requires 17 instead of 26 samples
// for the full cube.
////////////////////////////////////////////////////////////////////////////////

float ambientOcclusionAdjacency(vec3 p, vec3 n)
{
	/*
	           ____
	          /   /|
	         /___/ | p + n
	         |   | /
	         |___|/  
	           n   / offset
	        ___|__/______ 
	       /   |   /   /|
	      /___/|__/___/ |  second slab
	     /   /|| /   /| /
	    /___/_|_/___/ |/
	   /   /   /   /| /____ offset
	  /___/___/___/ |/
	  |   |   |   | /
	  |___|___|___|/ 
	        ___|________
	       /   |   /   /|
	      /___/|__/___/ |  first slab
	     /   /|P /   /| /
	    /___/_|_/___/ |/
	   /   /   /   /| / 
	  /___/___/___/ |/
	  |   |   |   | /
	  |___|___|___|/ 
	  
	 
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
	hit += texelFetch(occupancyTexture, ivec3(p+n), 0).r > 0 ? 1.0 : 0.0;

	return 1.0f - hit / 17;

}

////////////////////////////////////////////////////////////////////////////////
// raymarched AO
////////////////////////////////////////////////////////////////////////////////

float ambientOcclusion(in vec3 wsP, 
					   in vec3 vsP,
					   in vec3 wsN)
{
	// since we're dealing with axis-aligned voxels, a diagonal unit vector is a
	// safe choice for the cross product. 1/sqrt(3) = 0.57735026919 
	vec3 tangent = normalize(cross(wsN, vec3(0.57735026919)));
	vec3 bitangent = normalize(cross(tangent, wsN));
	tangent = cross(bitangent, wsN);

	// TODO use different set of random numbers - currently we introduce
	// correlation between DOF samples and AO samples.
	vec4 uniformRandomSample = texelFetch(noiseTexture, ivec2(sampleCount, 0), 0);
	float phi = uniformRandomSample.x * 2.0 * PI;
	float theta = uniformRandomSample.y * PI * ambientOcclusionSpread; // just one hemisphere
	
	// random direction in the local hemisphere (with pole pointing along +Y axis)
	vec3 lsShadowRay = polarToVector(phi, theta); 
	// orient hemisphere with normal
	vec3 wsShadowRay = lsShadowRay.x * tangent +
					   lsShadowRay.y * wsN +
					   lsShadowRay.z * bitangent;

	// traverse a few voxels to see if we hit something
	vec3 res = vec3(voxelResolution);
	int traversalMaxVoxels = int(ceil(length(res) * ambientOcclusionReach)); // in voxels
	vec3 vsAOHitPos;
	vec3 vsAOHitNormal;
	float ISECT_EPSILON = 0.01; // avoid self-intersection
	bool shadowRayHit = traverse(wsP + ISECT_EPSILON * wsN, 
								 wsShadowRay, 
								 traversalMaxVoxels,
								 vsAOHitPos,
								 vsAOHitNormal);
	if (!shadowRayHit) return 1.0;
	
	// modulate AO by the intersection distance
	float vsAOHitDist = length(vsP - vsAOHitPos); // voxel space
	float occlusion = vsAOHitPos / traversalMaxVoxels;
	occlusion *= occlusion; // seems to look better squared this way

	return occlusion;
}

