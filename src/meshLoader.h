#pragma once

#include <vector>

// forward declaration
class Mesh;

class MeshLoader
{
public:
	static Mesh* loadFromOBJ(const char* file);
	static void loadFromOBJ(const char* file,
							std::vector<float>& vertices,
							std::vector<unsigned int>& indices);
};
