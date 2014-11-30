#version 120

varying vec2 texCoord;

void main()
{
	// simply copy the input texture coordinate
	texCoord = gl_MultiTexCoord0.st;

	// don't apply any sort of transform on the geometry, this is a screen-space
	// shader.
	gl_Position = ftransform();
}

