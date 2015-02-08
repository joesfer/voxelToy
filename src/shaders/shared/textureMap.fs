#version 130

uniform sampler2D texture;
uniform vec4 viewport;

out vec4 outColor;

void main()
{
	outColor = texture2D(texture, (gl_FragCoord.xy - viewport.xy) / viewport.zw);
}

