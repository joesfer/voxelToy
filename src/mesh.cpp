#include <GL/glew.h>
#include "mesh.h"

#define ATTRIBUTE_LAYOUT_INDEX_POSITION 0

Mesh::Mesh( const float* vertices, size_t numVertices,
			const unsigned int* indices, size_t numIndices)
{
	m_numIndices = numIndices;

	// declare and enable vertex array object to record the vbo/ibo settings
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	// declare vertex data
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, 
				 3 * numVertices * sizeof(float),
				 vertices,
				 GL_STATIC_DRAW);

	glEnableVertexAttribArray(ATTRIBUTE_LAYOUT_INDEX_POSITION);
	glVertexAttribPointer(ATTRIBUTE_LAYOUT_INDEX_POSITION, 
						  3, 
						  GL_FLOAT, 
						  GL_FALSE, 
                          3 * sizeof(float),
						  0);
	
	// declare index data
	glGenBuffers(1, &m_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
				 numIndices * sizeof(unsigned int),
				 indices,
				 GL_STATIC_DRAW);

	// restore GL state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// compute bounds
	m_bounds.makeEmpty();
	Imath::V3f vertex;
	for(size_t v=0; v < numVertices; ++v)
	{
		vertex.x = vertices[3 * v + 0];
		vertex.y = vertices[3 * v + 1];
		vertex.z = vertices[3 * v + 2];
		m_bounds.extendBy(vertex);
	}
}

Mesh::~Mesh()
{
	if ( glIsVertexArray(m_ibo)) glDeleteVertexArrays(1, &m_vao);
	if ( glIsBuffer(m_vbo))      glDeleteBuffers(1, &m_vbo);
	if ( glIsBuffer(m_ibo))      glDeleteBuffers(1, &m_ibo);
}

void Mesh::draw() const
{
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
}
