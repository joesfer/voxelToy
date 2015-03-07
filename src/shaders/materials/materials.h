#define MATERIAL_MATTE   0
#define MATERIAL_METAL   1
#define MATERIAL_PLASTIC 2

// Calculate the BSDF value and pdf for a pair of local (tangent) space directions.
// returns vec4(f.xyz, pdf)
vec4 evaluateMaterialBSDF(in int materialType,
						  in vec3 albedo, 
						  in vec3 lsWo, 
						  in vec3 lsWi)
{
	switch(materialType)
	{
		case MATERIAL_MATTE:   return evaluateMaterialBSDF_Matte(albedo, lsWo, lsWi);
		case MATERIAL_METAL:   return evaluateMaterialBSDF_Metal(albedo, lsWo, lsWi);
		case MATERIAL_PLASTIC: return evaluateMaterialBSDF_Plastic(albedo, lsWo, lsWi);
		default: return vec4(0);
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
vec3 sampleMaterialBSDF(in int materialType,
						in vec3 albedo, 
						in vec3 lsWo, 
						inout ivec2 rngOffset, 
						out vec4 f_pdf)
{
	switch(materialType)
	{
		case MATERIAL_MATTE:   return sampleMaterialBSDF_Matte(albedo, lsWo, rngOffset, f_pdf);
		case MATERIAL_METAL:   return sampleMaterialBSDF_Metal(albedo, lsWo, rngOffset, f_pdf);
		case MATERIAL_PLASTIC: return sampleMaterialBSDF_Plastic(albedo, lsWo, rngOffset, f_pdf);
		default: return vec3(0);
	}
}

