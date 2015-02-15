#version 430

#include <editVoxels/selectVoxelDevice.h>

uniform vec4		newVoxelColor = vec4(0,0,1,0);
uniform mat4        cameraInverseModelView;
uniform vec2		screenSpaceMotion;

uniform vec3        groundColor = vec3(0.5, 0.5, 0.5);

//Voxel output
layout(r8ui, binding = 0) uniform uimage3D voxelOccupancy;
layout(rgba8, binding = 1) uniform image3D voxelColor;


void main()
{
	vec3 cameraRight = (cameraInverseModelView * vec4(1,0,0,0)).xyz;
	vec3 cameraUp = (cameraInverseModelView * vec4(0,1,0,0)).xyz;

	vec3 absCameraRight = abs(cameraRight); 
	vec3 absCameraUp = abs(cameraUp); 

	vec3 aaCameraRight = step(absCameraRight.yxx, absCameraRight.xyz) * step(absCameraRight.zzy, absCameraRight.xyz) * sign(cameraRight);
	vec3 aaCameraUp = step(absCameraUp.yxx, absCameraUp.xyz) * step(absCameraUp.zzy, absCameraUp.xyz) * sign(cameraUp);

	vec2 absScreenSpaceMotion = abs(screenSpaceMotion); 
	vec2 mask = step(absScreenSpaceMotion.yx, absScreenSpaceMotion.xy);
	vec2 dominantMotion = mask * sign(screenSpaceMotion);
	vec3 normal = any(bvec2(absScreenSpaceMotion)) ? 
					(aaCameraRight * dominantMotion.x + aaCameraUp * dominantMotion.y) :
					SelectVoxelData.normal.xyz;
				  

	ivec3 coord = SelectVoxelData.index.xyz + ivec3(normal);
	imageStore(voxelOccupancy, coord, uvec4(255));
	vec4 prevColor = imageLoad(voxelOccupancy, SelectVoxelData.index.xyz).r > 0 ?
						imageLoad(voxelColor, SelectVoxelData.index.xyz) :
						vec4(groundColor,1);
	imageStore(voxelColor, coord, prevColor);
}

