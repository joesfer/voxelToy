#include <shared/color.h>
#include <envLight/envMapSample.h>

vec3 getBackgroundColor(in vec3 v)
{
	if (backgroundUseImage != 0)
	{
		// sample from texture map
		return texture(backgroundTexture, uvCoordFromVector(v)).rgb;
	}
	else
	{
		// use gradient colors
		float bias = max(0.0, v.y);
		return (backgroundColorBottom * (1.0 - bias) + backgroundColorTop * bias);
	}
}

// returns vec4(L.rgb, pdf)
vec4 evaluateEnvironmentRadiance(in vec3 wsWi) 
{
	if (backgroundUseImage != 0)
	{
		const vec3 radiance = getBackgroundColor(wsWi); 
		float pdf = luminance(radiance) / backgroundIntegral; 
		return vec4(radiance, pdf);
	}
	else
	{
		float pdf = 1.0 / (4.0 * PI); // uniformly sampled sphere 
		return vec4(getBackgroundColor(wsWi), pdf);
	}
}

// returns radiance and wsW_pdf = vec4(wsW.xyz, pdf)
vec3 sampleEnvironmentRadiance(in Basis surfaceBasis, 
							   inout ivec2 rngOffset, 
							   out vec4 wsW_pdf)
{
	if (backgroundUseImage != 0)
	{
		const vec2 uniformRandomSample = rand(rngOffset).xy;
		const vec2 uv = sampleEnvironmentTexture(uniformRandomSample);
		const vec3 wsW = directionFromUVCoord(uv);
		const vec3 radiance = texture(backgroundTexture, uv).rgb;
		const float pdf = luminance(radiance) / backgroundIntegral;
		wsW_pdf = vec4(wsW, pdf);
		return radiance;
	}
	else
	{
		vec4 lsW_pdf = uniformlySampledHemisphere(rand(rngOffset).xy);
		wsW_pdf = vec4(localToWorld(lsW_pdf.xyz, surfaceBasis), lsW_pdf.w);
		return getBackgroundColor(wsW_pdf.xyz);
	}
}


