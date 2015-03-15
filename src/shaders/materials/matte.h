vec4 evaluateMaterialBSDF_Matte(in int materialDataOffset,
								in vec3 lsWo, 
								in vec3 lsWi)
{
	vec3 albedo = vec3(texelFetch(materialDataTexture, materialDataOffset + 3, 0).r,
					   texelFetch(materialDataTexture, materialDataOffset + 4, 0).r,
					   texelFetch(materialDataTexture, materialDataOffset + 5, 0).r);
	
	return evaluateBSDF_Lambertian(albedo, lsWo, lsWi);
}

vec3 sampleMaterialBSDF_Matte(in int materialDataOffset,
							  in vec3 lsWo, 
							  inout ivec2 rngOffset, 
							  out vec4 f_pdf)
{
	vec3 albedo = vec3(texelFetch(materialDataTexture, materialDataOffset + 3, 0).r,
					   texelFetch(materialDataTexture, materialDataOffset + 4, 0).r,
					   texelFetch(materialDataTexture, materialDataOffset + 5, 0).r);
	
	return sampleBSDF_Lambertian(albedo, lsWo, rngOffset, f_pdf);
}

vec3 emissionMaterialBSDF_Matte(in int materialDataOffset)
{
	vec3 emission = vec3(texelFetch(materialDataTexture, materialDataOffset + 0, 0).r,
					     texelFetch(materialDataTexture, materialDataOffset + 1, 0).r,
					     texelFetch(materialDataTexture, materialDataOffset + 2, 0).r);

	return emission;
}

