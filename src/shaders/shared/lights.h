vec3 getBackgroundColor(in vec3 v)
{
	float bias = max(0.0, v.y);
	return (backgroundColorBottom * (1.0 - bias) + backgroundColorTop * bias);
}

// returns vec4(L.rgb, pdf)
vec4 evaluateEnvironmentRadiance(in vec3 wsWi) // note how direction is irrelevant in this case
{
	float pdf = 1.0 / (4.0 * PI); // uniformly sampled sphere 
	return vec4(getBackgroundColor(wsWi), pdf);
}

// returns radiance and wsW_pdf = vec4(wsW.xyz, pdf)
vec3 sampleEnvironmentRadiance(in Basis surfaceBasis, 
							   inout ivec2 rngOffset, 
							   out vec4 wsW_pdf)
{
	vec4 lsW_pdf = uniformlySampledHemisphere(rand(rngOffset).xy);
	wsW_pdf = vec4(localToWorld(lsW_pdf.xyz, surfaceBasis), lsW_pdf.w);
	return getBackgroundColor(wsW_pdf.xyz);
}


