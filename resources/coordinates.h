vec4 screenToEyeSpace(vec3 windowSpace)
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 ndcPos;
	ndcPos.xy = ((2.0 * windowSpace.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * windowSpace.z - cameraNear - cameraFar) / (cameraFar - cameraNear);
 
	vec4 clipPos;
	clipPos.w = cameraProj[3][2] / (ndcPos.z - (cameraProj[2][2] / cameraProj[2][3]));
	clipPos.xyz = ndcPos * clipPos.w;
 
	vec4 eyePos = cameraInverseProj * clipPos;
	return eyePos;
}

vec3 screenToWorldSpace(vec3 windowSpace)
{
	vec4 eyePos = screenToEyeSpace(windowSpace);
	return (cameraInverseModelView * eyePos).xyz;
}


