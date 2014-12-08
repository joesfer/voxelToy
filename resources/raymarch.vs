#version 120

void main()
{
	// don't apply any sort of transform on the geometry, this is a screen-space
	// shader.
	gl_Position = ftransform();
}

