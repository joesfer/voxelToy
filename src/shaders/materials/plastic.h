vec4 evaluateMaterialBSDF_Plastic(in vec3 albedo, 
								in vec3 lsWo, 
								in vec3 lsWi)
{
	return evaluateBSDF_Microfacet(albedo, lsWo, lsWi);
}

vec3 sampleMaterialBSDF_Plastic(in vec3 albedo, 
							  in vec3 lsWo, 
							  inout ivec2 rngOffset, 
							  out vec4 f_pdf)
{
	return sampleBSDF_Microfacet(albedo, lsWo, rngOffset, f_pdf);
}



