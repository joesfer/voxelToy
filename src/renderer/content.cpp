#include "renderer/content.h"

namespace VoxelTools
{

void addVoxelSphere( const Imath::V3f sphereCenter, float sphereRadius, 
                     const Imath::V3i volumeResolution,
					 const Imath::Box3f volumeBounds,
					 GLubyte* occupancyTexels, RGB* colorTexels)

{
	using namespace Imath;

    V3f voxelSize = volumeBounds.size() / volumeResolution;

    V3i voxelSphereBoundsMin = ((sphereCenter - V3f(sphereRadius)) - volumeBounds.min) / volumeBounds.size() * volumeResolution;
    V3i voxelSphereBoundsMax = ((sphereCenter + V3f(sphereRadius)) - volumeBounds.min) / volumeBounds.size() * volumeResolution;

#if !NOISE
	RGB constantColor(GLubyte((float)rand() / RAND_MAX * 255),
					  GLubyte((float)rand() / RAND_MAX * 255),
					  GLubyte((float)rand() / RAND_MAX * 255));
#else
    float octaves = 8;
    float persistence = 0.5f;
    float scale = 10.0f;
    float noiseScale = 0.5f;
#endif

    for( size_t k = std::max(0, voxelSphereBoundsMin.z); 
          k < (size_t)std::max(0,std::min(volumeResolution.z, voxelSphereBoundsMax.z));
		  ++k )
    {
		for( size_t j = std::max(0, voxelSphereBoundsMin.y); 
              j < (size_t)std::max(0,std::min(volumeResolution.y, voxelSphereBoundsMax.y));
			  ++j )
        {
			for( size_t i = std::max(0, voxelSphereBoundsMin.x); 
                  i < (size_t)std::max(0,std::min(volumeResolution.x, voxelSphereBoundsMax.x));
				  ++i )
            {
                size_t offset = k * volumeResolution.x * volumeResolution.y + j * volumeResolution.x + i;
                V3f voxelCenter = V3f(i + 0.5f, j + 0.5f, k + 0.5f) * voxelSize + volumeBounds.min;

                float distance = (voxelCenter - sphereCenter).length() - sphereRadius;

                const bool hasVoxel = distance <= 0;

				if (!hasVoxel) continue;

#if NOISE
				float noiseR = octave_noise_3d(octaves,
                                                persistence,
                                                scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;
				float noiseG = octave_noise_3d(octaves / 2,
											  persistence,
											  scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;
				float noiseB = octave_noise_3d(octaves / 4,
											  persistence,
											  scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;

                colorTexels[offset] = RGB( GLubyte(noiseR * (float)i / volumeResolution.x * 255),
                                           GLubyte(noiseG * (float)j / volumeResolution.y * 255),
                                           GLubyte(noiseB * (float)k / volumeResolution.z * 255));
#else 
				colorTexels[offset] = constantColor;
#endif
				occupancyTexels[offset] = 255;
            }
        }
    }
}

void addPlane( const Imath::V3f& normal, const Imath::V3f& p,
			   const Imath::V3i volumeResolution,
			   const Imath::Box3f volumeBounds,
			   GLubyte* occupancyTexels, RGB* colorTexels )
{
	using namespace Imath;

    V3f voxelSize = volumeBounds.size() / volumeResolution;

#if !NOISE
	RGB constantColor(192, 192, 192);
#else
    float octaves = 8;
    float persistence = 0.5f;
    float scale = 10.0f;
    float noiseScale = 0.5f;
#endif
	float d = -normal.dot(p);

    for( size_t k = 0; k < (size_t)volumeResolution.z; ++k )
    {
        for( size_t j = 0; j < (size_t)volumeResolution.y; ++j )
        {
            for( size_t i = 0; i < (size_t)volumeResolution.x; ++i )
            {
                size_t offset = k * volumeResolution.x * volumeResolution.y + j * volumeResolution.x + i;
                V3f voxelCenter = V3f(i + 0.5f, j + 0.5f, k + 0.5f) * voxelSize + volumeBounds.min;

                float distance = abs(normal.dot(voxelCenter) + d);

                const bool hasVoxel = distance <= voxelSize.x;

				if (!hasVoxel) continue;

#if NOISE
				float noiseR = octave_noise_3d(octaves,
                                                persistence,
                                                scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;
				float noiseG = octave_noise_3d(octaves / 2,
											  persistence,
											  scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;
				float noiseB = octave_noise_3d(octaves / 4,
											  persistence,
											  scale,
                                              (float)i/volumeResolution.x,
                                              (float)j/volumeResolution.y,
                                              (float)k/volumeResolution.z) * noiseScale + 1.0f;

                colorTexels[offset] = RGB( GLubyte(noiseR * (float)i / volumeResolution.x * 255),
                                           GLubyte(noiseG * (float)j / volumeResolution.y * 255),
                                           GLubyte(noiseB * (float)k / volumeResolution.z * 255));
#else 
				colorTexels[offset] = constantColor;
#endif
				occupancyTexels[offset] = 255;
            }
        }
    }
}

} // namespace VoxelTools
