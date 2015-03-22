#version 430

#include <focalDistance/focalDistanceDevice.h>
#include <editVoxels/selectVoxelDevice.h>

uniform isampler3D  materialOffsetTexture;
uniform sampler1D   materialDataTexture;
uniform sampler2D   noiseTexture;
uniform ivec3       voxelResolution;
uniform vec3        volumeBoundsMin;
uniform vec3        volumeBoundsMax;
uniform vec3        wsVoxelSize; // (boundsMax-boundsMin)/resolution

uniform vec4        viewport;
uniform float       cameraNear;
uniform float       cameraFar;
uniform mat4        cameraProj;
uniform mat4        cameraInverseProj;
uniform mat4        cameraInverseModelView;
uniform float       cameraFocalLength;
uniform float       cameraLensRadius;
uniform vec2        cameraFilmSize;
uniform int         cameraLensModel;

uniform vec3        backgroundColorTop = vec3(153.0 / 255, 187.0 / 255, 201.0 / 255) * 2;
uniform vec3        backgroundColorBottom = vec3(77.0 / 255, 64.0 / 255, 50.0 / 255);
uniform vec3        groundColor = vec3(0.5, 0.5, 0.5);
uniform int         backgroundUseImage;
uniform sampler2D   backgroundTexture;
uniform sampler2D   backgroundCDFUTexture;
uniform sampler1D   backgroundCDFVTexture;
uniform float	    backgroundIntegral;
uniform float	    backgroundRotationRadians;

uniform int         sampleCount;
uniform int			pathtracerMaxNumBounces;

uniform float		wireframeOpacity = 0;
uniform float		wireframeThickness = 0.01;

uniform vec3		lightDirection = vec3(1, -1, -1);
uniform float		ambientLight = 0.5;
out vec4 outColor;

#include <shared/constants.h>
#include <shared/aabb.h>
#include <shared/coordinates.h>
#include <shared/dda.h>
#include <shared/sampling.h>
#include <shared/random.h>
#include <shared/generateRay.h>
#include <bsdf/lambertian.h>
#include <bsdf/microfacet.h>
#include <bsdf/bsdf.h>
#include <materials/matte.h>
#include <materials/metal.h>
#include <materials/plastic.h>
#include <materials/materials.h>
#include <shared/lights.h>


void main()
{
	ivec2 rngOffset = randomNumberGeneratorOffset(ivec4(gl_FragCoord), sampleCount);

	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay(gl_FragCoord.xyz, rngOffset, wsRayOrigin, wsRayDir);

	// test intersection with bounds to trivially discard rays before entering
	// traversal.
	float aabbIsectDist = rayAABBIntersection(wsRayOrigin, wsRayDir,
											  volumeBoundsMin, volumeBoundsMax); 

	bool hitGround;
	if (aabbIsectDist < 0)
	{
		// we're not even hitting the volume's bounding box. Early out.
		outColor = vec4(getBackgroundColor(wsRayDir),1);
		return;
	}

	vec3 wsRayEntryPoint = wsRayOrigin + aabbIsectDist * wsRayDir;
	vec3 vsHitPos;

	// Cast primary ray
	if ( !traverse(wsRayEntryPoint, wsRayDir, vsHitPos, hitGround) )
	{
		outColor = vec4(getBackgroundColor(wsRayDir),1);
		return;
	}

	// convert hit position from voxel space to world space. We also use the
	// calculations to generate a world-space basis 
	// <wsHitTangent, wsHitNormal, wsHitBinormal> which we'll use for the 
	// local<->world space conversions.
	Basis wsHitBasis;
	voxelSpaceToWorldSpace(vsHitPos, 
						   wsRayOrigin, wsRayDir,
						   wsHitBasis);
	
	//vec3 albedo = hitGround ? 
	//			groundColor :
	//			texelFetch(voxelColorTexture,
	//						 ivec3(vsHitPos.x, vsHitPos.y, vsHitPos.z), 0).xyz;

	if ( ivec3(vsHitPos) == SelectVoxelData.index.xyz )
	{
		// Draw selected voxel as red
		outColor = vec4(1,0,0,1);
		return;
	}

	// Wireframe overlay
	vec3 albedo = vec3(1,1,1); // TODO -- extract albedo from material parameters
	if (wireframeOpacity > 0)
	{
		vec3 vsVoxelCenter = (wsHitBasis.position - volumeBoundsMin) / (volumeBoundsMax - volumeBoundsMin) * voxelResolution;
		vec3 uvw = vsHitPos - vsVoxelCenter;
		vec2 uv = abs(vec2(dot(wsHitBasis.normal.yzx, uvw), dot( wsHitBasis.normal.zxy, uvw)));
		float wireframe = step(wireframeThickness, uv.x) * step(uv.x, 1-wireframeThickness) *
						  step(wireframeThickness, uv.y) * step(uv.y, 1-wireframeThickness);

		wireframe = (1-wireframeOpacity) + wireframeOpacity * wireframe;	
		albedo *= vec3(wireframe);
	}


	// dot normal lighting
	float lighting = max(0, dot(-wsRayDir, wsHitBasis.normal));
	lighting = sqrt(lighting);

	// trace shadow ray
	if ( traverse(wsHitBasis.position, -lightDirection, vsHitPos, hitGround) )
	{
		lighting *= ambientLight;
	}

	vec3 radiance = albedo * lighting; 
	
	outColor = vec4(radiance,1);
}


