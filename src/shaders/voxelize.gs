////////////////////////////////////////////////////////////////////////////////
// This code is based on 
// Hybrid Computational Voxelization Using the Graphics Pipeline
// By Randall Rauwendaal and Mike Bailey 
// http://jcgt.org/published/0002/01/02/
// 
// This corresponds to the geometry shader method (suggested for small
// triangles in the paper), which takes triangles as inputs and rasterizes them
// on a 3D texture using OpenGL image pipeline.
////////////////////////////////////////////////////////////////////////////////

#version 420

// Thin voxelization is when adjacent voxels are at least connected by vertices
#define THIN 0 
// Fat voxelization is when adjacent voxels need to share at least a face
#define FAT  1

#define THICKNESS THIN

// inputs from vertex shader
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// UNIFORM (from OpenGL)
uniform ivec3 voxelResolution;

//Voxel output
layout(r8ui, binding = 0) uniform uimage3D Voxels;

in block
{
	vec3 vsVertexPos;
} In[];


// Look-up table of permutations matrices used to reverse triangle swizzling and
// restore vertices to their original orientation.
const mat3 unswizzleLUT[] = { mat3(0,1,0,
								   0,0,1,
								   1,0,0), 
							  mat3(0,0,1,
								   1,0,0,
								   0,1,0), 
							  mat3(1,0,0,
								   0,1,0,
								   0,0,1) };

// swizzle triangle vertices -- determine the dominant axis-aligned plane for a
// given triangle (that where the triangle projection is largest) and rotate the
// triangle vertices to make that plane always be the XY plane. This method also
// returns the swizzling matrix so that we can undo this transformation later
// on.
void swizzleTri(inout vec3 v0, 
				inout vec3 v1, 
				inout vec3 v2, 
				out vec3 n, 
				out mat3 unswizzle)
{
	//       cross(e0, e1);
	n = cross(v1 - v0, v2 - v1);

	vec3 absN = abs(n);
	float maxAbsN = max(max(absN.x, absN.y), absN.z);

	if(absN.x >= absN.y && absN.x >= absN.z)			
	{													
		//X-direction dominant (YZ-plane)
		//Then you want to look down the X-direction

		v0.xyz = v0.yzx;
		v1.xyz = v1.yzx;
		v2.xyz = v2.yzx;
		
		n.xyz = n.yzx;

		//XYZ <-> YZX
		unswizzle = unswizzleLUT[0];
	}
	else if(absN.y >= absN.x && absN.y >= absN.z)		
	{													
		//Y-direction dominant (ZX-plane)
		//Then you want to look down the Y-direction

		v0.xyz = v0.zxy;
		v1.xyz = v1.zxy;
		v2.xyz = v2.zxy;

		n.xyz = n.zxy;

		//XYZ <-> ZXY
		unswizzle = unswizzleLUT[1];
	}
	else												
	{													
		//Z-direction dominant (XY-plane)
		//Then you want to look down the Z-direction (the default)

		//v0.xyz = v0.xyz;
		//v1.xyz = v1.xyz;
		//v2.xyz = v2.xyz;

		//n.xyz = n.xyz;

		//XYZ <-> XYZ
		unswizzle = unswizzleLUT[2];
	}
}

void writeVoxels(ivec3 coord, uint val, vec3 color)
{
	//modify as necessary for attributes/storage type
	imageStore(Voxels, coord, uvec4(val));
}

