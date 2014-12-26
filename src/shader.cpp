#include <GL/glew.h>

#include "shader.h"
#include <stdio.h>
#include <iostream>

void log( const std::string& msg )
{
	std::cout << msg << std::endl;
}

bool loadFile( const std::string& file, std::string& contents )
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

// search for #include directives (not supported in GLSL, and emulate them
// by pasting the header contents within the output stream
bool includeHeaders( std::string& code, const std::string& parentPath )
{
	using namespace std;	

	string::size_type s;
	while((s = code.find("#include", 0)) != string::npos)
	{
		string::size_type includeStart = s;
		string::size_type includeEnd = code.find('\n', s);
		if (includeEnd == string::npos) break; 
		string includeDirective = code.substr(includeStart, includeEnd - includeStart);
		code.erase(includeStart, includeEnd - includeStart);

		string headerFile;
		{
			// TODO: handle double quotes as well
			string::size_type openBracket = includeDirective.find('<', 0);
			string::size_type closeBracket = includeDirective.find('>',0); 
			if (openBracket == string::npos ||
				closeBracket == string::npos)
			{
				log("Failed to parse include directive " + includeDirective);
				return false;
			}
            headerFile = includeDirective.substr(openBracket + 1, closeBracket - openBracket - 1);
		}

		string headercode;
		if ( !loadFile(parentPath + headerFile, headercode) )
		{
			log("Failed to load header " + headerFile);
			return false;	
		}
		code.insert(includeStart, headercode);
	}

	return true;
}

bool parseShader( const std::string& file, std::string& contents )
{
	using namespace std;

	if (!loadFile( file, contents )) return false;

	string parentPath;
	string::size_type lastSlash = file.rfind('/', file.size() - 1);
	if (lastSlash != string::npos)
	{
		parentPath = file.substr(0, lastSlash + 1);
	}
	
	return includeHeaders(contents, parentPath);
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

