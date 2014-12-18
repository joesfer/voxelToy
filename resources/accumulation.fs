#version 120

uniform sampler2D sampleTexture;
uniform sampler2D averageTexture;
uniform int sampleCount;
uniform vec4 viewport;

void main()
{
	vec2 uv = gl_FragCoord.xy / viewport.zw;
	vec4 sample = texture2D(sampleTexture, uv);
	vec4 average = texture2D(averageTexture, uv);

	// running average
	gl_FragColor = (sample + average * sampleCount) / (sampleCount + 1);
}
