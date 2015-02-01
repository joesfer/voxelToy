#version 430

#include <selectVoxelDevice.h>

uniform vec4		newVoxelColor = vec4(0,0,1,0);

//Voxel output
layout(r8ui, binding = 0) uniform uimage3D voxelOccupancy;
layout(rgba8, binding = 1) uniform image3D voxelColor;


void main()
{
	ivec3 coord = SelectVoxelData.index.xyz + ivec3(SelectVoxelData.normal.xyz);
	imageStore(voxelOccupancy, coord, uvec4(255));
	imageStore(voxelColor, coord, newVoxelColor);
}

