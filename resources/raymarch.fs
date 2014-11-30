#version 120

uniform sampler3D distanceTexture;
uniform vec3 volumeBoundsMin;
uniform vec3 volumeBoundsMax;

uniform vec4 viewport;
uniform mat4 cameraInverseProj;
uniform mat4 cameraInverseModelView;
uniform vec3 wsLightDir;

varying vec2 texCoord;

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

vec3 screenToWorldSpace(vec2 screenPos)
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec4 ndcPos;
	ndcPos.xy = ((2.0 * screenPos) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / (gl_DepthRange.far - gl_DepthRange.near);
	ndcPos.w = 1.0;
	vec4 clipPos = ndcPos / gl_FragCoord.w;
	vec4 eyePos = clipPos * cameraInverseProj; 
	eyePos /= eyePos.w;
	eyePos.z *= -1;
	return (cameraInverseModelView * eyePos).xyz;
}

void main()
{
	// https://www.opengl.org/wiki/Compute_eye_space_from_window_space
	vec3 rayOrigin = (cameraInverseModelView * vec4(0,0,0,1)).xyz;
	vec3 rayDir = normalize(screenToWorldSpace(gl_FragCoord.xy) - rayOrigin);

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


