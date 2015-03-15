#pragma once

#include <GL/gl.h>
#include <string>
#include <src/log/logger.h>

class Shader
{
public:

	static bool compileProgramFromFile( const std::string& name,
										const std::string& includeBasePath,
										const std::string &vertexShaderFile,
										const std::string &vertexShaderPreprocessor,
										const std::string &fragmentShaderFile,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result,
										Logger* logger = NULL);

	static bool compileProgramFromFile( const std::string& name,
										const std::string& includeBasePath,
										const std::string &vertexShaderFile,
										const std::string &vertexShaderPreprocessor,
										const std::string &geometryShaderFile,
										const std::string &geometryShaderPreprocessor,
										const std::string &fragmentShaderFile,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result,
										Logger* logger = NULL);

	static bool compileProgramFromCode( const std::string& name,
										const std::string &vertexShaderCode,
										const std::string &vertexShaderPreprocessor,
										const std::string &geometryShaderCode,
										const std::string &geometryShaderPreprocessor,
										const std::string &fragmentShaderCode,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result,
										Logger* logger = NULL);

};

struct IntegratorShaderSettings
{
	GLuint m_program;

	// uniforms
	GLuint m_uniformMaterialOffsetTexture;
	GLuint m_uniformMaterialDataTexture;
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
	GLuint m_uniformCameraLensModel;
	GLuint m_uniformPathtracerMaxPathBounces;
	GLuint m_uniformWireframeOpacity;
	GLuint m_uniformWireframeThickness;
	GLuint m_uniformFocalDistanceSSBOStorageBlock;
	GLuint m_uniformSelectedVoxelSSBOStorageBlock;
	GLuint m_uniformBackgroundColorTop;
	GLuint m_uniformBackgroundColorBottom;
	GLuint m_uniformBackgroundUseImage;
	GLuint m_uniformBackgroundTexture;
	GLuint m_uniformBackgroundCDFUTexture;
	GLuint m_uniformBackgroundCDFVTexture;
	GLuint m_uniformBackgroundIntegral;
	GLuint m_uniformBackgroundRotationRadians;
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


