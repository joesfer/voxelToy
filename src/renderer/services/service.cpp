#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/services/service.h"

RendererService::~RendererService()
{
	glDeleteProgram(m_program);
}

void RendererService::runVertexShader() 
{
	// disable rasterization and issue a single vertex draw call. This is used
	// with vertex shaders performing computations which are not meant to be
	// drawn directly.
	glEnable(GL_RASTERIZER_DISCARD);
	glBegin(GL_POINTS);
		glVertex3f(0,0,0);
	glEnd();
	glDisable(GL_RASTERIZER_DISCARD);
}


