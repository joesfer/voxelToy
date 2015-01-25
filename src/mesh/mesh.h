#pragma once

#include <GL/gl.h>
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImathVec.h>

class Mesh
{
public:

	void draw() const;
	~Mesh();

	Imath::Box3f bounds() const { return m_bounds; }

private:
	Mesh( const float* vertices, size_t numVertices,
		  const unsigned int* indices, size_t numIndices);

	friend class MeshLoader;

private:
	GLuint m_vao; // Vertex array object
	size_t m_numIndices;

	// Resource dependencies. Not used directly, but will dispose associated
	// data once geometry is deleted
	GLuint m_vbo; // vertex buffer object
	GLuint m_ibo; // Index buffer object

	Imath::Box3f m_bounds;
};

Imath::Box3f computeBounds(const float* vertices, size_t numVertices);
