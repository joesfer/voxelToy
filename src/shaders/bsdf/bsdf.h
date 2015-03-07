
#define BSDF_LAMBERTIAN 0
#define BSDF_MICROFACET 1

//int materialType = BSDF_LAMBERTIAN;
int materialType = BSDF_MICROFACET;

// Calculate the BSDF value and pdf for a pair of local (tangent) space directions.
// returns vec4(f.xyz, pdf)
vec4 evaluateBSDF(in vec3 albedo, 
				  in vec3 lsWo, 
				  in vec3 lsWi)
{
	if(materialType == BSDF_MICROFACET)
	{
		return evaluateBSDF_Microfacet(albedo, 
									   lsWo, 
									   lsWi);
	}
	else
	{
		return evaluateBSDF_Lambertian(albedo, 
									   lsWo, 
									   lsWi);
	}
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
vec3 sampleBSDF(in vec3 albedo, 
				in vec3 lsWo, 
				inout ivec2 rngOffset, 
				out vec4 f_pdf)
{
	if(materialType == BSDF_MICROFACET)
	{
		return sampleBSDF_Microfacet(albedo, 
									 lsWo, 
									 rngOffset, 
									 f_pdf);
	}
	else
	{
		return sampleBSDF_Lambertian(albedo, 
									 lsWo, 
									 rngOffset, 
									 f_pdf);
	}
}

