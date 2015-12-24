#include <shared/color.h>
#include <envLight/envMapSample.h>

vec3 getBackgroundAverageColor()
{
	if (backgroundUseImage != 0)
	{
		// can't calculate this
		return vec3(0);
	}
	else
	{
		// use gradient colors
		float bias = 0.5; 
		return (backgroundColorBottom * (1.0 - bias) + backgroundColorTop * bias);
	}
}

vec3 getBackgroundColor(in vec3 v)
{
	if (backgroundUseImage != 0)
	{
		// sample from texture map
		return texture(backgroundTexture, uvCoordFromVector(v, backgroundRotationRadians)).rgb;
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
							   in vec2 uniformRandomSample, 
							   out vec4 wsW_pdf)
{
	if (backgroundUseImage != 0)
	{
		const vec2 uv = sampleEnvironmentTexture(uniformRandomSample);
		const vec3 wsW = directionFromUVCoord(uv, backgroundRotationRadians);
		const vec3 radiance = texture(backgroundTexture, uv).rgb;
		const float pdf = luminance(radiance) / backgroundIntegral;
		wsW_pdf = vec4(wsW, pdf);
		return radiance;
	}
	else
	{
		vec4 lsW_pdf = uniformlySampledHemisphere(uniformRandomSample);
		wsW_pdf = vec4(localToWorld(lsW_pdf.xyz, surfaceBasis), lsW_pdf.w);
		return getBackgroundColor(wsW_pdf.xyz);
	}
}

// Power is total amount of energy passing through a surface or region of space
// per unit time.
float getBackgroundLightPower()
{
	if (backgroundUseImage != 0)
	{
		return backgroundIntegral; 
	}
	else
	{
		const float sceneRadius = length(volumeBoundsMax - volumeBoundsMin) * 0.5;
		return luminance(getBackgroundAverageColor()) * (4.0 * PI * sceneRadius * sceneRadius);
	}
}
