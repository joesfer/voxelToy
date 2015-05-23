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

uniform isampler1D  emissiveVoxelIndicesTexture; // for light sampling
uniform float		emissiveVoxelsTotalPower;

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

vec3 directLighting(in int materialDataOffset, 
					in Basis wsHitBasis, 
					in vec3 wsWo, 
					inout ivec2 rngOffset)
{
	vec4 wsToLight_pdf = vec4(0);
	vec3 lightRadiance;

	// We have stored every emissive voxel (that is, each voxel which assigned 
	// material contains non-zero emision) into an array. These, along with the
	// environment map make up for all the lights in the scene. Thus we'll
	// sample unformly amongst N+1 positions (the +1 being the environment
	// light)
	vec4 u = rand(rngOffset);
	const int numEmissiveVoxels = textureSize(emissiveVoxelIndicesTexture, 0);  
	const int numLights = numEmissiveVoxels + 1;

	const bool samplingEmissiveVoxel = u.x >= 1.0 - (float(numEmissiveVoxels) / numLights);

	ivec3 vsEmissiveVoxelPos = ivec3(-1);
	if (samplingEmissiveVoxel)
	{
		int voxelIndex = int(u.x * numEmissiveVoxels);

		// sample light from an emissive voxel
		int emissiveVoxelIndex = texelFetch(emissiveVoxelIndicesTexture, voxelIndex, 0);
		vsEmissiveVoxelPos = voxelIndexToVoxelPos(emissiveVoxelIndex, voxelResolution);
		int emissiveVoxelMaterialDataOffset = texelFetch(materialOffsetTexture, vsEmissiveVoxelPos, 0).r;
		lightRadiance = emissionBSDF(emissiveVoxelMaterialDataOffset); 

		vec3 wsEmissiveVoxelPos = (vec3(vsEmissiveVoxelPos)/voxelResolution) * (volumeBoundsMax-volumeBoundsMin) + volumeBoundsMin;
		// jitter hit within voxel (as an approximation of jittering on the hit surface)
		wsEmissiveVoxelPos += u.yzw * wsVoxelSize; 

		vec3 wsToLight = wsEmissiveVoxelPos - wsHitBasis.position; 
		const float wsR = length(wsToLight);
		wsToLight_pdf.xyz = wsToLight / wsR; 

		// Need to obtain the normal of the hit on the light. This is a bit
		// wasteful because we've already calculated wsEmissivePosition (which
		// we in turn needed to work out the direction towards the light in the
		// first place, wsToLight_pdf.xyz.
		Basis wsLightHitBasis;
		voxelSpaceToWorldSpace(vsEmissiveVoxelPos, 
							   wsHitBasis.position, 
							   wsToLight_pdf.xyz,
							   wsLightHitBasis);


		// Calculate the area PDF, and then apply the jacobian to express that
		// same PDF in terms of solid angle (which is what we're integrating) 
		//
		// cubic voxels, all sides are the same length
		const float voxelArea = 6.0 * wsVoxelSize.x * wsVoxelSize.y; // TODO: precompute
		const float jacobian = (wsR*wsR) / abs(dot(-wsToLight_pdf.xyz, wsLightHitBasis.normal));
		wsToLight_pdf.w = jacobian / voxelArea; 
	}
	else
	{
		// sample from environment
		lightRadiance = sampleEnvironmentRadiance(wsHitBasis, u.yz, wsToLight_pdf);
	}

	// Since we're sampling only one light at a time, we need to multiply the
	// result by the total number of lights so that the expected value will be
	// that of the sum of lights. PBRT2 p.746.
	lightRadiance *= numLights;

	// Sample light with MIS
	vec3 vsShadowRayHitPos;
	bool hitGround;

	if (luminance(lightRadiance) < 1e-5f) 
	{
		// don't bother with visibility test
		return vec3(0);
	}

	// trace shadow ray to determine whether the radiance reaches the sampled
	// point.
	const bool shadowRayHitSomething = traverse(wsHitBasis.position, 
											    wsToLight_pdf.xyz, 
											    vsShadowRayHitPos, 
											    hitGround);
	if(samplingEmissiveVoxel)
	{
		// when sampling an emissive voxel, we want the shadow ray not to miss
		// and to hit the sampled voxel.
		if (ivec3(vsShadowRayHitPos) != vsEmissiveVoxelPos) 
		{
			// emissive voxel is not visible .
			return vec3(0);
		}
	}
	else
	{
		// for the environment light we want the shadow ray not to hit anything
		// in the scene.
		if (shadowRayHitSomething)
		{
			// environment light is not visible. 
			return vec3(0);
		}
	}

	// Apply MIS weight for the sampled direction. PBRT2 page 748/749
	
	// transform sampled directions to local space, which we need to evaluate
	// the BSDF
	vec3 lsWo = worldToLocal(wsWo, wsHitBasis);
	vec3 lsWi = worldToLocal(wsToLight_pdf.xyz, wsHitBasis);
	vec4 bsdfF_pdf = evaluateMaterialBSDF(materialDataOffset, lsWo, lsWi);

	const float misWeight = powerHeuristic(wsToLight_pdf.w, bsdfF_pdf.w);
	return bsdfF_pdf.xyz * lightRadiance * abs(dot(wsToLight_pdf.xyz, wsHitBasis.normal)) * misWeight / wsToLight_pdf.w;

	// Note we do the second half, BSDF sampling, on the main integrator loop, 
	// by sampling the BSDF for the next vertex path and calculating the MIS 
	// weight when the ray misses and thus we have implicit visibility with the 
	// environment light. 
}

