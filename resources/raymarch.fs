#version 120

uniform sampler3D distanceTexture;
uniform vec3 volumeBoundsMin;
uniform vec3 volumeBoundsMax;

uniform vec4 viewport;
uniform float cameraNear;
uniform float cameraFar;
uniform mat4 cameraProj;
uniform mat4 cameraInverseProj;
uniform mat4 cameraInverseModelView;
uniform vec3 wsLightDir;

float rayAABBIntersection(vec3 o, vec3 d)
{
	vec3 dirfrac = vec3(1.0f) / d; 

	float t1 = (volumeBoundsMin.x - o.x) * dirfrac.x;
	float t2 = (volumeBoundsMax.x - o.x) * dirfrac.x;
	float t3 = (volumeBoundsMin.y - o.y) * dirfrac.y;
	float t4 = (volumeBoundsMax.y - o.y) * dirfrac.y;
	float t5 = (volumeBoundsMin.z - o.z) * dirfrac.z;
	float t6 = (volumeBoundsMax.z - o.z) * dirfrac.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
	if (tmax < 0)
	{
		return -1; 
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		return -1; 
	}

	return tmin; 
}

bool pointInsideAABB(vec3 p)
{
	return p.x >= volumeBoundsMin.x &&
		   p.y >= volumeBoundsMin.y &&
		   p.z >= volumeBoundsMin.z &&
		   p.x <= volumeBoundsMax.x &&
		   p.y <= volumeBoundsMax.y &&
		   p.z <= volumeBoundsMax.z;
}

float distanceField(vec3 p)
{
	vec3 invExtents = vec3(1.0f) / (volumeBoundsMax - volumeBoundsMin);
	vec3 texPos = (p - volumeBoundsMin) * invExtents;
	float distance = texture3D(distanceTexture, texPos);
	return distance;
}

vec3 normal(vec3 p)
{
	const float epsilon = 1e-3f;
	vec3 n = vec3( distanceField(p + vec3(epsilon,0,0)) - distanceField(p - vec3(epsilon,0,0)),
				   distanceField(p + vec3(0,epsilon,0)) - distanceField(p - vec3(0,epsilon,0)),
				   distanceField(p + vec3(0,0,epsilon)) - distanceField(p - vec3(0,0,epsilon)) ); 
	return normalize(n); 
}

vec4 shade(vec3 p)
{
	return vec4(max(0, dot(normal(p), -wsLightDir)));
}

vec3 screenToWorldSpace(vec3 windowSpace)
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 ndcPos;
	ndcPos.xy = ((2.0 * windowSpace.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * windowSpace.z - cameraNear - cameraFar) / (cameraFar - cameraNear);
 
	vec4 clipPos;
	clipPos.w = cameraProj[3][2] / (ndcPos.z - (cameraProj[2][2] / cameraProj[2][3]));
	clipPos.xyz = ndcPos * clipPos.w;
 
	vec4 eyePos = cameraInverseProj * clipPos;
	return (cameraInverseModelView * eyePos).xyz;
}

void main()
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 rayOrigin = (cameraInverseModelView * vec4(0,0,1,1)).xyz;
	vec3 rayDir = normalize(screenToWorldSpace(vec3(gl_FragCoord.xy, cameraNear)) - rayOrigin);
	rayOrigin.z *=-1;
	rayDir.z *=-1;
	float aabbIsectDist = rayAABBIntersection(rayOrigin, rayDir); 
	if (aabbIsectDist < 0)
	{
		gl_FragColor = vec4(0);
		return;
	}
	
	int steps = 0;
	const int MAX_STEPS = 100; 

	float rayLength = aabbIsectDist;
	bool isect = false;
	while(!isect && steps < MAX_STEPS) 
	{
		vec3 rayPoint = rayOrigin + rayLength * rayDir;
		float dist = distanceField(rayPoint); 
		if ( abs(dist) < 1e-4f )
		{
			isect = true;
			break;
		}
		rayLength += dist;
		steps++;
	}

	if ( isect )
	{
		vec3 rayPoint = rayOrigin + rayLength * rayDir;
		gl_FragColor = shade(rayPoint);
	}
	else
	{
		gl_FragColor = vec4(0);
	}
}


