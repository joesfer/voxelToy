#version 130

uniform sampler2D sampleTexture;
uniform sampler2D averageTexture;
uniform int sampleCount;
uniform vec4 viewport;

void main()
{
	ivec2 uv = ivec2(gl_FragCoord.xy);
	vec4 sample = texelFetch(sampleTexture, uv, 0);
	vec4 average = texelFetch(averageTexture, uv, 0);

	// running average
	gl_FragColor = (sample + average * sampleCount) / (sampleCount + 1);
}
