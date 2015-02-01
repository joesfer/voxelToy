#version 420

// IN (from OpenGL)
layout(location = 0) in vec3 in_vertexPos;

// Input uniforms
uniform ivec3 voxelResolution;
uniform mat4 modelTransform;

out block
{
	vec3 vsVertexPos; // voxel-space vertex position
} Out;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	// model vertices are defined in the unit cube
	vec4 lsVertexPos = modelTransform * vec4(in_vertexPos, 1);
	// translate them to voxel space
	Out.vsVertexPos = lsVertexPos.xyz * voxelResolution;
	
	gl_Position = lsVertexPos;
}
