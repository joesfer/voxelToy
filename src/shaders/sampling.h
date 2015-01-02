float PI = 3.14159265358979323846264; 

/** @brief generate random samples in a disk
*
* given two uniform random variables u1 and u2, produce
* a sample x, y on the unit disk centered at the origin
* Shirley 1997.
*/
vec2 sampleDisk( vec2 u )
{
	float r, theta;
	// Map uniform random numbers to [-1,1]^2
	vec2 s = u * 2.0 - vec2(1.0);
	// Map square to (r,\theta)
	// Handle degeneracy at the origin
	if (s.x == 0.0 && s.y == 0.0)
	{
		return vec2(0.0);
	}
	if (s.x >= -s.y) {
		if (s.x > s.y) {
			// Handle first region of disk
			r = s.x;
			if (s.y > 0.0)
				theta = s.y/r;
			else
				theta = 8.0 + s.y/r;
		}
		else {
			// Handle second region of disk
			r = s.y;
			theta = 2.0 - s.x/r;
		}
	}
	else {
		if (s.x <= s.y) {
			// Handle third region of disk
			r = -s.x;
			theta = 4.0 - s.y/r;
		}
		else {
			// Handle fourth region of disk
			r = -s.y;
			theta = 6.0 + s.x/r;
		}
	}
	theta *= PI * 0.25; 
	vec2 sample = vec2(cos(theta), sin(theta)) * r;
	return sample;
}

// pole at +Y (theta = 0)
vec3 polarToVector(float phi, float theta)
{
	float sinTheta = sin(theta);
	return vec3( sinTheta * cos(phi), cos(theta), sinTheta * sin(phi) );
}
