#pragma once

namespace Material
{
	enum MaterialType
	{
		MT_LAMBERT = 0,
		MT_METAL = 1,
		MT_PLASTIC = 2,
	};

	struct LambertMaterialData
	{
		float emission[3];
		float albedo[3];
	};

	struct MetalMaterialData 
	{
		float emission[3];
		float reflectance[3];
		float roughness;
	};

	struct PlasticMaterialData 
	{
		float emission[3];
		float diffuseAlbedo[3];
		float roughness;
	};

} // namespace Material
