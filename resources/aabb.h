float rayAABBIntersection(vec3 o, vec3 d)
{
	vec3 invDir = vec3(1.0f) / d; 

	vec3 tMin3 = (volumeBoundsMin - o) * invDir;
	vec3 tMax3 = (volumeBoundsMax - o) * invDir;

	vec3 tmin = min(tMin3, tMax3);
	float tminf = max(tmin.x, max(tmin.y, tmin.z));
	vec3 tmax = max(tMin3, tMax3);
	float tmaxf = min(tmax.x, min(tmax.y, tmax.z));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
	if (tmaxf < 0)
	{
		return -1; 
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tminf > tmaxf)
	{
		return -1; 
	}

	return tminf >= 0 ? tminf : tmaxf;
}


