#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include "renderer/renderer.h"

void Renderer::requestAction(float x, float y, 
							 float dx, float dy,
							 Action::PICKING_ACTION action,
							 bool restartAccumulation)
{
	Action a;
	a.m_point.x = x;
	a.m_point.y = y;
	a.m_velocity.x = dx;
	a.m_velocity.y = dy;
	a.m_type = action;
	a.m_invalidatesRender = restartAccumulation;
	m_scheduledActions.push_back(a);
}

void Renderer::processPendingActions()
{
	for( size_t i = 0; i < m_scheduledActions.size(); ++i )
	{	
		const Action& a = m_scheduledActions[i];
		if (a.m_invalidatesRender) 
		{
			// restart accumulation on next visible frame
			m_numberSamples = 0;
		}

		Imath::V2f position = Imath::V2f(a.m_point.x, 1.0f - a.m_point.y) * m_renderSettings.m_imageResolution;
		Imath::V2f velocity = a.m_velocity * m_renderSettings.m_imageResolution;
		RendererServiceType requiredService = SERVICE_TOTAL;

		switch(a.m_type)
		{
			case Action::PA_SELECT_FOCAL_POINT:  requiredService = SERVICE_SET_FOCAL_DISTANCE ; break;
			case Action::PA_SELECT_ACTIVE_VOXEL: requiredService = SERVICE_SELECT_ACTIVE_VOXEL; break;
			case Action::PA_ADD_VOXEL:           requiredService = SERVICE_ADD_VOXEL          ; break;
			case Action::PA_REMOVE_VOXEL:        requiredService = SERVICE_REMOVE_VOXEL       ; break;
			default: break;
		}

		if ( m_services[requiredService] != NULL )
		{
			m_services[requiredService]->setMouseParameters(position, velocity);
			m_services[requiredService]->execute();
		}
	}
	glUseProgram(0);
	m_scheduledActions.resize(0);
}


