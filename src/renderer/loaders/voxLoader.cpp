#include "renderer/loaders/voxLoader.h"
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <iostream>

#include "renderer/material/material.h"
void VoxLoader::generateMaterialLambert(Imath::V3f emission,
									    Imath::V3f albedo,
									    std::vector<float>& materialData)
{
	using namespace Material;
	LambertMaterialData data;
	data.emission[0] = emission.x ; data.emission[1] = emission.y ; data.emission[2] = emission.z ;
	data.albedo[0] = albedo.x     ; data.albedo[1] = albedo.y     ; data.albedo[2] = albedo.z     ;
	storeMaterialData(MT_LAMBERT, 
					  reinterpret_cast<float*>(&data), 
					  sizeof(LambertMaterialData) / sizeof(float), 
					  materialData);
}

void VoxLoader::generateMaterialMetal(Imath::V3f emission,
								      Imath::V3f reflectance,
								      float roughness,
								      std::vector<float>& materialData)
{
	using namespace Material;
	MetalMaterialData data;
	data.emission[0] = emission.x       ; data.emission[1] = emission.y       ; data.emission[2] = emission.z       ;
	data.reflectance[0] = reflectance.x ; data.reflectance[1] = reflectance.y ; data.reflectance[2] = reflectance.z ;
	data.roughness = roughness;
	storeMaterialData(MT_METAL, 
					  reinterpret_cast<float*>(&data), 
					  sizeof(MetalMaterialData) / sizeof(float), 
					  materialData);
}

void VoxLoader::generateMaterialPlastic(Imath::V3f emission,
									    Imath::V3f diffuseAlbedo,
									    float roughness,
									    std::vector<float>& materialData)
{
	using namespace Material;
	PlasticMaterialData data;
	data.emission[0] = emission.x           ; data.emission[1] = emission.y           ; data.emission[2] = emission.z           ;
	data.diffuseAlbedo[0] = diffuseAlbedo.x ; data.diffuseAlbedo[1] = diffuseAlbedo.y ; data.diffuseAlbedo[2] = diffuseAlbedo.z ;
	data.roughness = roughness;
	storeMaterialData(MT_PLASTIC, 
					  reinterpret_cast<float*>(&data), 
					  sizeof(PlasticMaterialData) / sizeof(float), 
					  materialData);
}

void VoxLoader::storeMaterialData(Material::MaterialType type,
							      const float* data,
							      size_t dataSize,
							      std::vector<float>& storage)
{
	size_t offset = storage.size();
	storage.resize(offset 
				   + 1  // type flag
				   + dataSize // material data
				   );
	storage[offset] = (float)type;
	memcpy(&storage[offset + 1], data, dataSize * sizeof(float)); 
}

float VoxLoader::getMaterialEmisiveness(const float* materialData)
{
	using namespace Material;
	MaterialType type = (MaterialType)*materialData;
	switch(type)
	{
		case MT_LAMBERT:
		{
			const LambertMaterialData* data = reinterpret_cast<const LambertMaterialData*>(materialData);	
			return (data->emission[0] + data->emission[1] + data->emission[2])/3;
		} 
		case MT_METAL:
		{
			const MetalMaterialData* data = reinterpret_cast<const MetalMaterialData*>(materialData);	
			return (data->emission[0] + data->emission[1] + data->emission[2])/3;
		} 
		case MT_PLASTIC:
		{
			const PlasticMaterialData* data = reinterpret_cast<const PlasticMaterialData*>(materialData);	
			return (data->emission[0] + data->emission[1] + data->emission[2])/3;
		} 
		default: return 0.0f;
	}
}

