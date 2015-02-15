#version 430

#include <editVoxels/selectVoxelDevice.h>

//Voxel output
layout(r8ui, binding = 0) uniform uimage3D voxelOccupancy;

void main()
{
	imageStore(voxelOccupancy, SelectVoxelData.index.xyz, uvec4(0));
}

