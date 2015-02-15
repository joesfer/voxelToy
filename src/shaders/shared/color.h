float luminance(in vec3 rgb)
{
	// Compute luminance via a weighted sum of R,G,B
	// (assuming Rec709 primaries and a linear scale)
	// This needs to match how we computed the background radiance integral on
	// the host side (image.cpp)
	return dot(rgb, vec3(0.2126f, 0.7152f, 0.0722f));
}


