#include "meshLoader.h"
#include "mesh.h"
#include <vector>
#include <iostream>
#include <memory.h>

// use syoyo's tinyObj loader to handle OBJ
// https://github.com/syoyo/tinyobjloader
#include "thirdParty/tinyobjloader/tiny_obj_loader.h"

/*static*/ void MeshLoader::loadFromOBJ(const char* filePath,
										std::vector<float>& vertices,
										std::vector<unsigned int>& indices)
{
	using namespace std;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;

	string err = tinyobj::LoadObj(shapes, materials, filePath);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;	
	}

	{
		size_t totalVertexComponents = 0;
		size_t totalIndices = 0;
		for(size_t s = 0; s < shapes.size(); ++s)
		{
			const tinyobj::shape_t& shape = shapes[s];
			totalIndices += shape.mesh.indices.size();
			totalVertexComponents += shape.mesh.positions.size();
		}

		indices.resize(totalIndices);
		vertices.resize(totalVertexComponents);
	}

	{ // merge all meshes
		size_t vertexOffset = 0;
		size_t indexOffset = 0;
		for(size_t s = 0; s < shapes.size(); ++s)
		{
			const tinyobj::shape_t& shape = shapes[s];
			const size_t numVertices = shape.mesh.positions.size() / 3;
			const size_t numIndices = shape.mesh.indices.size();
			
            // copy vertex data
            memcpy(&vertices[3 * vertexOffset],
                    &shape.mesh.positions[0],
                    numVertices * 3 * sizeof(float));
			
			// reindex triangles
			for(size_t i = 0; i < numIndices; ++i)
			{
				indices[i + indexOffset] = shape.mesh.indices[i] + vertexOffset;
			}
		
			//
			vertexOffset += numVertices;
			indexOffset += numIndices;
		}
	}
}

/*static*/ Mesh* MeshLoader::loadFromOBJ(const char* filePath)
{
	using namespace std;
	vector<float> vertices;
	vector<unsigned int> indices;
	loadFromOBJ(filePath, vertices, indices);
	return new Mesh(&vertices[0], vertices.size() / 3, &indices[0], indices.size());
}


