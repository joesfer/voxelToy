#version 130

uniform sampler2D texture;
uniform vec4 viewport;

void main()
{
	gl_FragColor = texelFetch(texture, ivec2(gl_FragCoord.xy), 0);
}

