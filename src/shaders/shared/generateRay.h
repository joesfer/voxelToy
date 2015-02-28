void generateRay_Orthographic(in vec3 fragmentPos, 
							  out vec3 wsRayOrigin, 
							  out vec3 wsRayDir)
{
	wsRayOrigin = (cameraInverseModelView * screenToEyeSpaceOrthographic(fragmentPos)).xyz;
	wsRayDir = (cameraInverseModelView * vec4(0,0,-1,0)).xyz;
}

#ifdef PINHOLE
void generateRay_Pinhole(in vec3 fragmentPos, 
						 inout ivec2 rngOffset,
						 out vec3 wsRayOrigin, 
						 out vec3 wsRayDir)
{
	vec4 uniformRandomSample = rand(rngOffset);
	vec3 jitter =  vec3(uniformRandomSample.x - 0.5,
					    uniformRandomSample.y - 0.5, 
						0);

	vec4 esOriginPos = vec4(0,0,0,1);
	vec4 esFragmentPos = screenToEyeSpacePerspective(fragmentPos + jitter);

	wsRayOrigin = (cameraInverseModelView * esOriginPos).xyz;
	vec3 wsFragmentPos = (cameraInverseModelView * esFragmentPos).xyz;
	wsRayDir = normalize(cameraInverseModelView * vec4(normalize(esFragmentPos.xyz), 0)).xyz;
}
#endif

#ifdef THINLENS
void generateRay_ThinLens(in vec3 fragmentPos, 
						  inout ivec2 rngOffset,
						  out vec3 wsRayOrigin, 
						  out vec3 wsRayDir)
{
	// thin lens model

	/*  

	    |                   From PBRT:
	    I                   We know _all_ rays from a given image sample (I) going
	    |\                  through the lens (I->*) will converge on the same 
	    | \   lens          point in the focal plane (F). This includes the sample 
	    |  \  /\            going through the center of the lens, which is not
	    |   \/ *\           refracted and matches the pinhole camera model. Thus
	    |    |  |           to find F, we calculate the intersection of I-># with
	    |    |\#|           the focal plane, and then set the ray direction to
	    |    |  |           be * -> F
	    |    \* /\      :
	    |     \/  \     :
	               \    :   I = image (film) sample
	    film   |    \   :   * = samples on the lens
	                 \  :   # = sample on the center of the lens = pinhole sample
	    |<---->|      \ :   F = intersection of I -> # on the focal plane, which
	       s2          \:       is also where any ray I->* will converge once
	           |        F       refracted by the lens.
	                    :\
	           |        : \
	                    :
	           |        focal plane
	           |<------>|
	               s1
	  
	  
	   Z <----|-----------> -Z (OpenGL style)
	 
	 */

	// Note that the correct interpretation (http://en.wikipedia.org/wiki/Focal_length)
	// would be: 1/f = 1/s1 + 1/s2; 
	// 1/focalLength = 1/focalDistance + 1/imagePlaneToLensDistance
	// however this makes imagePlaneToLensDistance change with the focal point,
	// and that in turn affects the FOV of the camera (this _does_ happen in real
	// cameras, which is known in photography as "focus breathing"). However this 
	// is quite confusing so we can simply lock down s2 to be the focal length and 
	// s1 the focal distance, and force the convergence of rays as described in 
	// the comment above.
	float cameraFocalDistance = FocalDistanceData.focalDistance;

	vec4 uniformRandomSample = rand(rngOffset);
	vec2 unitDiskSample = uniformlySampleDisk(uniformRandomSample.xy);
	vec3 esLensSamplePoint = vec3(unitDiskSample * cameraLensRadius, 0);

	vec3 esRayOrigin = screenToEyeSpacePerspective(fragmentPos).xyz;

	vec3 esRayDir;
	esRayDir = normalize(esRayOrigin); 

	float t = cameraFocalDistance / -(esRayDir.z); // rayDir.z will be negative
	
	// work out the intersection with the focal plane using a ray starting at
	// the image plane sample, going through the center of the lens. This is
	// what would happen in a pinhole (i.e. completely sharp) camera, but all
	// our lens samples do converge at exactly the same point after refraction. 
	//vec3 esFocalPlanePoint = esImagePlanePoint + t * esImagePlaneToLensCenter;
	vec3 esFocalPlanePoint = esRayOrigin + t * esRayDir;
	vec3 wsFocalPlanePoint = (cameraInverseModelView * vec4(esFocalPlanePoint, 1)).xyz;


	wsRayOrigin = (cameraInverseModelView * vec4(esLensSamplePoint,1)).xyz;
	wsRayDir = normalize(wsFocalPlanePoint - wsRayOrigin); 
}
#endif

void generateRay(vec3 frag, inout ivec2 rngOffset, out vec3 wsRayOrigin, out vec3 wsRayDir)
{
	if (cameraLensModel == 0)
	{
		generateRay_Pinhole(frag, rngOffset, wsRayOrigin, wsRayDir);
	}
	else if(cameraLensModel == 1)
	{
#ifdef THINLENS
		generateRay_ThinLens(frag, rngOffset, wsRayOrigin, wsRayDir);
#else
		generateRay_Pinhole(frag, rngOffset, wsRayOrigin, wsRayDir);
#endif
	}
	else
	{
		generateRay_Orthographic(frag, wsRayOrigin, wsRayDir);
	}

}

