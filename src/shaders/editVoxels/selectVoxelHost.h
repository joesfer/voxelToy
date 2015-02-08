#pragma once

// Host-side declaration of the Shader Storage Object struct used to read and
// store the focal distance

typedef struct 
{
	int index[4];
	float normal[4];
} SelectVoxelData;
