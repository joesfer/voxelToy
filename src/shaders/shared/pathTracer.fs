#version 430

#include <../focalDistance/focalDistanceDevice.h>
#include <../editVoxels/selectVoxelDevice.h>

uniform sampler3D   occupancyTexture;
uniform sampler3D   voxelColorTexture;
uniform sampler2D   noiseTexture;
uniform ivec3       voxelResolution;
uniform vec3        volumeBoundsMin;
uniform vec3        volumeBoundsMax;

uniform vec4        viewport;
uniform float       cameraNear;
uniform float       cameraFar;
uniform mat4        cameraProj;
uniform mat4        cameraInverseProj;
uniform mat4        cameraInverseModelView;
uniform float       cameraFocalLength;
uniform float       cameraLensRadius;
uniform vec2        cameraFilmSize;

uniform vec4        backgroundColorSky = vec4(153.0 / 255, 187.0 / 255, 201.0 / 255, 1) * 2;
uniform vec4        backgroundColorGround = vec4(77.0 / 255, 64.0 / 255, 50.0 / 255, 1);
uniform vec3        groundColor = vec3(0.5, 0.5, 0.5);

uniform int         sampleCount;
uniform int         enableDOF;
uniform int			pathtracerMaxPathLength;

uniform float		wireframeOpacity = 0;
uniform float		wireframeThickness = 0.01;

out vec4 outColor;

#include <aabb.h>
#include <coordinates.h>
#include <dda.h>
#include <sampling.h>
#include <random.h>
#include <generateRay.h>
#include <bsdf.h>
#include <lights.h>

float ISECT_EPSILON = 0.01;

void generateRay(inout ivec2 rngOffset, out vec3 wsRayOrigin, out vec3 wsRayDir)
{
	if (enableDOF != 0)
	{
		generateRay_ThinLens(gl_FragCoord.xyz, rngOffset, wsRayOrigin, wsRayDir);
	}
	else
	{
		generateRay_Pinhole(gl_FragCoord.xyz, wsRayOrigin, wsRayDir);
	}

}

vec3 directLighting(in vec3 albedo, 
					in Basis wsHitBasis, 
					in vec3 wsWo, 
					inout ivec2 rngOffset)
{
	// for now we only consider the environment light
	
	vec4 wsToLight_pdf;
	vec3 lightRadiance = sampleEnvironmentRadiance(wsHitBasis, rngOffset, wsToLight_pdf);
	if ( wsToLight_pdf.w < 1e-5 )
	{
		// don't bother with the visibility test, and early out.
		return vec3(0);
	}

	// trace shadow ray to determine whether the radiance reaches the sampled
	// point.
	vec3 vsShadowHitPos;
	bool hitGround;
	if( traverse(wsHitBasis.position + ISECT_EPSILON * wsToLight_pdf.xyz, 
				 wsToLight_pdf.xyz, vsShadowHitPos, hitGround) )
	{
		// light is not visible. No light contribution.
		return vec3(0);
	}

	// Apply MIS weight for the sampled direction. PBRT2 page 748/749.
	//
	// TODO: in this particular case (Lambertians + infinite area lights only) I
	// don't think MIS is actually making any difference since both distributions
	// are pretty much uniform.
	
	// transform sampled directions to local space, which we need to evaluate
	// the BSDF
	vec3 lsWo = worldToLocal(wsWo, wsHitBasis);
	vec3 lsWi = worldToLocal(wsToLight_pdf.xyz, wsHitBasis);
	vec4 bsdfF_pdf = evaluateBsdf(albedo, lsWo, lsWi);

	float misWeight = powerHeuristic(wsToLight_pdf.w, bsdfF_pdf.w);

	return bsdfF_pdf.xyz * lightRadiance * abs(dot(wsToLight_pdf.xyz, wsHitBasis.normal)) * misWeight / max(1e-5, wsToLight_pdf.w);
}

void main()
{
	ivec2 rngOffset = randomNumberGeneratorOffset(ivec4(gl_FragCoord), sampleCount);

	vec3 wsRayOrigin;
	vec3 wsRayDir;
	generateRay(rngOffset, wsRayOrigin, wsRayDir);

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

	float rayLength = aabbIsectDist;

	// push the intersection slightly inside the hit voxel so that when we cast 
	// to a voxel index we don't mistakenly take an adjacent voxel. This is 
	// important to ensure the traversal starts inside of the volume bounds.
	vec3 halfVoxellDist = 0*sign(wsRayDir) * 0.5 / voxelResolution; 
	vec3 wsRayEntryPoint = wsRayOrigin + rayLength * wsRayDir + halfVoxellDist;

	vec3 vsHitPos;

	// Cast primary ray
	vec3 throughput = vec3(1.0);
	if ( !traverse(wsRayEntryPoint, wsRayDir, vsHitPos, hitGround) )
	{
		outColor = vec4(getBackgroundColor(wsRayDir),1);
		return;
	}

	vec3 radiance = vec3(0.0);

	int pathLength = 1; // we've traced the primary ray already

	// PBRT2 section 16.3
	while(pathLength <= pathtracerMaxPathLength)
	{
		// convert hit position from voxel space to world space. We also use the
		// calculations to generate a world-space basis 
		// <wsHitTangent, wsHitNormal, wsHitBinormal> which we'll use for the 
		// local<->world space conversions.
		Basis wsHitBasis;
		voxelSpaceToWorldSpace(vsHitPos, 
							   wsRayOrigin, wsRayDir,
							   wsHitBasis);
		
		vec3 albedo = hitGround ? 
					groundColor :
					texelFetch(voxelColorTexture,
							     ivec3(vsHitPos.x, vsHitPos.y, vsHitPos.z), 0).xyz;

		// Wireframe overlay
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

		if ( ivec3(vsHitPos) == SelectVoxelData.index.xyz )
		{
			// Draw selected voxel as red
			albedo = vec3(1,0,0); 
			radiance += albedo; 
			break;
		}

		// the salient direction for the incoming light, bounced back though the 
		// current ray.
		vec3 wsWo = -wsRayDir; 
		vec3 lsWo = worldToLocal(wsWo, wsHitBasis);

		// TODO emission

		// Sample illumination from lights to find path contribution
		radiance += throughput * directLighting(albedo, wsHitBasis, wsWo, rngOffset);
		
		// Sample the BSDF to get the new path direction
		vec4 bsdfF_pdf;
		vec3 lsWi = sampleBSDF(albedo, lsWo, rngOffset, bsdfF_pdf); 
		vec3 wsWi = localToWorld(lsWi, wsHitBasis);
		
		// update throughput
		throughput *= (bsdfF_pdf.xyz * abs(dot(wsWi, wsHitBasis.normal)) / bsdfF_pdf.w);

		wsRayOrigin = wsHitBasis.position;
		wsRayDir = wsWi;

		// find new vertex of path 
		if ( !traverse(wsRayOrigin + wsRayDir * ISECT_EPSILON, wsRayDir, vsHitPos, hitGround) )
		{
			// the ray missed the scene. Handle the environment light here.
			vec4 environtmentRadiance_Pdf = evaluateEnvironmentRadiance(wsRayDir);
			float misWeight = powerHeuristic(bsdfF_pdf.w, environtmentRadiance_Pdf.w);
			radiance += throughput * environtmentRadiance_Pdf.xyz  * misWeight;
			break;
		}

		pathLength++;
	}

	outColor = vec4(radiance,1);
}