void main()
{
	ivec2 rngOffset = randomNumberGeneratorOffset(ivec4(gl_FragCoord), sampleCount);
	vec3 radiance = vec3(0.0);

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
		radiance = getBackgroundColor(wsRayDir);
		vec3 tonemappedRadiance = pow(radiance * tonemappingExposure, vec3(1.0 / tonemappingGamma));
		outColor = vec4(tonemappedRadiance,1);
		return;
	}

	vec3 wsRayEntryPoint = wsRayOrigin + aabbIsectDist * wsRayDir;

	vec3 vsHitPos;

	// Cast primary ray
	vec3 throughput = vec3(1.0);
	if ( !traverse(wsRayEntryPoint, wsRayDir, vsHitPos, hitGround) )
	{
		radiance = getBackgroundColor(wsRayDir);
		vec3 tonemappedRadiance = pow(radiance * tonemappingExposure, vec3(1.0 / tonemappingGamma));
		outColor = vec4(tonemappedRadiance,1);
		return;
	}


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
		
		ivec3 iVsHitPos = ivec3(vsHitPos);
		int materialDataOffset = texelFetch(materialOffsetTexture, iVsHitPos, 0).r;

		if ( iVsHitPos == SelectVoxelData.index.xyz )
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
	
		// Wireframe overlay
		if (wireframeOpacity > 0)
		{
			vec3 vsVoxelCenter = (wsHitBasis.position - volumeBoundsMin) / (volumeBoundsMax - volumeBoundsMin) * voxelResolution;
			vec3 uvw = vsHitPos - vsVoxelCenter;
			vec2 uv = abs(vec2(dot(wsHitBasis.normal.yzx, uvw), dot( wsHitBasis.normal.zxy, uvw)));
			float wireframe = step(wireframeThickness, uv.x) * step(uv.x, 1-wireframeThickness) *
							  step(wireframeThickness, uv.y) * step(uv.y, 1-wireframeThickness);

			wireframe = (1-wireframeOpacity) + wireframeOpacity * wireframe;	
			throughput *= wireframe;
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

		vec3 wsWi = localToWorld(lsWi, wsHitBasis);
		
		// update throughput
		throughput *= (bsdfF_pdf.xyz * abs(dot(wsWi, wsHitBasis.normal)) / bsdfF_pdf.w);

		wsRayOrigin = wsHitBasis.position;
		wsRayDir = wsWi;

		// find new vertex of path 
		if ( !traverse(wsRayOrigin, wsRayDir, vsHitPos, hitGround) )
		{
			// the ray missed the scene. Handle the environment light here.
			vec4 lightL_pdf = evaluateEnvironmentRadiance(wsRayDir);
			float misWeight = powerHeuristic(bsdfF_pdf.w, lightL_pdf.w);
			radiance += throughput * lightL_pdf.xyz  * misWeight;
			break;
		}

		bounces++;
	}

	vec3 tonemappedRadiance = pow(radiance * tonemappingExposure, vec3(1.0 / tonemappingGamma));
	outColor = vec4(tonemappedRadiance,1);
}


