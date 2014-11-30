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

	static bool compileProgramFromCode( const std::string& name,
										const std::string &vertexShaderCode,
										const std::string &vertexShaderPreprocessor,
										const std::string &fragmentShaderCode,
										const std::string &fragmentShaderPreprocessor,
										GLuint& result );
			
};
