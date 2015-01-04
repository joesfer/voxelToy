// Sample BSDF function by first choosing an incoming direction lsWi, given the
// outgoing lsWo direction, and return the result of evaluating the bsdf for
// the pair of directions. Note the supplied vectors are in local (tangent)
// space, because it's easier to generate them this way, but will need to be
// converted back to world space later.
//
// Returns the sampled direction in local space, lsWi, and a vec4 f_pdf
// containing the value of the bsdf and corresponding pdf for the sampled 
// directions.
vec3 sampleBSDF(in vec3 albedo, in vec3 lsWo, inout ivec2 rngOffset, out vec4 f_pdf)
{
	// hard-coded Lambertian BSDF
	vec4 lsWi_pdf = cosineSampledHemisphere(rand(rngOffset).xy);
	f_pdf.xyz = albedo / PI; // f 
	f_pdf.w = lsWi_pdf.w; // pdf <-- this already includes the division by PI to express the PDF in terms of solid angle
	return lsWi_pdf.xyz; 
}

// Calculate the BSDF value and pdf for a pair of local (tangent) space directions.
// returns vec4(f.xyz, pdf)
vec4 evaluateBsdf(in vec3 albedo, in vec3 lsWo, in vec3 lsWi)
{
	// hard-coded Lambertian BSDF. This follows the same logic as the
	// cosine-weighted hemiphere sampling.
	float pdf = lsWi.y / PI;
	vec3 f = albedo / PI;
	return vec4(f,pdf);
}


