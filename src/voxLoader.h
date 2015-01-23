#pragma once
#include <string>
#include <GL/gl.h>
#include <OpenEXR/ImathVec.h>

class VoxLoader
{
public:
	virtual bool load(const std::string& filePath,
					  GLubyte*& occupancyTexels, GLubyte*& colorTexels,
					  Imath::V3i& voxelResolution) = 0;
};

class VoxSlapLoader : public VoxLoader
{
public:
	virtual bool load(const std::string& filePath,
					  GLubyte*& occupancyTexels, GLubyte*& colorTexels,
					  Imath::V3i& voxelResolution);
};

class MagicaVoxelLoader: public VoxLoader
{
public:
	virtual bool load(const std::string& filePath,
					  GLubyte*& occupancyTexels, GLubyte*& colorTexels,
					  Imath::V3i& voxelResolution);
};
