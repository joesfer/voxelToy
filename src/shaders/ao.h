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


