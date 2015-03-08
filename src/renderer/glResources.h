#include <GL/gl.h>

#include <OpenEXR/ImathVec.h>

#pragma once

struct GLResourceConfiguration
{
	enum TextureUnits
	{
		TEXTURE_UNIT_OCCUPANCY = 0,
		TEXTURE_UNIT_COLOR,
		TEXTURE_UNIT_SAMPLE,
		TEXTURE_UNIT_AVERAGE0,
		TEXTURE_UNIT_AVERAGE1,
		TEXTURE_UNIT_NOISE,
		TEXTURE_UNIT_BACKGROUND,
		TEXTURE_UNIT_BACKGROUND_CDF_U,
		TEXTURE_UNIT_BACKGROUND_CDF_V
	};

	static const GLuint m_focalDistanceSSBOBindingPointIndex = 0;
	static const GLuint m_selectedVoxelSSBOBindingPointIndex = 1;
	
	GLuint m_mainFBO;
	GLuint m_mainRBO;
	GLuint m_sampleTexture;
	GLuint m_averageTexture[2];
	GLuint m_noiseTexture;
	GLint m_textureDimensions[2];

	GLuint m_focalDistanceSSBO;
	GLuint m_selectedVoxelSSBO;

	GLuint m_occupancyTexture;
	GLuint m_voxelColorTexture;
	GLuint m_backgroundTexture;
	GLuint m_backgroundCDFUTexture;
	GLuint m_backgroundCDFVTexture;

	Imath::V3i	 m_volumeResolution;
};


