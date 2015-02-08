#pragma once

#include <GL/gl.h>
#include <string>

class Shader
{
public:

	static bool compileProgramFromFile( const std::string& name,
										const std::string &vertexShaderFile,
										const std::string &vertexShaderPreprocessor,
										const std::string &fragmentShaderFile,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result );

	static bool compileProgramFromFile( const std::string& name,
										const std::string &vertexShaderFile,
										const std::string &vertexShaderPreprocessor,
										const std::string &geometryShaderFile,
										const std::string &geometryShaderPreprocessor,
										const std::string &fragmentShaderFile,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result );


	static bool compileProgramFromCode( const std::string& name,
										const std::string &vertexShaderCode,
										const std::string &vertexShaderPreprocessor,
										const std::string &geometryShaderCode,
										const std::string &geometryShaderPreprocessor,
										const std::string &fragmentShaderCode,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result );

};

struct IntegratorShaderSettings
{
	GLuint m_program;

	// uniforms
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformVoxelColorTexture;
	GLuint m_uniformNoiseTexture;
	GLuint m_uniformVoxelDataResolution;
	GLuint m_uniformVolumeBoundsMin;
	GLuint m_uniformVolumeBoundsMax;
	GLuint m_uniformViewport;
	GLuint m_uniformCameraNear;
	GLuint m_uniformCameraFar;
	GLuint m_uniformCameraProj;
	GLuint m_uniformCameraInverseProj;
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformCameraFocalLength;
	GLuint m_uniformCameraLensRadius;
	GLuint m_uniformCameraFilmSize;
	GLuint m_uniformLightDir;
	GLuint m_uniformSampleCount;
	GLuint m_uniformEnableDOF;
	GLuint m_uniformPathtracerMaxPathLength;
	GLuint m_uniformWireframeOpacity;
	GLuint m_uniformWireframeThickness;
	GLuint m_uniformFocalDistanceSSBOStorageBlock;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
};
			
struct AccumulationShaderSettings
{
	GLuint m_program;
	
	// uniforms
	GLuint m_uniformSampleTexture;
	GLuint m_uniformAverageTexture;
	GLuint m_uniformSampleCount;
    GLuint m_uniformViewport;
};

struct TexturedShaderSettings 
{
	GLuint m_program;
	
	// uniforms
	GLuint m_uniformTexture;
    GLuint m_uniformViewport;
};

struct PickingShaderSettings
{
	GLuint m_program;
	
	// uniforms
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformVoxelDataResolution;
	GLuint m_uniformVolumeBoundsMin;
	GLuint m_uniformVolumeBoundsMax;
	GLuint m_uniformViewport;
	GLuint m_uniformCameraNear;
	GLuint m_uniformCameraFar;
	GLuint m_uniformCameraProj;
	GLuint m_uniformCameraInverseProj;
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformCameraFocalLength;
	GLuint m_uniformSampledFragment;
	GLuint m_uniformSSBOStorageBlock;
};

typedef PickingShaderSettings FocalDistanceShaderSettings;
typedef PickingShaderSettings SelectActiveVoxelShaderSettings;

struct AddVoxelShaderSettings
{
	GLuint m_program;

	// uniforms
	GLuint m_uniformCameraInverseModelView;
	GLuint m_uniformScreenSpaceMotion;
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformVoxelColorTexture;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
	GLuint m_uniformNewVoxelColor;
};

struct RemoveVoxelShaderSettings
{
	GLuint m_program;

	// uniforms
	GLuint m_uniformVoxelOccupancyTexture;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
};

struct VoxelizeShaderSettings
{
	GLuint m_program;

	GLuint m_uniformVoxelDataResolution;
	GLuint m_uniformModelTransform;
	GLuint m_uniformVoxelOccupancyTexture;
};

