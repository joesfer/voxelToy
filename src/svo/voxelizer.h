#pragma once

#include <OpenEXR/ImathVec.h>
#include <stdint.h>

class Voxelizer
{
public:
	static void voxelizeMeshDenseGrid(const Imath::V3f* vertices,       // input mesh vertices
									  const unsigned int* indices,      // input mesh indices
									  unsigned int numTriangles,        // number of triangles in the mesh = |indices| / 3
									  const Imath::V3i voxelDimensions, // dense voxel grid dimensions
									  unsigned char* voxelStorage);     // dense voxel grid storage to operate upon, of specified dimensions

	static bool voxelizeMeshMorton(const Imath::V3f* vertices,          // input mesh vertices
								   const unsigned int* indices,         // input mesh indices
								   unsigned int numTriangles,           // number of triangles in the mesh = |indices| / 3
								   const Imath::V3i voxelDimensions,    // dimensions of the SVO's lowest level
								   uint64_t* mortonCodes,               // storage for resulting morton codes
								   size_t mortonCodesCapacity,          // maximum capacity for supplied storage
                                   size_t& numMortonCodes);             // output number of morton codes generated
};
