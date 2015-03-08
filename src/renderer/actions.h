#pragma once

#include <OpenEXR/ImathVec.h>

struct Action
{
	enum PICKING_ACTION
	{
		PA_SELECT_FOCAL_POINT,
		PA_SELECT_ACTIVE_VOXEL,
		PA_ADD_VOXEL,
		PA_REMOVE_VOXEL
	};
	
	PICKING_ACTION m_type;
	// Normalized coordinates in screen space from where the requested picking
	// action is originated (the actual pixel coordinates are calculated as
	// m_pickingActionPoint * viewportSize;
	Imath::V2f m_point;
	// screen-space normalized movement 
	Imath::V2f m_velocity;
	bool m_invalidatesRender;
};


