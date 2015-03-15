#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <iostream>

#include "renderer/loaders/voxSlap.h"

bool VoxSlapLoader::load(const std::string& /*filePath*/,
						 std::vector<GLuint>& /*occupancyTexels*/, 
						 std::vector<float>& /*materialData*/,
						 Imath::V3i& /*voxelResolution*/)
{
#if 0
	// Vox format description:
	// long xsiz, ysiz, zsiz; //Variable declarations
	// char voxel[xsiz][ysiz][zsiz];
	// char palette[256][3];

	FILE* fd = fopen(filePath.c_str(), "rb"); if (!fd) return false;
	if (fread(&(voxelResolution.x), sizeof(int), 3, fd) != 3) return false;
	size_t numVoxels = voxelResolution.x * voxelResolution.y * voxelResolution.z;
	occupancyTexels = (GLubyte*)malloc(numVoxels * sizeof(GLubyte));
	if (!occupancyTexels) return false;
	if(fread(occupancyTexels, sizeof(GLubyte), numVoxels, fd) != numVoxels) return false;

	size_t paletteSize = 256 * 3 * sizeof(char);
	char* palette = (char*)malloc(paletteSize);
	if (!palette) return false;
	if(fread(palette, 3 * sizeof(char), paletteSize, fd) != 256) return false;
	fclose(fd);

	colorTexels = (GLubyte*)malloc(numVoxels * 4 * sizeof(GLubyte));
	const unsigned char noVoxel = 255; 
	for(size_t i = 0; i < numVoxels; ++i)
	{
		if(occupancyTexels[i] == noVoxel)
		{
			occupancyTexels[i] = 0;
			continue;
		}
		const char paletteIndex = occupancyTexels[i];
		occupancyTexels[i] = 255;
		memcpy(&colorTexels[4*i], &palette[3*paletteIndex], 3 * sizeof(GLuint));
		colorTexels[4*i+3] = 0;
	}
	free(palette);
	return true;
#else
	return false;
#endif
}

