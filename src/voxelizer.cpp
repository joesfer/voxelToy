#include "voxelizer.h"
#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathFun.h>

#include "thirdParty/boost/threadpool.hpp"

using namespace boost::threadpool;

class VecSwizzle
{
public:
	static Imath::V3f yzx(const Imath::V3f& v) { return Imath::V3f(v.y, v.z, v.x); }
	static Imath::V3f zxy(const Imath::V3f& v) { return Imath::V3f(v.z, v.x, v.y); }
};

// Thin voxelization is when adjacent voxels are at least connected by vertices
#define THIN 0 
// Fat voxelization is when adjacent voxels need to share at least a face
#define FAT  1

#define THICKNESS THIN

// Look-up table of permutations matrices used to reverse triangle swizzling and
// restore vertices to their original orientation.
const Imath::M44f unswizzleLUT[3] = { Imath::M44f(0,1,0,0,
 											      0,0,1,0,
											      1,0,0,0,
												  0,0,0,1), 
									 Imath::M44f(0,0,1,0,
											     1,0,0,0,
											     0,1,0,0,
												 0,0,0,1), 
									 Imath::M44f(1,0,0,0,
											     0,1,0,0,
											     0,0,1,0,
												 0,0,0,1) };
// swizzle triangle vertices -- determine the dominant axis-aligned plane for a
// given triangle (that where the triangle projection is largest) and rotate the
// triangle vertices to make that plane always be the XY plane. This method also
// returns the swizzling matrix so that we can undo this transformation later
// on.
void swizzleTri(Imath::V3f& v0, 
				Imath::V3f& v1, 
				Imath::V3f& v2, 
				Imath::V3f& n, 
				Imath::M44f& unswizzle)
{
	using namespace Imath;

	//       cross(e0, e1);
	n =	(v1 - v0).cross(v2 - v1);

	Imath::V3f absN(abs(n.x), abs(n.y), abs(n.z));

	if(absN.x >= absN.y && absN.x >= absN.z)			
	{													
		//X-direction dominant (YZ-plane)
		//Then you want to look down the X-direction

		v0 = VecSwizzle::yzx(v0);
		v1 = VecSwizzle::yzx(v1);
		v2 = VecSwizzle::yzx(v2);
		
		n = VecSwizzle::yzx(n);

		//XYZ <-> YZX
		unswizzle = unswizzleLUT[0];
	}
	else if(absN.y >= absN.x && absN.y >= absN.z)		
	{													
		//Y-direction dominant (ZX-plane)
		//Then you want to look down the Y-direction

		v0 = VecSwizzle::zxy(v0);
		v1 = VecSwizzle::zxy(v1);
		v2 = VecSwizzle::zxy(v2);

		n  = VecSwizzle::zxy(n);

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

inline void writeVoxels(Imath::V3i coord, 
						unsigned char val,
						const Imath::V3i voxelDimensions,
						unsigned char* voxelStorage)
{
	//modify as necessary for attributes/storage type
	voxelStorage[ coord.x + 
				  coord.y * voxelDimensions.x + 
                  coord.z * voxelDimensions.x * voxelDimensions.y ] = val;
}

void voxelizeTriPostSwizzle(Imath::V3f v0, 
							Imath::V3f v1, 
							Imath::V3f v2, 
							Imath::V3f n, 
							const Imath::M44f& unswizzle, 
							Imath::V3i minVoxIndex, 
							Imath::V3i maxVoxIndex,
							const Imath::V3i voxelDimensions,
							unsigned char* voxelStorage)
{
	using namespace Imath;

	V3f e0 = v1 - v0;	//figure 17/18 line 2
	V3f e1 = v2 - v1;	//figure 17/18 line 2
	V3f e2 = v0 - v2;	//figure 17/18 line 2

	//INward Facing edge normals XY
	V2f n_e0_xy = (n.z >= 0) ? V2f(-e0.y, e0.x) : V2f(e0.y, -e0.x);	//figure 17/18 line 4
	V2f n_e1_xy = (n.z >= 0) ? V2f(-e1.y, e1.x) : V2f(e1.y, -e1.x);	//figure 17/18 line 4
	V2f n_e2_xy = (n.z >= 0) ? V2f(-e2.y, e2.x) : V2f(e2.y, -e2.x);	//figure 17/18 line 4

	//INward Facing edge normals YZ
	V2f n_e0_yz = (n.x >= 0) ? V2f(-e0.z, e0.y) : V2f(e0.z, -e0.y);	//figure 17/18 line 5
	V2f n_e1_yz = (n.x >= 0) ? V2f(-e1.z, e1.y) : V2f(e1.z, -e1.y);	//figure 17/18 line 5
	V2f n_e2_yz = (n.x >= 0) ? V2f(-e2.z, e2.y) : V2f(e2.z, -e2.y);	//figure 17/18 line 5

	//INward Facing edge normals ZX
	V2f n_e0_zx = (n.y >= 0) ? V2f(-e0.x, e0.z) : V2f(e0.x, -e0.z);	//figure 17/18 line 6
	V2f n_e1_zx = (n.y >= 0) ? V2f(-e1.x, e1.z) : V2f(e1.x, -e1.z);	//figure 17/18 line 6
	V2f n_e2_zx = (n.y >= 0) ? V2f(-e2.x, e2.z) : V2f(e2.x, -e2.z);	//figure 17/18 line 6

#if THICKNESS == THIN
	float d_e0_xy = n_e0_xy.dot(V2f(0.5f, 0.5f) - V2f(v0.x, v0.y)) + 0.5f * std::max(abs(n_e0_xy.x), abs(n_e0_xy.y));	//figure 18 line 7
	float d_e1_xy = n_e1_xy.dot(V2f(0.5f, 0.5f) - V2f(v1.x, v1.y)) + 0.5f * std::max(abs(n_e1_xy.x), abs(n_e1_xy.y));	//figure 18 line 7
	float d_e2_xy = n_e2_xy.dot(V2f(0.5f, 0.5f) - V2f(v2.x, v2.y)) + 0.5f * std::max(abs(n_e2_xy.x), abs(n_e2_xy.y));	//figure 18 line 7
                                                                       
	float d_e0_yz = n_e0_yz.dot(V2f(0.5f, 0.5f) - V2f(v0.y, v0.z)) + 0.5f * std::max(abs(n_e0_yz.x), abs(n_e0_yz.y));	//figure 18 line 8
	float d_e1_yz = n_e1_yz.dot(V2f(0.5f, 0.5f) - V2f(v1.y, v1.z)) + 0.5f * std::max(abs(n_e1_yz.x), abs(n_e1_yz.y));	//figure 18 line 8
	float d_e2_yz = n_e2_yz.dot(V2f(0.5f, 0.5f) - V2f(v2.y, v2.z)) + 0.5f * std::max(abs(n_e2_yz.x), abs(n_e2_yz.y));	//figure 18 line 8
                                                                       
	float d_e0_zx = n_e0_zx.dot(V2f(0.5f, 0.5f) - V2f(v0.z, v0.x)) + 0.5f * std::max(abs(n_e0_zx.x), abs(n_e0_zx.y));	//figure 18 line 9
	float d_e1_zx = n_e1_zx.dot(V2f(0.5f, 0.5f) - V2f(v1.z, v1.x)) + 0.5f * std::max(abs(n_e1_zx.x), abs(n_e1_zx.y));	//figure 18 line 9
	float d_e2_zx = n_e2_zx.dot(V2f(0.5f, 0.5f) - V2f(v2.z, v2.x)) + 0.5f * std::max(abs(n_e2_zx.x), abs(n_e2_zx.y));	//figure 18 line 9
#elif THICKNESS == FAT
	float d_e0_xy = -n_e0_xy.dot(V2f(v0.x, v0.y)) + std::max(0.0f, n_e0_xy.x) + std::max(0.0f, n_e0_xy.y);	//figure 17 line 7
	float d_e1_xy = -n_e1_xy.dot(V2f(v1.x, v1.y)) + std::max(0.0f, n_e1_xy.x) + std::max(0.0f, n_e1_xy.y);	//figure 17 line 7
	float d_e2_xy = -n_e2_xy.dot(V2f(v2.x, v2.y)) + std::max(0.0f, n_e2_xy.x) + std::max(0.0f, n_e2_xy.y);	//figure 17 line 7
                                                 
	float d_e0_yz = -n_e0_yz.dot(V2f(v0.y, v0.z)) + std::max(0.0f, n_e0_yz.x) + std::max(0.0f, n_e0_yz.y);	//figure 17 line 8
	float d_e1_yz = -n_e1_yz.dot(V2f(v1.y, v1.z)) + std::max(0.0f, n_e1_yz.x) + std::max(0.0f, n_e1_yz.y);	//figure 17 line 8
	float d_e2_yz = -n_e2_yz.dot(V2f(v2.y, v2.z)) + std::max(0.0f, n_e2_yz.x) + std::max(0.0f, n_e2_yz.y);	//figure 17 line 8
                                                 
	float d_e0_zx = -n_e0_zx.dot(V2f(v0.z, v0.x)) + std::max(0.0f, n_e0_zx.x) + std::max(0.0f, n_e0_zx.y);	//figure 18 line 9
	float d_e1_zx = -n_e1_zx.dot(V2f(v1.z, v1.x)) + std::max(0.0f, n_e1_zx.x) + std::max(0.0f, n_e1_zx.y);	//figure 18 line 9
	float d_e2_zx = -n_e2_zx.dot(V2f(v2.z, v2.x)) + std::max(0.0f, n_e2_zx.x) + std::max(0.0f, n_e2_zx.y);	//figure 18 line 9
#endif

	V3f nProj = (n.z < 0.0) ? -n : n;	//figure 17/18 line 10

	const float dTri = nProj.dot(v0);
#if THICKNESS == THIN
	const float dTriThin   = dTri - V2f(nProj.x, nProj.y).dot(V2f(0.5));	//figure 18 line 11
#elif THICKNESS == FAT
	const float dTriFatMin = dTri - std::max(nProj.x, 0) - std::max(nProj.y, 0);	//figure 17 line 11
	const float dTriFatMax = dTri - min(nProj.x, 0) - min(nProj.y, 0);	//figure 17 line 12
#endif

	const float nzInv = 1.0 / nProj.z;
	
	V3i p;					//voxel coordinate
	int   zMin,      zMax;		//voxel Z-range
	float zMinInt,   zMaxInt;	//voxel Z-intersection min/max
	float zMinFloor, zMaxCeil;	//voxel Z-intersection floor/ceil
	for(p.x = minVoxIndex.x; p.x < maxVoxIndex.x; p.x++)	//figure 17 line 13, figure 18 line 12
	{
		for(p.y = minVoxIndex.y; p.y < maxVoxIndex.y; p.y++)	//figure 17 line 14, figure 18 line 13
		{
			float dd_e0_xy = d_e0_xy + n_e0_xy.dot(V2f(p.x, p.y));
			float dd_e1_xy = d_e1_xy + n_e1_xy.dot(V2f(p.x, p.y));
			float dd_e2_xy = d_e2_xy + n_e2_xy.dot(V2f(p.x, p.y));
		
			bool xy_overlap = (dd_e0_xy >= 0) && (dd_e1_xy >= 0) && (dd_e2_xy >= 0);

			if(xy_overlap)	//figure 17 line 15, figure 18 line 14
			{
				float dot_n_p = V2f(nProj.x, nProj.y).dot(V2f(p.x, p.y));
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

				zMin = std::max(minVoxIndex.z, zMin);	//clamp to bounding box max Z
				zMax = std::min(maxVoxIndex.z, zMax);	//clamp to bounding box min Z

				for(p.z = zMin; p.z < zMax; p.z++)	//figure 17/18 line 18
				{
					float dd_e0_yz = d_e0_yz + n_e0_yz.dot(V2f(p.y, p.z));
					float dd_e1_yz = d_e1_yz + n_e1_yz.dot(V2f(p.y, p.z));
					float dd_e2_yz = d_e2_yz + n_e2_yz.dot(V2f(p.y, p.z));
                                                                  
					float dd_e0_zx = d_e0_zx + n_e0_zx.dot(V2f(p.z, p.x));
					float dd_e1_zx = d_e1_zx + n_e1_zx.dot(V2f(p.z, p.x));
					float dd_e2_zx = d_e2_zx + n_e2_zx.dot(V2f(p.z, p.x));

					bool yz_overlap = (dd_e0_yz >= 0) && (dd_e1_yz >= 0) && (dd_e2_yz >= 0);
					bool zx_overlap = (dd_e0_zx >= 0) && (dd_e1_zx >= 0) && (dd_e2_zx >= 0);

					if(yz_overlap && zx_overlap)	//figure 17/18 line 19
					{
						V3f unswizzled; 
						unswizzle.multDirMatrix(V3f(p), unswizzled);
						writeVoxels(V3i(unswizzled), 1, voxelDimensions, voxelStorage);	//figure 17/18 line 20
					}
				} //z-loop
			} //xy-overlap test
		} //y-loop
	} //x-loop
}


void voxelizeTriangleTask(const Imath::V3f* vertices,
						  const unsigned int* indices,
						  unsigned int fromTriangle,
						  unsigned int toTriangle,
						  const Imath::V3i voxelDimensions,
						  unsigned char* voxelStorage)
{
	using namespace Imath;

	for(unsigned int triangle = fromTriangle; triangle <= toTriangle; ++triangle)
	{

		V3f n;
		M44f unswizzle;
		V3f v0 = vertices[indices[3 * triangle + 0]];
		V3f v1 = vertices[indices[3 * triangle + 1]];
		V3f v2 = vertices[indices[3 * triangle + 2]];
		
		swizzleTri(v0, v1, v2, n, unswizzle);

		V3f AABBmin(std::min(std::min(v0.x, v1.x), v2.x),
					std::min(std::min(v0.y, v1.y), v2.y),
					std::min(std::min(v0.z, v1.z), v2.z));
		V3f AABBmax(std::max(std::max(v0.x, v1.x), v2.x),
					std::max(std::max(v0.y, v1.y), v2.y),
					std::max(std::max(v0.z, v1.z), v2.z));

		V3i minVoxIndex = V3i(clamp(floor(AABBmin.x), 0, voxelDimensions.x),
							  clamp(floor(AABBmin.y), 0, voxelDimensions.y),
							  clamp(floor(AABBmin.z), 0, voxelDimensions.z));
		V3i maxVoxIndex = V3i(clamp( ceil(AABBmax.x), 0, voxelDimensions.x),
							  clamp( ceil(AABBmax.y), 0, voxelDimensions.y),
							  clamp( ceil(AABBmax.z), 0, voxelDimensions.z));

		voxelizeTriPostSwizzle(v0, v1, v2, n, 
							   unswizzle, 
							   minVoxIndex, maxVoxIndex,
							   voxelDimensions,
							   voxelStorage);
	}
}


/*static*/ void Voxelizer::voxelizeMesh(const Imath::V3f* vertices,
										const unsigned int* indices,
										unsigned int numTriangles,
										const Imath::V3i& voxelDimensions,
										unsigned char* voxelStorage)
{
	unsigned int numThreads = 8;
	pool tp(numThreads);
	unsigned int chunkSize = numTriangles / numThreads;

	for(unsigned int i = 0; i < numTriangles; ++i)
	{
		unsigned int fromTriangle = i * chunkSize;
		unsigned int toTriangle = std::min(numTriangles - 1, (i+1) * chunkSize);
		tp.schedule(boost::bind(voxelizeTriangleTask, 
								vertices,
								indices,
								fromTriangle, 
								toTriangle,
								voxelDimensions,
								voxelStorage));
	}
}

