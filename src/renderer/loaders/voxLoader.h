#pragma once
#include <string>
#include <GL/gl.h>
#include <OpenEXR/ImathVec.h>
#include <vector>

#include "renderer/material/material.h"

class VoxLoader
{
public:
	// Load voxel data from a file.
	//
	// The voxel information is returned in a dense grid of 'voxelResolution'
	// dimensions. Each grid element contains a 32-bit offset to the
	// materialData array holding its properties, or -1 if the voxel is
	// empty.
	// materialData is an opaque array holding all the information for the scene
	// materials. The data is stored in pairs of blocks [Type][properties],
	// where the size of [properties] depends on each material type. The offset
	// from the voxelMaterials array elements always points to the [Type] block
	// of each material.
	virtual bool load(const std::string& filePath,
					  std::vector<GLint>& voxelMaterials, 
					  std::vector<float>& materialData,
					  Imath::V3i& voxelResolution) = 0;
protected:
	// Append material data into opaque array
	void generateMaterialLambert(Imath::V3f emission,
								 Imath::V3f albedo,
								 std::vector<float>& materialData);
	void generateMaterialMetal(Imath::V3f emission,
							   Imath::V3f reflectance,
							   float roughness,
							   std::vector<float>& materialData);
	void generateMaterialPlastic(Imath::V3f emission,
							     Imath::V3f diffuseAlbedo,
								 float roughness,
							     std::vector<float>& materialData);
private:
	void storeMaterialData(Material::MaterialType type,
						   const float* data,
						   size_t dataSize,
						   std::vector<float>& storage);
};
