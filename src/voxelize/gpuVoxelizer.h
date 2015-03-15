#pragma once

#include <OpenEXR/ImathMatrix.h>
#include <GL/gl.h>

class Mesh;
class Logger;

class GPUVoxelizer
{
public:
	GPUVoxelizer(const std::string& shaderPath, Logger* logger = NULL);
	~GPUVoxelizer();

	bool voxelizeMesh(const Mesh* mesh,
					  const Imath::M44f& meshTransform,
					  const Imath::V3i& resolution,
					  GLuint textureUnit);
private:
	bool m_initialized;
	GLuint m_program;
	GLuint m_uniformVoxelDataResolution;
	GLuint m_uniformModelTransform;
	GLuint m_uniformMaterialOffsetTexture;
};
