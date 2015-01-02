#version 130

uniform sampler2D texture;
uniform vec4 viewport;

out vec4 outColor;

void main()
{
	outColor = texelFetch(texture, ivec2(gl_FragCoord.xy), 0);
}

