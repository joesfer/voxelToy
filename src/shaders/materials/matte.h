vec4 evaluateMaterialBSDF_Matte(in vec3 albedo, 
								in vec3 lsWo, 
								in vec3 lsWi)
{
	return evaluateBSDF_Lambertian(albedo, lsWo, lsWi);
}

vec3 sampleMaterialBSDF_Matte(in vec3 albedo, 
							  in vec3 lsWo, 
							  inout ivec2 rngOffset, 
							  out vec4 f_pdf)
{
	return sampleBSDF_Lambertian(albedo, lsWo, rngOffset, f_pdf);
}

