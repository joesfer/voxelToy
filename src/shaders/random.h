float hash( float n ) { return fract(sin(n)*43758.5453123); }

ivec2 randomNumberGeneratorOffset(in ivec4 seed, in int sequence)
{
	ivec2 res = textureSize(noiseTexture,0);
	int offset = int((seed.x + seed.y * res.x) + hash(sequence) * (res.x * res.y));
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


