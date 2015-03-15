#pragma once

#include <string>
#include <vector>

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

	struct SerializedData
	{
		std::string m_propertyName;
		float m_dataRangeFrom, m_dataRangeTo;
		float m_value;
		size_t m_dataOffset;
		enum
		{
			PROPERTY_TYPE_FLOAT,
			PROPERTY_TYPE_COLOR,
		} m_propertyType;
		std::vector<SerializedData> m_childProperties;
	};

	SerializedData serializeLambert(const LambertMaterialData&, size_t);
	SerializedData serializeMetal(const MetalMaterialData&, size_t);
	SerializedData serializePlastic(const PlasticMaterialData&, size_t);

} // namespace Material