void voxelizeTriPostSwizzle(vec3 v0, vec3 v1, vec3 v2, vec3 n, mat3 unswizzle, ivec3 minVoxIndex, ivec3 maxVoxIndex)
{
	vec3 e0 = v1 - v0;	//figure 17/18 line 2
	vec3 e1 = v2 - v1;	//figure 17/18 line 2
	vec3 e2 = v0 - v2;	//figure 17/18 line 2

	//INward Facing edge normals XY
	vec2 n_e0_xy = (n.z >= 0) ? vec2(-e0.y, e0.x) : vec2(e0.y, -e0.x);	//figure 17/18 line 4
	vec2 n_e1_xy = (n.z >= 0) ? vec2(-e1.y, e1.x) : vec2(e1.y, -e1.x);	//figure 17/18 line 4
	vec2 n_e2_xy = (n.z >= 0) ? vec2(-e2.y, e2.x) : vec2(e2.y, -e2.x);	//figure 17/18 line 4

	//INward Facing edge normals YZ
	vec2 n_e0_yz = (n.x >= 0) ? vec2(-e0.z, e0.y) : vec2(e0.z, -e0.y);	//figure 17/18 line 5
	vec2 n_e1_yz = (n.x >= 0) ? vec2(-e1.z, e1.y) : vec2(e1.z, -e1.y);	//figure 17/18 line 5
	vec2 n_e2_yz = (n.x >= 0) ? vec2(-e2.z, e2.y) : vec2(e2.z, -e2.y);	//figure 17/18 line 5

	//INward Facing edge normals ZX
	vec2 n_e0_zx = (n.y >= 0) ? vec2(-e0.x, e0.z) : vec2(e0.x, -e0.z);	//figure 17/18 line 6
	vec2 n_e1_zx = (n.y >= 0) ? vec2(-e1.x, e1.z) : vec2(e1.x, -e1.z);	//figure 17/18 line 6
	vec2 n_e2_zx = (n.y >= 0) ? vec2(-e2.x, e2.z) : vec2(e2.x, -e2.z);	//figure 17/18 line 6

#if THICKNESS == THIN
	float d_e0_xy = dot(n_e0_xy, .5-v0.xy) + 0.5 * max(abs(n_e0_xy.x), abs(n_e0_xy.y));	//figure 18 line 7
	float d_e1_xy = dot(n_e1_xy, .5-v1.xy) + 0.5 * max(abs(n_e1_xy.x), abs(n_e1_xy.y));	//figure 18 line 7
	float d_e2_xy = dot(n_e2_xy, .5-v2.xy) + 0.5 * max(abs(n_e2_xy.x), abs(n_e2_xy.y));	//figure 18 line 7

	float d_e0_yz = dot(n_e0_yz, .5-v0.yz) + 0.5 * max(abs(n_e0_yz.x), abs(n_e0_yz.y));	//figure 18 line 8
	float d_e1_yz = dot(n_e1_yz, .5-v1.yz) + 0.5 * max(abs(n_e1_yz.x), abs(n_e1_yz.y));	//figure 18 line 8
	float d_e2_yz = dot(n_e2_yz, .5-v2.yz) + 0.5 * max(abs(n_e2_yz.x), abs(n_e2_yz.y));	//figure 18 line 8

	float d_e0_zx = dot(n_e0_zx, .5-v0.zx) + 0.5 * max(abs(n_e0_zx.x), abs(n_e0_zx.y));	//figure 18 line 9
	float d_e1_zx = dot(n_e1_zx, .5-v1.zx) + 0.5 * max(abs(n_e1_zx.x), abs(n_e1_zx.y));	//figure 18 line 9
	float d_e2_zx = dot(n_e2_zx, .5-v2.zx) + 0.5 * max(abs(n_e2_zx.x), abs(n_e2_zx.y));	//figure 18 line 9
#elif THICKNESS == FAT
	float d_e0_xy = -dot(n_e0_xy, v0.xy) + max(0.0f, n_e0_xy.x) + max(0.0f, n_e0_xy.y);	//figure 17 line 7
	float d_e1_xy = -dot(n_e1_xy, v1.xy) + max(0.0f, n_e1_xy.x) + max(0.0f, n_e1_xy.y);	//figure 17 line 7
	float d_e2_xy = -dot(n_e2_xy, v2.xy) + max(0.0f, n_e2_xy.x) + max(0.0f, n_e2_xy.y);	//figure 17 line 7

	float d_e0_yz = -dot(n_e0_yz, v0.yz) + max(0.0f, n_e0_yz.x) + max(0.0f, n_e0_yz.y);	//figure 17 line 8
	float d_e1_yz = -dot(n_e1_yz, v1.yz) + max(0.0f, n_e1_yz.x) + max(0.0f, n_e1_yz.y);	//figure 17 line 8
	float d_e2_yz = -dot(n_e2_yz, v2.yz) + max(0.0f, n_e2_yz.x) + max(0.0f, n_e2_yz.y);	//figure 17 line 8

	float d_e0_zx = -dot(n_e0_zx, v0.zx) + max(0.0f, n_e0_zx.x) + max(0.0f, n_e0_zx.y);	//figure 18 line 9
	float d_e1_zx = -dot(n_e1_zx, v1.zx) + max(0.0f, n_e1_zx.x) + max(0.0f, n_e1_zx.y);	//figure 18 line 9
	float d_e2_zx = -dot(n_e2_zx, v2.zx) + max(0.0f, n_e2_zx.x) + max(0.0f, n_e2_zx.y);	//figure 18 line 9
#endif

	vec3 nProj = (n.z < 0.0) ? -n : n;	//figure 17/18 line 10

	const float dTri = dot(nProj, v0);
#if THICKNESS == THIN
	const float dTriThin   = dTri - dot(nProj.xy, vec2(0.5));	//figure 18 line 11
#elif THICKNESS == FAT
	const float dTriFatMin = dTri - max(nProj.x, 0) - max(nProj.y, 0);	//figure 17 line 11
	const float dTriFatMax = dTri - min(nProj.x, 0) - min(nProj.y, 0);	//figure 17 line 12
#endif

	const float nzInv = 1.0 / nProj.z;
	
	ivec3 p;					//voxel coordinate
	int   zMin,      zMax;		//voxel Z-range
	float zMinInt,   zMaxInt;	//voxel Z-intersection min/max
	float zMinFloor, zMaxCeil;	//voxel Z-intersection floor/ceil
	for(p.x = minVoxIndex.x; p.x < maxVoxIndex.x; p.x++)	//figure 17 line 13, figure 18 line 12
	{
		for(p.y = minVoxIndex.y; p.y < maxVoxIndex.y; p.y++)	//figure 17 line 14, figure 18 line 13
		{
			float dd_e0_xy = d_e0_xy + dot(n_e0_xy, p.xy);
			float dd_e1_xy = d_e1_xy + dot(n_e1_xy, p.xy);
			float dd_e2_xy = d_e2_xy + dot(n_e2_xy, p.xy);
		
			bool xy_overlap = (dd_e0_xy >= 0) && (dd_e1_xy >= 0) && (dd_e2_xy >= 0);

			if(xy_overlap)	//figure 17 line 15, figure 18 line 14
			{
				float dot_n_p = dot(nProj.xy, p.xy);
#if THICKNESS == THIN
				zMinInt = (-dot_n_p + dTriThin) * nzInv;
				zMaxInt = zMinInt;
#elif THICKNESS == FAT
				zMinInt = (-dot_n_p + dTriFatMin) * nzInv;
				zMaxInt = (-dot_n_p + dTriFatMax) * nzInv;
#endif
				zMinFloor = floor(zMinInt);
				zMaxCeil  =  ceil(zMaxInt);

				zMin = int(zMinFloor) - int(zMinFloor == zMinInt);
				zMax = int(zMaxCeil ) + int(zMaxCeil  == zMaxInt);

				zMin = max(minVoxIndex.z, zMin);	//clamp to bounding box max Z
				zMax = min(maxVoxIndex.z, zMax);	//clamp to bounding box min Z

				for(p.z = zMin; p.z < zMax; p.z++)	//figure 17/18 line 18
				{
					float dd_e0_yz = d_e0_yz + dot(n_e0_yz, p.yz);
					float dd_e1_yz = d_e1_yz + dot(n_e1_yz, p.yz);
					float dd_e2_yz = d_e2_yz + dot(n_e2_yz, p.yz);

					float dd_e0_zx = d_e0_zx + dot(n_e0_zx, p.zx);
					float dd_e1_zx = d_e1_zx + dot(n_e1_zx, p.zx);
					float dd_e2_zx = d_e2_zx + dot(n_e2_zx, p.zx);

					bool yz_overlap = (dd_e0_yz >= 0) && (dd_e1_yz >= 0) && (dd_e2_yz >= 0);
					bool zx_overlap = (dd_e0_zx >= 0) && (dd_e1_zx >= 0) && (dd_e2_zx >= 0);

					if(yz_overlap && zx_overlap)	//figure 17/18 line 19
					{
						writeVoxels(ivec3(unswizzle*p), 1, vec3(0));	//figure 17/18 line 20
					}
				} //z-loop
			} //xy-overlap test
		} //y-loop
	} //x-loop
}

void main()
{
	vec3 n;
	mat3 unswizzle;
	vec3 v0 = In[0].vsVertexPos;
	vec3 v1 = In[1].vsVertexPos;
	vec3 v2 = In[2].vsVertexPos;
	
	swizzleTri(v0, v1, v2, n, unswizzle);

	vec3 AABBmin = min(min(v0, v1), v2);
	vec3 AABBmax = max(max(v0, v1), v2);

	ivec3 minVoxIndex = ivec3(clamp(floor(AABBmin), ivec3(0), voxelResolution));
	ivec3 maxVoxIndex = ivec3(clamp( ceil(AABBmax), ivec3(0), voxelResolution));

	voxelizeTriPostSwizzle(v0, v1, v2, n, unswizzle, minVoxIndex, maxVoxIndex);
}
