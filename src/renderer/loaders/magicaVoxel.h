#pragma once
#include "renderer/loaders/voxLoader.h"

class MagicaVoxelLoader: public VoxLoader
{
public:
	virtual bool load(const std::string& filePath,
					  std::vector<GLint>& voxelMaterials, 
					  std::vector<float>& materialData,
					  std::vector<GLint>& emissiveVoxelIndices,
					  Imath::V3i& voxelResolution);
};

