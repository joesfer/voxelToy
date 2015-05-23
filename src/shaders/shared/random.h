// Thomas Wang hash http://www.burtleburtle.net/bob/hash/integer.html
// Reference from Nathan Reed's blog http://www.reedbeta.com/blog/2013/01/12/quick-and-easy-gpu-random-numbers-in-d3d11/
int hash(int seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

ivec2 randomNumberGeneratorOffset(in ivec4 seed, in int sequence)
{
	ivec2 res = textureSize(noiseTexture,0);
	int offset = hash(seed.x + int(seed.y * viewport.z)) ^ hash(sequence);
	return ivec2(offset % res.x,  (offset / res.x) % res.y);
}

vec4 rand(inout ivec2 offset)
{
	vec4 randomNumber = texelFetch(noiseTexture, offset, 0);
	ivec2 res = textureSize(noiseTexture,0);
	offset.x = (offset.x + 1) % res.x;
	if (offset.x == 0) offset.y = (offset.y + 1) % res.y;
	return randomNumber;
}


