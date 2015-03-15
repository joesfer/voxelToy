#version 430

#include <focalDistance/focalDistanceDevice.h>
#include <editVoxels/selectVoxelDevice.h>

uniform isampler3D  materialOffsetTexture;
uniform sampler1D   materialDataTexture;
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

uniform float		tonemappingGamma = 2.2;
uniform float		tonemappingExposure = 1;

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

float ISECT_EPSILON = 0.01;

vec3 directLighting(in int materialDataOffset, 
					in Basis wsHitBasis, 
					in vec3 wsWo, 
					inout ivec2 rngOffset)
{
	// for now we only sample the environment light. See TODO below.
	
	vec4 wsToLight_pdf;
	vec3 lightRadiance = sampleEnvironmentRadiance(wsHitBasis, rngOffset, wsToLight_pdf);
	if ( wsToLight_pdf.w < 1e-9 )
	{
		// don't bother with the visibility test, and early out.
		return vec3(0);
	}

	// Sample light with MIS
	
	// trace shadow ray to determine whether the radiance reaches the sampled
	// point.
	vec3 vsShadowHitPos;
	bool hitGround;
	if( traverse(wsHitBasis.position + ISECT_EPSILON * wsToLight_pdf.xyz, 
	//if( traverse(wsHitBasis.position + ISECT_EPSILON * wsHitBasis.normal, 
				 wsToLight_pdf.xyz, vsShadowHitPos, hitGround) )
	{
		// light is not visible. Evaluate possible emission from hit blocker.
		
		// ground is always considered non-emisive. 
		if (hitGround) return vec3(0);

		// TODO: this sampling scheme is inefficient for glowing materials
		// because we're only really sampling the environment light. 
		//
		// We should, on top of this, randomly sample the emissive
		// blocks (which are considered lights), by keeping them in a list of
		// lights or similar. Currently we just happen to evaluate the radiance
		// in the sampled direction in case the light we were actually trying to
		// sample is blocked.
		//
		// In the case of constant colors this will resort to a uniform sample, 
		// this is probably not too bad, but environment maps will be heavily 
		// biased towards the bright spots and thus emissive objects may be
		// heavily undersampled.
		//
		int materialDataOffset = texelFetch(materialOffsetTexture,
														   ivec3(vsShadowHitPos.x, 
															     vsShadowHitPos.y, 
															     vsShadowHitPos.z), 0).r;
		lightRadiance = emissionBSDF(materialDataOffset);

		// Adjust PDF factor now we're not hitting the environment but a block.
		// We'll disregard the block's material, pretend it's Lambertian and 
		// thus consider all directions equally probable in the hemisphere 
		// (uniform hemisphere sampling)
		wsToLight_pdf.w = 1.0 / (2.0 * PI);
	}

	// Apply MIS weight for the sampled direction. PBRT2 page 748/749);
	
	// transform sampled directions to local space, which we need to evaluate
	// the BSDF
	vec3 lsWo = worldToLocal(wsWo, wsHitBasis);
	vec3 lsWi = worldToLocal(wsToLight_pdf.xyz, wsHitBasis);
	vec4 bsdfF_pdf = evaluateMaterialBSDF(materialDataOffset, lsWo, lsWi);

	float misWeight = powerHeuristic(wsToLight_pdf.w, bsdfF_pdf.w);
	return bsdfF_pdf.xyz * lightRadiance * abs(dot(wsToLight_pdf.xyz, wsHitBasis.normal)) * misWeight / wsToLight_pdf.w;

	// Note we do the second half, BSDF sampling, on the main integrator loop, 
	// by sampling the BSDF for the next vertex path and calculating the MIS 
	// weight when the ray misses and thus we have implicit visibility with the 
	// environment light. 
}

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
		outColor = vec4(getBackgroundColor(wsRayDir), 0);
		return;
	}

	float rayLength = aabbIsectDist;

	// push the intersection slightly inside the hit voxel so that when we cast 
	// to a voxel index we don't mistakenly take an adjacent voxel. This is 
	// important to ensure the traversal starts inside of the volume bounds.
	vec3 halfVoxellDist = sign(wsRayDir) * 0.5 / voxelResolution; 
	vec3 wsRayEntryPoint = wsRayOrigin + rayLength * wsRayDir + halfVoxellDist;

	vec3 vsHitPos;

	// Cast primary ray
	vec3 throughput = vec3(1.0);
	if ( !traverse(wsRayEntryPoint, wsRayDir, vsHitPos, hitGround) )
	{
		outColor = vec4(getBackgroundColor(wsRayDir), 0);
		return;
	}

	vec3 radiance = vec3(0.0);

	int bounces = 0; 

	// PBRT2 section 16.3
	while(bounces < pathtracerMaxNumBounces)
	{
		// convert hit position from voxel space to world space. We also use the
		// calculations to generate a world-space basis 
		// <wsHitTangent, wsHitNormal, wsHitBinormal> which we'll use for the 
		// local<->world space conversions.
		Basis wsHitBasis;
		voxelSpaceToWorldSpace(vsHitPos, 
							   wsRayOrigin, wsRayDir,
							   wsHitBasis);
		
		int materialDataOffset = texelFetch(materialOffsetTexture,
														   ivec3(vsHitPos.x, 
															     vsHitPos.y, 
															     vsHitPos.z), 0).r;

		if ( ivec3(vsHitPos) == SelectVoxelData.index.xyz )
		{
			// Draw selected voxel as red
			radiance += vec3(1,0,0); 
			break;
		}

		// the salient direction for the incoming light, bounced back though the 
		// current ray.
		vec3 wsWo = -wsRayDir; 
		vec3 lsWo = worldToLocal(wsWo, wsHitBasis);

		// add emission from surface
		if ( bounces == 0 )
		{
			vec3 Le = emissionBSDF(materialDataOffset);
			radiance += throughput * Le;
		}

		// Sample illumination from lights to find path contribution
		radiance += throughput * directLighting(materialDataOffset, 
												wsHitBasis, 
												wsWo, 
												rngOffset);
		
		// Sample the BSDF to get the new path direction
		vec4 bsdfF_pdf;
		vec3 lsWi = sampleMaterialBSDF(materialDataOffset,
									   lsWo, 
									   rngOffset, 
									   bsdfF_pdf); 
		// Wireframe overlay
		if (wireframeOpacity > 0)
		{
			vec3 vsVoxelCenter = (wsHitBasis.position - volumeBoundsMin) / (volumeBoundsMax - volumeBoundsMin) * voxelResolution;
			vec3 uvw = vsHitPos - vsVoxelCenter;
			vec2 uv = abs(vec2(dot(wsHitBasis.normal.yzx, uvw), dot( wsHitBasis.normal.zxy, uvw)));
			float wireframe = step(wireframeThickness, uv.x) * step(uv.x, 1-wireframeThickness) *
							  step(wireframeThickness, uv.y) * step(uv.y, 1-wireframeThickness);

			wireframe = (1-wireframeOpacity) + wireframeOpacity * wireframe;	
			bsdfF_pdf.xyz *= vec3(wireframe);
		}


		vec3 wsWi = localToWorld(lsWi, wsHitBasis);
		
		// update throughput
		throughput *= (bsdfF_pdf.xyz * abs(dot(wsWi, wsHitBasis.normal)) / bsdfF_pdf.w);

		wsRayOrigin = wsHitBasis.position;
		wsRayDir = wsWi;

		// find new vertex of path 
		if ( !traverse(wsRayOrigin + wsRayDir * ISECT_EPSILON, wsRayDir, vsHitPos, hitGround) )
		//if ( !traverse(wsRayOrigin + wsHitBasis.normal * ISECT_EPSILON, wsRayDir, vsHitPos, hitGround) )
		{
			// the ray missed the scene. Handle the environment light here.
			vec4 lightL_pdf = evaluateEnvironmentRadiance(wsRayDir);
			float misWeight = powerHeuristic(bsdfF_pdf.w, lightL_pdf.w);
			radiance += throughput * lightL_pdf.xyz  * misWeight;
			break;
		}

		bounces++;
	}

	radiance = pow(radiance * tonemappingExposure, vec3(1.0 / tonemappingGamma));
	outColor = vec4(radiance,1);
}


