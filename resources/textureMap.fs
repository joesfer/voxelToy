#version 120

uniform sampler2D texture;
uniform vec4 viewport;

void main()
{
	vec2 uv = gl_FragCoord.xy / viewport.zw;
	gl_FragColor = vec4(0.1) + texture2D(texture, uv);
}

