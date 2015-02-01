// Device-side declaration of the Shader Storage Object struct used to read and
// store the focal distance
#extension GL_ARB_shader_storage_buffer_object : require

layout(std430, binding=0) buffer FocalDistanceData_t
{
	float focalDistance;
} FocalDistanceData;
