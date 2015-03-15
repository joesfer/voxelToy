#include "renderer/material/material.h"
#include <sstream>
using namespace Material;

SerializedData serializeFloat(const char* name, 
							  size_t dataOffset,
							  float value, 
							  float min, 
							  float max)
{
	SerializedData result;
	result.m_propertyName = name;
	result.m_dataOffset = dataOffset;
	result.m_propertyType = SerializedData::PROPERTY_TYPE_FLOAT;
	result.m_dataRangeFrom = min;
	result.m_dataRangeTo = max;
	result.m_value = value;
	
	return result;
}

SerializedData serializeColor(const char* name, 
							  size_t dataOffset, 
							  const float rgb[3])
{
	SerializedData result;
	result.m_propertyName = name;
	result.m_dataOffset = dataOffset;
	result.m_propertyType = SerializedData::PROPERTY_TYPE_COLOR;
	result.m_dataRangeFrom = 0;
	result.m_dataRangeTo = 0;
	result.m_value = 0;
	result.m_childProperties.push_back(serializeFloat("red"   , dataOffset     , rgb[0] , 0 , 1));
	result.m_childProperties.push_back(serializeFloat("green" , dataOffset + 1 , rgb[1] , 0 , 1));
	result.m_childProperties.push_back(serializeFloat("blue"  , dataOffset + 2 , rgb[2] , 0 , 1));
	return result;
}

SerializedData Material::serializeLambert(const LambertMaterialData& material,
									      size_t dataOffset)
{
	SerializedData data;
	data.m_propertyName = "Lambert Material";
	data.m_dataOffset = dataOffset;
	data.m_value = data.m_dataRangeFrom = data.m_dataRangeTo = 0;

	data.m_childProperties.push_back(serializeColor("emission" , dataOffset + 1 , material.emission));
	data.m_childProperties.push_back(serializeColor("albedo"   , dataOffset + 4 , material.albedo));

	return data;
}

SerializedData Material::serializeMetal(const MetalMaterialData& material,
									    size_t dataOffset)
{
	SerializedData data;
	data.m_propertyName = "Metal Material";
	data.m_dataOffset = dataOffset;
	data.m_value = data.m_dataRangeFrom = data.m_dataRangeTo = 0;

	data.m_childProperties.push_back(serializeColor("emission"    , dataOffset + 1 , material.emission));
	data.m_childProperties.push_back(serializeColor("reflectance" , dataOffset + 4 , material.reflectance));
	data.m_childProperties.push_back(serializeFloat("roughness"   , dataOffset + 7 , material.roughness      , 0 , 1000));

	return data;
}

SerializedData Material::serializePlastic(const PlasticMaterialData& material, 
										  size_t dataOffset)
{
	SerializedData data;
	data.m_propertyName = "Plastic Material";
	data.m_dataOffset = dataOffset;
	data.m_value = data.m_dataRangeFrom = data.m_dataRangeTo = 0;

	data.m_childProperties.push_back(serializeColor("emission"      , dataOffset + 1 , material.emission));
	data.m_childProperties.push_back(serializeColor("diffuseAlbedo" , dataOffset + 4 , material.diffuseAlbedo));
	data.m_childProperties.push_back(serializeFloat("roughness"     , dataOffset + 7 , material.roughness        , 0 , 1000));

	return data;
}

