#pragma once

#include <OpenEXR/ImathVec.h>

class Voxelizer
{
public:
	static void voxelizeMesh(const Imath::V3f* vertices,
						     const unsigned int* indices,
						     unsigned int numTriangles,
						     const Imath::V3i& voxelDimensions,
						     unsigned char* voxelStorage);
};
