#include "renderer/renderer.h"
#include "renderer/loaders/magicaVoxel.h"
#include "mesh/meshLoader.h"
#include "mesh/mesh.h"
#include "voxelize/cpuVoxelizer.h"
#include "voxelize/gpuVoxelizer.h"
#include "camera/cameraController.h"

#define VOXELIZE_GPU 1

void Renderer::loadVoxFile(const std::string& file)
{
	std::vector<GLint> voxelMaterials;
	std::vector<float> materialData;
	MagicaVoxelLoader loader;

	if (!loader.load(file, 
					 voxelMaterials, 
					 materialData, 
					 m_glResources.m_volumeResolution))
	{
		return;
	}
							
	createVoxelDataTexture(m_glResources.m_volumeResolution, 
						   &voxelMaterials[0], 
						   materialData.size(),
						   &materialData[0]);
	m_camera.controller().setDistanceFromTarget(m_volumeBounds.size().length() * 0.5f);

	resetRender();
}

Imath::M44f computeMeshTransform(const Imath::Box3f& bounds, const Imath::V3i& voxelResolution)
{
	using namespace Imath;
	M44f meshTransform;

	// set mesh transform so that the mesh fits within the unit cube. This will
	// be changed later when we let the user manipulate the mesh transform and
	// the mesh/volume intersection.
	V3f voxelMargin = V3f(1.0f) / voxelResolution; // 1 voxel
	int majorAxis = bounds.majorAxis();
	float s = (1.0f - 2.0 * voxelMargin[majorAxis] ) / bounds.size()[majorAxis];
	V3f t = -bounds.min + voxelMargin / s;
	meshTransform.x[0][0] = s ; meshTransform.x[0][1] = 0 ; meshTransform.x[0][2] = 0 ; meshTransform.x[0][3] = t.x  * s ;
	meshTransform.x[1][0] = 0 ; meshTransform.x[1][1] = s ; meshTransform.x[1][2] = 0 ; meshTransform.x[1][3] = t.y  * s ;
	meshTransform.x[2][0] = 0 ; meshTransform.x[2][1] = 0 ; meshTransform.x[2][2] = s ; meshTransform.x[2][3] = t.z  * s ;
	meshTransform.x[3][0] = 0 ; meshTransform.x[3][1] = 0 ; meshTransform.x[3][2] = 0 ; meshTransform.x[3][3] = 1.0f	 ;

	return meshTransform;
}

void Renderer::loadMesh(const std::string& file)
{
	Imath::M44f meshTransform;

#if VOXELIZE_GPU
	Mesh* mesh = MeshLoader::loadFromOBJ(file.c_str());

	if (mesh == NULL) return;

	createVoxelDataTexture(Imath::V3i(64), NULL, 0, NULL);

	meshTransform = computeMeshTransform(mesh->bounds(), m_glResources.m_volumeResolution);
	GPUVoxelizer voxelizer(m_shaderPath, m_logger);
	voxelizer.voxelizeMesh(mesh, 
						   meshTransform, 
						   m_glResources.m_volumeResolution, 
						   GLResourceConfiguration::TEXTURE_UNIT_MATERIAL_OFFSET); 	

	delete(mesh);

#else 
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	MeshLoader::loadFromOBJ(file.c_str(), vertices, indices);
	Imath::Box3f bounds = computeBounds(&vertices[0], vertices.size() / 3);

	meshTransform = computeMeshTransform(bounds, m_glResources.m_volumeResolution);

	// FIXME: I must be having a mismatch in the way I upload the matrices to
	// GLSL -- this transpose should not be necessary if the above matrix is
	// valid for the GPU voxelization.
	meshTransform.transpose();

	Imath::V3f* verts = reinterpret_cast<Imath::V3f*>(&vertices[0]);
	for(size_t i = 0; i < vertices.size() / 3; ++i)
	{
		meshTransform.multVecMatrix(verts[i], verts[i]);
		// now transform the vertices from world space into voxel space, this would
		// be done by the vertex shader
		verts[i] *= m_glResources.m_volumeResolution;
	}

	const size_t numVoxels = (size_t)(m_glResources.m_volumeResolution.x * m_glResources.m_volumeResolution.y * m_glResources.m_volumeResolution.z);
	const size_t numTriangles = indices.size() / 3;
	GLint* materialOffsetTexels = (GLint*)malloc(numVoxels * sizeof(GLint));
	for(size_t i = 0; i < numVoxels; ++i) materialOffsetTexels[i] = -1;

	CPUVoxelizer::voxelizeMesh(verts, &indices[0], numTriangles, m_glResources.m_volumeResolution, materialOffsetTexels);

	createVoxelDataTexture(m_glResources.m_volumeResolution);
	glBindTexture(GL_TEXTURE_3D, m_glResources.m_materialOffsetTexture);
	glTexImage3D(GL_TEXTURE_3D,
				 0,
				 GL_R32UI,
				 m_glResources.m_volumeResolution.x,
				 m_glResources.m_volumeResolution.y,
				 m_glResources.m_volumeResolution.z,
				 0,
				 GL_RED,
				 GL_INT,
				 materialOffsetTexels);
	free(materialOffsetTexels);
#endif

	resetRender();
}


