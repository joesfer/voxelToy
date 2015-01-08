#pragma once

// forward declaration
class Mesh;

class MeshLoader
{
public:
	static Mesh* loadFromOBJ(const char*);
};
