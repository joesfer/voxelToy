#pragma once 

#include <OpenEXR/ImathVec.h>
#include <string>

struct RenderSettings
{
	// maximum path length allowed in the path tracer (1 = direct
	// illumination only).
	int m_pathtracerMaxNumBounces;

    // Max number of accumulated samples before the render finishes
    int m_pathtracerMaxSamples;

	// rendered image resolution in pixels
	Imath::V2i m_imageResolution;

	// Viewport within which to render the image. This may not match the
	// resolution of the rendered image, in which case stretching or squashing
	// will occur.
	int m_viewport[4];

	float m_wireframeOpacity;
	float m_wireframeThickness;

	std::string m_backgroundImage;
	Imath::V3f m_backgroundColor[2]; // gradient (top/bottom)
	int m_backgroundRotationDegrees;
};



