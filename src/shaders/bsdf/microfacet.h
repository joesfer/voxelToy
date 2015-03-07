
float indexOfRefraction = 7.3; 
float exponent = 100; 

// Geometry term. Masking function based on V cavities model.
float G(in vec3 lsWo,
		in vec3 lsWi,
		in vec3 lsWh)
{
	float NdotWh = lsWh.y;
	float NdotWo = lsWo.y;
	float NdotWi = lsWi.y;
	float WoDotWh = abs(dot(lsWo, lsWh));
	return min(1.0, min((2.0 * NdotWh * NdotWo / WoDotWh), 
						(2.0 * NdotWh * NdotWi / WoDotWh)));
}

// Blinn-Phong microfacet distribution
// return vec2(f,pdf)
vec4 D(in vec3 reflectance,
	   in vec3 lsWo,
	   in vec3 lsWh)
{
	float powCosTheta = pow(lsWh.y, exponent);

	float f = (exponent + 2.0) * INV_TWOPI * powCosTheta;

	float cosTheta = lsWh.y;
	float woDotWh = dot(lsWo, lsWh);
	float pdf = (woDotWh <= 0.0) ? 
					0.0 :
					// This pdf already comes in terms of solid angle (the 4.0 /
					// woDotWh on the denominator)
					((exponent + 1.0) * powCosTheta) / (TWO_PI * 4.0 * woDotWh);

	return vec4(reflectance * f, pdf);
}

// Sample the Blinn microfacet distribution. Return f_pdf = vec4(f, pdf) and 
// sampled direction by value
vec3 sampleD(in vec3 reflectance,
			 in vec3 lsWo, 
			 in vec2 uniformRandomSample, 
			 out vec4 f_pdf)
{
	// Sample half-angle 
	float cosTheta = pow(uniformRandomSample.x, 1.0 / (exponent+1));
	float sinTheta = sqrt(max(0, 1.0 - cosTheta*cosTheta));
	float phi = uniformRandomSample.y * 2.0 * PI;
	vec3 lsWh = sphericalToCartesian(phi, cosTheta, sinTheta); 

	// compute incident direction by reflecting about Wh
	vec3 lsWi = -lsWo + 2.0 * dot(lsWo, lsWh) * lsWh;

	// Evaluate 
	f_pdf = D(reflectance, lsWo, lsWh);

	// Return direction
	return lsWi;
}

// Fresnel term. Schlick approximation.
float F(float cosThetaH)
{
	float sqrtR0 = (1.0 - indexOfRefraction) / (1.0 + indexOfRefraction);
	float r0 = sqrtR0*sqrtR0;
	return r0 + (1.0f - r0)*pow(1.0 - cosThetaH, 5);
}

// Calculate the BSDF value and pdf for a pair of local (tangent) space directions.
// returns vec4(f.xyz, pdf)
vec4 evaluateBSDF_Microfacet(in vec3 albedo, 
							 in vec3 lsWo, 
							 in vec3 lsWi)
{
	// Microfacet model (Torrance-Sparrow)
	float cosThetaWi = lsWi.y; // angle between normal and Wi
	float cosThetaWo = lsWo.y; // angle between normal and Wo
	vec3 lsWh = normalize(lsWi + lsWo);
	float cosThetaH = dot(lsWi, lsWh); // angle between Wh and Wi

	float pdf;
	// D = microfacet distribution
	// G = geometric term (self shadowing)
	// F = Fresnel term
	vec4 f_pdf = D(albedo, lsWo, lsWh);
	f_pdf.xyz *= G(lsWo, lsWi, lsWh) * F(cosThetaH) / (4.0 * cosThetaWo * cosThetaWi);
	return f_pdf;
}

// Sample BSDF function by first choosing an incoming direction lsWi, given the
// outgoing lsWo direction, and return the result of evaluating the bsdf for
// the pair of directions. Note the supplied vectors are in local (tangent)
// space, because it's easier to generate them this way, but will need to be
// converted back to world space later.
//
// Returns the sampled direction in local space, lsWi, and a vec4 f_pdf
// containing the value of the bsdf and corresponding pdf for the sampled 
// directions.
vec3 sampleBSDF_Microfacet(in vec3 albedo,
						   in vec3 lsWo,
						   inout ivec2 rngOffset,
						   out vec4 f_pdf)
{
	vec2 uniformRandomSample = rand(rngOffset).xy;
	vec3 lsWi = sampleD(albedo, lsWo, uniformRandomSample, f_pdf);

	float cosThetaWi = lsWi.y; // angle between normal and Wi
	float cosThetaWo = lsWo.y; // angle between normal and Wo
	vec3 lsWh = normalize(lsWi + lsWo);
	float cosThetaH = dot(lsWi, lsWh); // angle between Wh and Wi

	f_pdf.xyz = albedo 
				* f_pdf.xyz
				* G(lsWo, lsWi, lsWh)
				* F(cosThetaH)
				/ (4.0 * cosThetaWo * cosThetaWi);
	
	return lsWi; 
}


