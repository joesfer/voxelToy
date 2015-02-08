// Device-side declaration of the Shader Storage Object struct used to read and
// store the focal distance
#extension GL_ARB_shader_storage_buffer_object : require

layout(std430, binding=1) buffer SelectVoxelData_t
{
	ivec4 index;
	vec4  normal;
} SelectVoxelData;
