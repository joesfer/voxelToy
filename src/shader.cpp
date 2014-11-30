#include <GL/glew.h>

#include <shader.h>
#include <stdio.h>
#include <iostream>

void log( const std::string& msg )
{
	std::cout << msg << std::endl;
}

bool parseShader( const std::string& file, std::string& contents )
{
	using namespace std;

	FILE* fd = fopen( file.c_str(), "rt" );
	if ( fd == NULL ) 
	{
		return false;
	}
	fseek(fd, 0, SEEK_END); //go to end
	long len = ftell( fd ); //get position at end (length)
	if ( len > 0 )
	{
		fseek( fd, 0, SEEK_SET); //go to beginning
		contents.resize( len );
		fread( &contents[0], len, 1, fd); //read into buffer
	}
	fclose(fd);
	return len > 0;
}

bool Shader::compileProgramFromFile( const std::string& name,
									 const std::string &vertexShaderFile,
									 const std::string &vertexShaderPreprocessor,
									 const std::string &fragmentShaderFile,
									 const std::string &fragmentShaderPreprocessor,
									 GLuint& result )
{
	using namespace std;

	string vs, fs;
	if (!parseShader( vertexShaderFile, vs ))
	{
		log(std::string("error parsing file ") + vertexShaderFile);
		return false;
	}
	if (!parseShader( fragmentShaderFile, fs ))
	{
		log(std::string("error parsing file ") + fragmentShaderFile);
		return false;
	}

	return compileProgramFromCode( name,
								   vs, vertexShaderPreprocessor,
								   fs, fragmentShaderPreprocessor,
								   result );
}

bool Shader::compileProgramFromCode ( const std::string& name,
									  const std::string &vertexShaderCode,
									  const std::string &vertexShaderPreprocessor,
									  const std::string &fragmentShaderCode,
									  const std::string &fragmentShaderPreprocessor,
									  GLuint& result )
{
    glewInit();
	GLuint program = glCreateProgram();

	char infoLog[ 512 ];
	GLsizei logLength = 0;

	{

		// compile vertex shader
		{
			GLuint vs = glCreateShader( GL_VERTEX_SHADER );
			const char* source[2] = { vertexShaderPreprocessor.c_str(),
									  vertexShaderCode.c_str() };
			glShaderSource( vs, 2, source, NULL );
			glCompileShader( vs );
			glGetShaderInfoLog( vs, 512, &logLength, infoLog );
			if ( logLength > 0 )
			{
				log( std::string("Program ") + name + std::string(": Vertex shader compilation log:\n") + infoLog);
				log( vertexShaderPreprocessor );
				log( vertexShaderCode );
			}

			glAttachShader( program, vs );
			
			// mark this copy for disposal. It won't be actually deleted till we 
			// dispose the program it is attached to. 
			glDeleteShader( vs ); 
		}

		// compile fragment shader
		{
			GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );

			const char* source[2] = { fragmentShaderPreprocessor.c_str(),
									  fragmentShaderCode.c_str() };
			glShaderSource( fs, 2, source, NULL );
			glCompileShader( fs );
			glGetShaderInfoLog( fs, 512, &logLength, infoLog );
			if ( logLength > 0 )
			{
			   log( std::string("Program ") + name + std::string(": Fragment shader compilation log:\n") + infoLog );
				log( fragmentShaderPreprocessor );
				log( fragmentShaderCode );
			}

			glAttachShader( program, fs );
			
			// mark this copy for disposal. It won't be actually deleted till we 
			// dispose the program it is attached to. 
			glDeleteShader( fs );
		}

		glLinkProgram( program );
	}

	result = program;
	GLint linked; 
	glGetProgramiv( program, GL_LINK_STATUS, &linked);
	glGetProgramInfoLog( program, 512, &logLength, infoLog );
	if ( logLength > 0 )
	{
		log( std::string("Program ") + name + std::string(": Fragment shader compilation log:\n") + infoLog );
	}
	return linked == GL_TRUE;
}

