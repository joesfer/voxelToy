vec4 evaluateMaterialBSDF_Metal(in int materialDataOffset,
								in vec3 lsWo, 
								in vec3 lsWi)
{
	//vec3 emission = vec3(texelFetch(materialDataTexture, materialDataOffset + 0, 0).r,
	//					   texelFetch(materialDataTexture, materialDataOffset + 1, 0).r,
	//			  		   texelFetch(materialDataTexture, materialDataOffset + 2, 0).r);

	vec3 reflectance = vec3(texelFetch(materialDataTexture, materialDataOffset + 3, 0).r,
						    texelFetch(materialDataTexture, materialDataOffset + 4, 0).r,
						    texelFetch(materialDataTexture, materialDataOffset + 5, 0).r);

	float roughness = texelFetch(materialDataTexture, materialDataOffset + 6, 0).r;
	float exponent = roughness; // TODO
	
	return evaluateBSDF_Microfacet(reflectance, exponent, lsWo, lsWi);
}

vec3 sampleMaterialBSDF_Metal(in int materialDataOffset, 
							  in vec3 lsWo, 
							  inout ivec2 rngOffset, 
							  out vec4 f_pdf)
{
	//vec3 emission = vec3(texelFetch(materialDataTexture, materialDataOffset + 0, 0).r,
	//					   texelFetch(materialDataTexture, materialDataOffset + 1, 0).r,
	//			  		   texelFetch(materialDataTexture, materialDataOffset + 2, 0).r);

	vec3 reflectance = vec3(texelFetch(materialDataTexture, materialDataOffset + 3, 0).r,
						    texelFetch(materialDataTexture, materialDataOffset + 4, 0).r,
						    texelFetch(materialDataTexture, materialDataOffset + 5, 0).r);

	float roughness = texelFetch(materialDataTexture, materialDataOffset + 6, 0).r;
	float exponent = roughness; // TODO

	return sampleBSDF_Microfacet(reflectance, exponent, lsWo, rngOffset, f_pdf);
}

vec3 emissionMaterialBSDF_Metal(in int materialDataOffset)
{
	vec3 emission = vec3(texelFetch(materialDataTexture, materialDataOffset + 0, 0).r,
					     texelFetch(materialDataTexture, materialDataOffset + 1, 0).r,
					     texelFetch(materialDataTexture, materialDataOffset + 2, 0).r);

	return emission;
}



